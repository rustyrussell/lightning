#include <arpa/inet.h>
#include <assert.h>
#include <ccan/io/io.h>
#include <ccan/str/hex/hex.h>
#include <ccan/tal/str/str.h>
#include <common/type_to_string.h>
#include <common/utils.h>
#include <common/wireaddr.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <wire/wire.h>


#define BASE32DATA "abcdefghijklmnopqrstuvwxyz234567"


char *b32_encode(char *dst, u8 *src, u8 ver) {
	u16 byte = 0,
	poff = 0;
	for(; byte < 	((ver==2)?16:56); poff += 5)
	{
	if(poff > 7) {
	poff -= 8;
	src++;
	}
	dst[byte++] = BASE32DATA[ (htobe16(*(u16*)src) >> (11 -poff)) & (u16)0x001F];
	}
	dst[byte] = 0;
	return dst;
}

//FIXME quiknditry

static int b32_decode( u8 *dst,u8 *src,u8 ver) {
	int rem = 0;

	int i;
	u8 *p=src;
	int buf;
	u8 ch;
	for (i=0; i < ((ver==2)?16:56) ; p++) {
	ch = *p;
	buf <<= 5;
	if ( (ch >= 'a' && ch <= 'z')) {
	ch = (ch & 0x1F) - 1;
	} else
	if (ch >= '2' && ch <= '7') {
			ch -= '2' - 0x1A ;
	} else {
			return -1;
	}
	buf = buf | ch;
	rem = rem + 5;
	if (rem >= 8) {
		dst[i++] = buf >> (rem - 8);
		rem -= 8;
	}
		}
return 0;
}


/* Returns false if we didn't parse it, and *cursor == NULL if malformed. */
bool fromwire_wireaddr(const u8 **cursor, size_t *max, struct wireaddr *addr)
{
	addr->type = fromwire_u8(cursor, max);

	switch (addr->type) {
	case ADDR_TYPE_IPV4:
		addr->addrlen = 4;
		break;
	case ADDR_TYPE_IPV6:
		addr->addrlen = 16;
		break;
	case ADDR_TYPE_TOR_V2:
		addr->addrlen = TOR_V2_ADDRLEN;
		break;
	case ADDR_TYPE_TOR_V3:
		addr->addrlen = TOR_V3_ADDRLEN;
		break;

	default:
		return false;
	}
	fromwire(cursor, max, addr->addr, addr->addrlen);
	addr->port = fromwire_u16(cursor, max);

	return *cursor != NULL;
}


void towire_wireaddr(u8 **pptr, const struct wireaddr *addr)
{
	if (!addr || addr->type == ADDR_TYPE_PADDING) {
		towire_u8(pptr, ADDR_TYPE_PADDING);
		return;
	}
	towire_u8(pptr, addr->type);
	towire(pptr, addr->addr, addr->addrlen);
	towire_u16(pptr, addr->port);
}

char *fmt_wireaddr(const tal_t *ctx, const struct wireaddr *a)
{
	char addrstr[FQDN_ADDRLEN];
	char *ret, *hex;

	switch (a->type) {
	case ADDR_TYPE_IPV4:
		if (!inet_ntop(AF_INET, a->addr, addrstr, INET_ADDRSTRLEN))
			return "Unprintable-ipv4-address";
		return tal_fmt(ctx, "%s:%u", addrstr, a->port);
	case ADDR_TYPE_IPV6:
		if (!inet_ntop(AF_INET6, a->addr, addrstr, INET6_ADDRSTRLEN))
			return "Unprintable-ipv6-address";
		return tal_fmt(ctx, "[%s]:%u", addrstr, a->port);
	case ADDR_TYPE_TOR_V2:
		return tal_fmt(ctx, "%s.onion:%u", b32_encode(addrstr, (u8 *)a->addr,2), a->port);
	case ADDR_TYPE_TOR_V3:
		return tal_fmt(ctx, "%s.onion:%u", b32_encode(addrstr, (u8 *)a->addr,3), a->port);
		case ADDR_TYPE_PADDING:
		break;
	}

	hex = tal_hexstr(ctx, a->addr, a->addrlen);
	ret = tal_fmt(ctx, "Unknown type %u %s:%u", a->type, hex, a->port);
	tal_free(hex);
	return ret;
}
REGISTER_TYPE_TO_STRING(wireaddr, fmt_wireaddr);

/* Valid forms:
 *
 * [anything]:<number>
 * anything-without-colons-or-left-brace:<number>
 * anything-without-colons
 * string-with-multiple-colons
 *
 * Returns false if it wasn't one of these forms.  If it returns true,
 * it only overwrites *port if it was specified by <number> above.
 */
static bool separate_address_and_port(tal_t *ctx, const char *arg,
				      char **addr, u16 *port)
{
	char *portcolon;

	if (strstarts(arg, "[")) {
		char *end = strchr(arg, ']');
		if (!end)
			return false;
		/* Copy inside [] */
		*addr = tal_strndup(ctx, arg + 1, end - arg - 1);
		portcolon = strchr(end+1, ':');
	} else {
		portcolon = strchr(arg, ':');
		if (portcolon) {
			/* Disregard if there's more than one : or if it's at
			   the start or end */
			if (portcolon != strrchr(arg, ':')
			    || portcolon == arg
			    || portcolon[1] == '\0')
				portcolon = NULL;
		}
		if (portcolon)
			*addr = tal_strndup(ctx, arg, portcolon - arg);
		else
			*addr = tal_strdup(ctx, arg);
	}

	if (portcolon) {
		char *endp;
		*port = strtol(portcolon + 1, &endp, 10);
		return *port != 0 && *endp == '\0';
	}
	return true;
}



bool parse_wireaddr(const char *arg, struct wireaddr *addr, u16 defport,
		  const char **err_msg)
{
	struct in6_addr v6;
	struct in_addr v4;
	struct sockaddr_in6 *sa6;
	struct sockaddr_in *sa4;
	struct addrinfo *addrinfo;
	struct addrinfo hints;
	int gai_err;

	u8 tor_dec_bytes[TOR_V3_ADDRLEN];
	u16 port;
	char *ip;

	bool res;
	tal_t *tmpctx = tal_tmpctx(NULL);


	res = false;
	port = defport;
	if (err_msg)
		*err_msg = NULL;

	if (!separate_address_and_port(tmpctx, arg, &ip, &port))
		goto finish;


	if (streq(ip, "localhost"))
		ip = "127.0.0.1";
	else if (streq(ip, "ip6-localhost"))
		ip = "::1";

	memset(&addr->addr, 0, sizeof(addr->addr));

	if (inet_pton(AF_INET, ip, &v4) == 1) {
		addr->type = ADDR_TYPE_IPV4;
		addr->addrlen = 4;
		addr->port = port;
		memcpy(&addr->addr, &v4, addr->addrlen);
		res = true;
	} else if (inet_pton(AF_INET6, ip, &v6) == 1) {
		addr->type = ADDR_TYPE_IPV6;
		addr->addrlen = 16;
		addr->port = port;
		memcpy(&addr->addr, &v6, addr->addrlen);
		res = true;
	}

	if (strends(ip, ".onion"))
	{
		if (strlen(ip)<25) {//FIXME bool is_V2_or_V3_TOR(addr);
		//odpzvneidqdf5hdq.onion
		addr->type =   ADDR_TYPE_TOR_V2;
		addr->addrlen = TOR_V2_ADDRLEN;
		addr->port = port;
		b32_decode((u8 *)tor_dec_bytes,(u8 *)ip,2);
		memcpy(&addr->addr,tor_dec_bytes, addr->addrlen);
		res = true;
			}
		else {
		//4ruvswpqec5i2gogopxl4vm5bruzknbvbylov2awbo4rxiq4cimdldad.onion
		addr->type = ADDR_TYPE_TOR_V3;
		addr->addrlen = TOR_V3_ADDRLEN;
		addr->port = port;
		b32_decode((u8 *)tor_dec_bytes,(u8 *)ip,3);
		memcpy(&addr->addr,tor_dec_bytes, addr->addrlen);
		res = true;
			}

		goto finish;
	};

	/* Resolve with getaddrinfo */
	if (!res) {
		memset(&hints, 0, sizeof(hints));

		hints.ai_family = AF_UNSPEC;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_protocol = 0;
		hints.ai_flags = AI_ADDRCONFIG;
		gai_err = getaddrinfo(ip, tal_fmt(tmpctx, "%d", port),
							&hints, &addrinfo);
		if (gai_err != 0) {
			if (err_msg)
				*err_msg = gai_strerror(gai_err);
			goto finish;
		}
		/* Use only the first found address */
		if (addrinfo->ai_family == AF_INET) {
			addr->type = ADDR_TYPE_IPV4;
			addr->addrlen = 4;
			addr->port = port;
			sa4 = (struct sockaddr_in *) addrinfo->ai_addr;
			memcpy(&addr->addr, &sa4->sin_addr, addr->addrlen);
			res = true;
		} else if (addrinfo->ai_family == AF_INET6) {
			addr->type = ADDR_TYPE_IPV6;
			addr->addrlen = 16;
			addr->port = port;
			sa6 = (struct sockaddr_in6 *) addrinfo->ai_addr;
			memcpy(&addr->addr, &sa6->sin6_addr, addr->addrlen);
			res = true;
		}
		/* Clean up */
		freeaddrinfo(addrinfo);
	}

finish:
	if (!res && err_msg && !*err_msg)
		*err_msg = "Error parsing hostname";
	tal_free(tmpctx);
	return res;

}

bool parse_tor_wireaddr(const char *arg,u8 *ip_ld,u16 *port_ld)
{
	u16 port;
	char *ip;

	bool res;
	tal_t *tmpctx = tal_tmpctx(NULL);
	res = false;
	ip = tal_strdup(tmpctx,"127.0.0.1");
	port = 9050;
	if (!separate_address_and_port(tmpctx, arg,&ip, &port)) {
		tal_free(tmpctx);
		return false;
	}
	else
	{
		assert(strlen(ip) < 16);
		memcpy(ip_ld,ip,strlen(ip)+1);
		(*port_ld) = port;
		res= true;
	}
	tal_free(tmpctx);
	return res;
}


#define MAX_TOR_COOKIE_LEN 32
#define MAX_TOR_SERVICE_READBUFFER_LEN 255
#define MAX_TOR_ONION_V2_ADDR_LEN 16
#define MAX_TOR_ONION_V3_ADDR_LEN 56

struct tor_service_reaching {
	struct lightningd *ld;
	u8 buffer[MAX_TOR_SERVICE_READBUFFER_LEN];
	char *cookie[MAX_TOR_COOKIE_LEN];
	u8 *p;
	bool noauth;
	size_t hlen;
};


static struct io_plan *io_tor_connect_create_onion_finished(struct io_conn *conn, struct tor_service_reaching *reach)
{
    u8 tor_dec_bytes[TOR_V3_ADDRLEN];

	if(reach->hlen ==  MAX_TOR_ONION_V2_ADDR_LEN ) {
	size_t n = tal_count(reach->ld->wireaddrs);
	tal_resize(&reach->ld->wireaddrs, n+1);
	reach->ld->wireaddrs[n].type = ADDR_TYPE_TOR_V2;
	reach->ld->wireaddrs[n].addrlen = TOR_V2_ADDRLEN;
	reach->ld->wireaddrs[n].port = reach->ld->portnum;
	b32_decode((u8 *)tor_dec_bytes,(u8 *)reach->buffer,2);
	memcpy(&reach->ld->wireaddrs[n].addr,tor_dec_bytes, reach->ld->wireaddrs[n].addrlen);

	}
	/*on the other hand we can stay connected until ln finish to keep onion alive and then vanish*/
	//because when we run with Detach flag as we now do every start of LN creates a new addr while the old
	//stays valid until reboot this might not be desired so we can also drop Detach and use the
	//read_partial to keep it open until LN drops
	//FIXME: SAIBATO
	//return io_read_partial(conn, reach->p, 1 ,&reach->hlen, io_tor_connect_create_onion_finished, reach);
	return io_close(conn);
}


static struct io_plan *io_tor_connect_after_create_onion(struct io_conn *conn, struct tor_service_reaching *reach)
{

	reach->p = reach->p+reach->hlen;

 if (!strstr((char *)reach->buffer,"ServiceID=")) {
	if (reach->hlen == 0) return io_close(conn);
	return io_read_partial(conn, reach->p, 1 ,&reach->hlen, io_tor_connect_after_create_onion, reach);
	}
	else
	{
	memset(reach->buffer,0,sizeof(reach->buffer));
	return io_read_partial(conn, reach->buffer, MAX_TOR_ONION_V2_ADDR_LEN ,&reach->hlen, io_tor_connect_create_onion_finished, reach);
	};

}


//V3 tor after 3.3.3.aplha FIXME: TODO SAIBATO
//sprintf((char *)reach->buffer,"ADD_ONION NEW:ED25519-V3 Port=9735,127.0.0.1:9735\r\n");

static struct io_plan *io_tor_connect_make_onion(struct io_conn *conn, struct tor_service_reaching *reach)
{
	if (strstr((char *)reach->buffer,"250 OK") == NULL) return io_close(conn);
	sprintf((char *)reach->buffer,"ADD_ONION NEW:RSA1024 Port=%d,127.0.0.1:%d Flags=DiscardPK,Detach\r\n",
			reach->ld->portnum,reach->ld->portnum);

			reach->hlen = strlen((char *)reach->buffer);
			reach->p = reach->buffer;
	return io_write(conn, reach->buffer,  reach->hlen, io_tor_connect_after_create_onion, reach);

}


static struct io_plan *io_tor_connect_after_authenticate(struct io_conn *conn, struct tor_service_reaching *reach)
{
return io_read(conn, reach->buffer,7,io_tor_connect_make_onion, reach);
}


static struct io_plan *io_tor_connect_authenticate(struct io_conn *conn, struct tor_service_reaching *reach)
{
		sprintf((char *)reach->buffer,"AUTHENTICATE %s\r\n",(char *)reach->cookie);

		if (reach->noauth)
				sprintf((char *)reach->buffer,"AUTHENTICATE\r\n");

		reach->hlen = strlen((char *)reach->buffer);

return io_write(conn, reach->buffer,  reach->hlen, io_tor_connect_after_authenticate, reach);

}



static struct io_plan *io_tor_connect_after_answer_pi(struct io_conn *conn, struct tor_service_reaching *reach)
{
	char *p,*p2;
	int i;
	static u8 buf[MAX_TOR_COOKIE_LEN];

	if((p = strstr((char *)reach->buffer,"COOKIEFILE=")))
	{
	assert(strlen(p) > 12);
	p2 = strstr((char *)(p+12),"\"");
	assert(p2 != NULL);
	i=strlen(p2);
	*(char *)(p +(strlen(p)-i)) = 0;
	int fd = open((char *)(p+12), O_RDONLY );

	if (fd < 0)
	return io_close(conn);
	if (!read(fd, &buf, sizeof(buf)))
		 {
			 close(fd);
			 return io_close(conn);
		 } else  close(fd);

	hex_encode(buf, 32, (char *)reach->cookie, 80);
	reach->noauth=false;
	}
	else
	if (strstr((char *)reach->buffer,"NULL")) reach->noauth=true;
	else // FIXME: TODO set passwd in options if we not sudo
	if (strstr((char *)reach->buffer,"COOKIE")) return io_close(conn);

	return io_tor_connect_authenticate(conn,reach);

}

static struct io_plan *io_tor_connect_after_protocolinfo(struct io_conn *conn, struct tor_service_reaching *reach)
{

	memset(reach->buffer,0,MAX_TOR_SERVICE_READBUFFER_LEN);
	return io_read_partial(conn, reach->buffer, MAX_TOR_SERVICE_READBUFFER_LEN ,&reach->hlen, &io_tor_connect_after_answer_pi, reach);
}

static struct io_plan *io_tor_connect_after_resp_to_connect(struct io_conn *conn, struct tor_service_reaching *reach)
{

			 sprintf((char *)reach->buffer,"PROTOCOLINFO\r\n");
		  	 reach->hlen = strlen((char *)reach->buffer);

	return io_write(conn, reach->buffer,  reach->hlen, io_tor_connect_after_protocolinfo, reach);
}


static struct io_plan *tor_connect_finish(struct io_conn *conn,
		struct tor_service_reaching *reach)
{
	return io_tor_connect_after_resp_to_connect(conn,reach);
};

static struct io_plan *tor_conn_init(struct io_conn *conn, struct lightningd *ld)
 {
	static struct tor_service_reaching reach;
	static struct addrinfo *ai_tor;
	reach.ld = ld;
	getaddrinfo(tal_strdup(NULL,"127.0.0.1"),tal_strdup(NULL,"9051"), NULL,&ai_tor);

	return io_connect(conn, ai_tor, &tor_connect_finish, &reach);
 }

bool create_tor_hidden_service_conn(struct lightningd *ld)
  {
 	int fd;
 	struct io_conn *conn;

	fd = socket(AF_INET, SOCK_STREAM, 0);
 	conn = io_new_conn(NULL, fd, &tor_conn_init,ld);
 	if (!conn)
 		exit(1);

 	return true;
  }

