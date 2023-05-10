# Note that these actually #include everything they need, except ccan/ and bitcoin/.
# That allows for unit testing of statics, and special effects.
PLUGIN_RENEPAY_TEST_SRC := $(wildcard plugins/renepay/test/run-*.c)
PLUGIN_RENEPAY_TEST_OBJS := $(PLUGIN_RENEPAY_TEST_SRC:.c=.o)
PLUGIN_RENEPAY_TEST_PROGRAMS := $(PLUGIN_RENEPAY_TEST_OBJS:.o=)

ALL_C_SOURCES += $(PLUGIN_RENEPAY_TEST_SRC)
ALL_TEST_PROGRAMS += $(PLUGIN_RENEPAY_TEST_PROGRAMS)
$(PLUGIN_RENEPAY_TEST_OBJS): $(PLUGIN_RENEPAY_SRC)

PLUGIN_RENEPAY_TEST_COMMON_OBJS :=		\
	common/amount.o				\
	common/autodata.o			\
	common/fp16.o				\
	common/gossmap.o			\
	common/node_id.o			\
	common/pseudorand.o			\
	common/dijkstra.o			\
	common/setup.o				\
	common/type_to_string.o			\
	common/utils.o				\
	plugins/renepay/dijkstra.o

$(PLUGIN_RENEPAY_TEST_PROGRAMS): $(CCAN_OBJS) $(BITCOIN_OBJS) $(WIRE_OBJS) $(PLUGIN_RENEPAY_TEST_COMMON_OBJS)

check-units: $(PLUGIN_RENEPAY_TEST_PROGRAMS:%=unittest/%)
