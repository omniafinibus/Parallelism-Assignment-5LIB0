WORKSPACE:=..
SYSWORKSPACE:=$(WORKSPACE)/..
TOOLS:=$(SYSWORKSPACE)/tools
SYSVEP:=$(SYSWORKSPACE)/vep_0
VEP_ID := $(shell pwd | sed -e 's/.*\/vep_\([0-9][0-9]*\)\/.*/\1/g' )

SOURCES := $(wildcard *.c)
HEADERS := $(wildcard *.h)

SHAREDMEM:=$(WORKSPACE)/shared_memories
SHAREDMEMC = $(wildcard $(SHAREDMEM)/*.c)
SHAREDMEMH = $(SHAREDMEM)/vep_shared_memory_regions.h
SHAREDMEMO = $(SHAREDMEMC:.c=.o)
GENERATED_MEMORY_MAP = $(SHAREDMEM)/vep_memory_map.h $(SHAREDMEM)/vep_memory_map.c

CFLAGS = -Wall -Wextra -O2 -g3 -I.
CFLAGS += ${COMPILER_FLAGS}
CFLAGS += -DARM -DVEP_ID=$(VEP_ID) $(USER_CFLAGS)
# include the RISC-V VEP memory map but do NOT link with the RISC-V object files in shared_memories
CFLAGS += -I$(SYSVEP)/libbsp/include -I$(SYSVEP)/libchannel/include -I$(SYSVEP)/libarmshared-arm/include -I$(SYSVEP)/libfifo-arm/include -I$(SYSVEP)/libmutex-arm/include -I$(WORKSPACE)/shared_memories -I$(WORKSPACE)/shared

# The order of the -l is important, do not change
LDFLAGS += -L$(SYSVEP)/libchannel/lib -L$(SYSVEP)/libfifo-arm/lib -L$(SYSVEP)/libmutex-arm/lib
LDFLAGS += -lchannel-arm -lfifo-arm -lmutex-arm
LDFLAGS += $(USER_LIBS)

##
# Default target
##
all: $(TARGET)

OBJECTS := ${SOURCES:.c=.o}
OBJECTS := ${OBJECTS:.s=.o}
OBJECTS := ${OBJECTS:.S=.o}
DEPENDENCIES := ${SOURCES:.c=.d}

GCC := gcc

%.d %.o: %.c
	${GCC} ${CFLAGS} $< -c -o ${^:.c=.o}

$(SYSVEP)/libchannel/lib/libchannel-arm.a:
	make -C $(SYSVEP)/libchannel

$(SYSVEP)/libfifo-arm/lib/libfifo-arm.a:
	make -C $(SYSVEP)/libfifo-arm

$(SYSVEP)/libfifo-arm/lib/libmutex-arm.a:
	make -C $(SYSVEP)/libmutex-arm

$(GENERATED_MEMORY_MAP): $(TOOLS)/generate-json $(WORKSPACE)/vep-config.txt
	@rm -f vep-config.tmp
	@# add vep id to vep-config.txt
	@sed "s/^/vep ${VEP_ID} /" $(WORKSPACE)/vep-config.txt > vep-config.tmp
	@$(TOOLS)/generate-json vep-config.tmp -shmem $(VEP_ID) || rm -f $(LINKER_SCRIPT) vep-config.tmp
	@rm -f vep-config.tmp

${TARGET}: $(SYSVEP)/libfifo-arm/lib/libfifo-arm.a $(GENERATED_MEMORY_MAP) $(wildcard $(SYSVEP)/libarmshared-arm/libsrc/*.c) $(wildcard $(SYSVEP)/libfifo-arm/libsrc/*.c) $(wildcard $(SYSVEP)/libmutex-arm/libsrc/*.c) $(SHAREDMEMH) $(SHAREDMEMC) $(OBJECTS)
	${GCC} ${CFLAGS} -o $(TARGET) $(OBJECTS) $(LDFLAGS) $(SYSVEP)/libbsp/libsrc/platform.c $(wildcard $(SYSVEP)/libarmshared-arm/libsrc/*.c) $(SYSVEP)/libfifo-arm/lib/libfifo-arm.a $(SYSVEP)/libmutex-arm/lib/libmutex-arm.a $(SHAREDMEMC)

realclean: clean

clean:
	-rm -f ${OBJECTS} ${DEPENDENCIES} ${TARGET}

.PHONY: clean realclean all 
