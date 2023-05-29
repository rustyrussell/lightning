#include <plugins/renepay/debug.h>


void debug_knowledge(
		struct chan_extra_map* chan_extra_map,
		const char*fname)
{
	FILE *f = fopen(fname,"a");
	fprintf(f,"Knowledge:\n");
	struct chan_extra_map_iter it;
	for(struct chan_extra *ch = chan_extra_map_first(chan_extra_map,&it);
	    ch;
	    ch=chan_extra_map_next(chan_extra_map,&it))
	{
		const char *scid_str = 
			type_to_string(tmpctx,struct short_channel_id,&ch->scid);
		for(int dir=0;dir<2;++dir)
		{
			fprintf(f,"%s[%d]:(%s,%s)\n",scid_str,dir,
				type_to_string(tmpctx,struct amount_msat,&ch->half[dir].known_min),
				type_to_string(tmpctx,struct amount_msat,&ch->half[dir].known_max));
		}
	}
	fclose(f);
}

void debug_payflows(struct pay_flow **flows, const char* fname)
{
	FILE *f = fopen(fname,"a");
	fprintf(f,"%s\n",fmt_payflows(tmpctx,flows));
	fclose(f);
}

void debug_exec_branch(int lineno, const char* fun, const char* fname)
{
	FILE *f = fopen(fname,"a");
	fprintf(f,"executing line: %d (%s)\n",lineno,fun);
	fclose(f);
}

void debug_outreq(const struct out_req *req, const char*fname)
{
	FILE *f = fopen(fname,"a");
	size_t len;
	const char * str =  json_out_contents(req->js->jout,&len);
	fprintf(f,"%s",str);
	if (req->errcb)
		fprintf(f,"}");
	fprintf(f,"}\n");
	fclose(f);
}

void debug_call(const char* fun, const char* fname)
{
	pthread_t tid = pthread_self();
	FILE *f = fopen(fname,"a");
	fprintf(f,"calling function: %s (pthread_t %ld)\n",fun,tid);
	fclose(f);
}

void debug_reply(const char* buf,const jsmntok_t *toks, const char*fname)
{
	FILE *f = fopen(fname,"a");
	fprintf(f,"%.*s\n\n",
		   json_tok_full_len(toks),
		   json_tok_full(buf, toks));
	fclose(f);
}

void debug(const char* fname, const char *fmt, ...)
{
	FILE *f = fopen(fname,"a");
	
	va_list args;
	va_start(args, fmt);
	
	vfprintf(f,fmt,args);
	
	va_end(args);
	fclose(f);
}

