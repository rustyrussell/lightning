#! /bin/sh -e

PROPAGATE=0
CHECK_ONLY=0
KEEP_GOING=0
while [ $# -gt 0 ]; do
    case "$1" in
	-p) PROPAGATE=1; shift ;;
	-n) CHECK_ONLY=1; shift ;;
	-k) KEEP_GOING=1; shift ;;
	*) break ;;
    esac
done

if [ $# -lt 1 ]; then
    echo "Usage: $0 [-p] [-n] <filepath>...; removes #includes one at a time and checks compile" >&2
    echo "  -p: when removing an include from a header, add it to all .c/.h files which directly include that header" >&2
    echo "  -n: only check files compile standalone (for CI); don't reduce" >&2
    echo "  -k: don't stop at first failure" >&2
    exit 1
fi

# Insert an include line into a file in sorted position among #include <> lines.
add_include() {
    inc="$1" cfile="$2"
    grep -qF "$inc" "$cfile" && return
    awk -v new="$inc" '
	{ lines[NR] = $0 }
	/^#include /{ last_inc = NR }
	END {
	    for (i = 1; i <= NR; i++) {
		if (lines[i] ~ /^#include </ && !inserted && lines[i] > new) {
		    print new; inserted = 1
		}
		print lines[i]
		if (!inserted && i == last_inc) {
		    print new; inserted = 1
		}
	    }
	    if (!inserted) print new
	}
    ' "$cfile" > "$cfile.tmp" && mv "$cfile.tmp" "$cfile"
}

CCMD=$(make show-flags | sed -n 's/CC://p')
RET=0
for file; do
    # We have a rule (and a check!) that a .c includes its own .h directly.
    OWN_HDR='<'$(echo "$file" | sed -n 's/\.c$/.h/p')'>'
    i=1
    echo "$file":
    # First check: does this file compile standalone?
    cp "$file" "$file".c
    if ! $CCMD /tmp/out.$$.o "$file".c 2>/tmp/err.$$.txt; then
	echo " does not compile standalone:"
	cat /tmp/err.$$.txt >&2
	rm -f "$file".c /tmp/out.$$.o /tmp/err.$$.txt
	[ $KEEP_GOING -eq 1 ] && RET=1 && continue
	exit 1
    fi
    rm -f "$file".c /tmp/out.$$.o /tmp/err.$$.txt
    [ $CHECK_ONLY -eq 1 ] && echo " ok" && continue
    while true; do
	# Don't eliminate config.h includes, wiregen includes, or annotated includes (non-whitespace after >).
	LINE="$(grep '^#include <' "$file" | grep -v '[<"]config.h[">]' | grep -F -v "$OWN_HDR" | grep -v '_wiregen\.[ch]>' | grep -v '>.*[^[:space:]]' | tail -n +$i | head -n1)"
	[ -n "$LINE" ] || break
	# Make sure even headers end in .c
	grep -F -v "$LINE" "$file" > "$file".c

	if $CCMD /tmp/out.$$.o "$file".c 2>/dev/null; then
	    printf "%s" "-$LINE"
	    mv "$file".c "$file"
	    # If propagating and this is a header, add removed include to dependent .c files.
	    if [ $PROPAGATE -eq 1 ] && echo "$file" | grep -q '\.h$'; then
		HDR=$(echo "$file" | sed 's|^\./||')
		grep -rl --include="*.c" --include="*.h" "#include <$HDR>" . 2>/dev/null | while read -r cfile; do
		    add_include "$LINE" "$cfile"
		done
	    fi
	else
	    # shellcheck disable=SC2039,SC3037
	    printf "."
	    rm -f "$file".c
	    i=$((i + 1))
	fi
	rm -f /tmp/out.$$.o
    done
    echo
done
exit $RET
