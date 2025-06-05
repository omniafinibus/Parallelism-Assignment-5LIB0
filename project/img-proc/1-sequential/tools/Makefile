# tools depend on precompiled vep_0/libchannel, hence doesn't need a dependency

LDLIBS = -lchannel-arm -lm
LDFLAGS = -L. -L../vep_0/libchannel/lib
CFLAGS = -Wall -Wextra -pedantic -O0 -g3 -std=c99 -I. -I../vep_0/libbsp/include

LDLIBS_SHM =
LDFLAGS_SHM = -L.
CFLAGS_SHM = -Wall -Wextra -O0 -g3 -I. -I../vep_0/libbsp/include -I../vep_0/libarmshared-arm/include

SYSVEP = ../vep_0

TOOLS:=channel_init channel_dump channel_readall channel_cheap_bridge
# to be updated: channel_readline

all: ${TOOLS} generate-json shared_mem_tool

$(SYSVEP)/libarmshared-arm/lib/libarmshared-arm.a: 
	make -C $(SYSVEP)

shared_mem_tool: shared_mem_tool.c $(SYSVEP)/libbsp/libsrc/platform.c
	gcc $(CFLAGS_SHM) $(LDFLAGS_SHM) -o shared_mem_tool shared_mem_tool.c $(SYSVEP)/libbsp/libsrc/platform.c $(SYSVEP)/libarmshared-arm/libsrc/arm_shared_memory_system.c $(LDLIBS_SHM)

generate-json: generate-json.c $(SYSVEP)/libbsp/libsrc/platform.c
	@# libbsp is compiled for RISC-V, we only want to pick up some constants (arrays)
	gcc $(CFLAGS) $(LDFLAGS) -o generate-json generate-json.c $(SYSVEP)/libbsp/libsrc/platform.c $(LDLIBS)

clean:
	rm -f ${TOOLS} generate-json shared_mem_tool
	@# removed by succesful runs of rerun.sh
	rm -f vep-config.txt vep-config.cmd vep-config.json state.json

veryclean: clean

.PHONY: all clean veryclean vep_0
