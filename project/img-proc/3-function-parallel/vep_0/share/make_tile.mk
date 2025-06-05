###
# Copyright Verintec
# Makefile for compiling code on a tile.
#
# Use dependency files for dependency tracking.
##

# Include the tile specific flags.
include $(SYSVEP)/tiles/tile${TILE_ID}_mb.mk

PREFIX:=/opt/riscv/bin/riscv32-unknown-elf

# Name used by xilinx for this specific tile.
TILE_NAME:=tile${TILE_ID}_mb

TARGET := out.elf
LINKER_SCRIPT := lscript.ld
GENERATED_MEMORY_MAP = $(SHAREDMEM)/vep_memory_map.h $(SHAREDMEM)/vep_memory_map.c

SHAREDMEMC = $(wildcard $(SHAREDMEM)/*.c)
SHAREDMEMO = $(SHAREDMEMC:.c=.o)
SHAREDMEMO := $(SHAREDMEMO:$(SHAREDMEM)/=)

CFLAGS += ${COMPILER_FLAGS}
CFLAGS += $(USER_CFLAGS)
CFLAGS += $(foreach lib,$(USER_LIBS),-I$(WORKSPACE)/lib$(lib)/include)
CFLAGS += -I$(SHAREDMEM) -I../shared
CFLAGS += -I${SYSVEP}/tiles
CFLAGS += -DVEP_ID=${VEP_ID} -DTILE_ID=${TILE_ID} -DPARTITION_ID=${PARTITION_ID}

# Library
# the order of the SYS_LIBS is important, do not change
SYS_LIBS += bsp channel fifo-riscv mutex-riscv
CFLAGS += $(foreach lib,$(SYS_LIBS),-I${SYSVEP}/lib$(lib)/include/)
SYS_LIBS_TARGETS := $(foreach lib,$(SYS_LIBS),${SYSVEP}/lib$(lib)/lib/lib$(lib)_$(TILE_NAME).a)
USER_LIBS_DIRS := $(foreach lib,$(USER_LIBS),../lib$(lib))
USER_LIBS_TARGETS := $(foreach lib,$(USER_LIBS),../lib$(lib)/lib/lib$(lib)_$(TILE_NAME).a)

##
# Default target
##
all: $(TARGET)

objectdump objdump $(TARGET).objectdump: $(TARGET)
	${PREFIX}-objdump -DS $(TARGET) > $(TARGET).objectdump

$(SYS_LIBS_TARGETS):
	make -C $(SYSVEP)

$(USER_LIBS_TARGETS):
	make -C $(USER_LIBS_DIRS)

$(TOOLS)/generate-json:
	make -C $(TOOLS)

$(GENERATED_MEMORY_MAP): $(TOOLS)/generate-json $(WORKSPACE)/vep-config.txt
	@rm -f vep-config.tmp
	@# add vep id to vep-config.txt
	@sed "s/^/vep ${VEP_ID} /" $(WORKSPACE)/vep-config.txt > vep-config.tmp
	@$(TOOLS)/generate-json vep-config.tmp -shmem $(VEP_ID)
	@rm -f vep-config.tmp

$(LINKER_SCRIPT): $(TOOLS)/generate-json $(WORKSPACE)/vep-config.txt
	@rm -f vep-config.tmp
	@# add vep id to vep-config.txt
	@sed "s/^/vep ${VEP_ID} /" $(WORKSPACE)/vep-config.txt > vep-config.tmp
	@# remove empty lscript.ld if generate-jason failed
	@$(TOOLS)/generate-json vep-config.tmp -ld $(VEP_ID) $(TILE_ID) $(PARTITION_ID) > $(LINKER_SCRIPT) || rm -f $(LINKER_SCRIPT) vep-config.tmp
	@[ -f $(LINKER_SCRIPT) ]
	@rm -f vep-config.tmp

# TODO filter on C/C++ sources separate.
OBJECTS 	 := ${SOURCES:.c=.o}
OBJECTS 	 := ${OBJECTS:.s=.o}
OBJECTS 	 := ${OBJECTS:.S=.o}
DEPENDENCIES := ${SOURCES:.c=.d}

MB_GCC := ${PREFIX}-gcc
MB_OBJCOPY := ${PREFIX}-objcopy

$(OBJECTS): $(SYS_LIBS_TARGETS) $(WORKSPACE)/vep-config.txt ${LINKER_SCRIPT} $(GENERATED_MEMORY_MAP) $(USER_LIBS_TARGETS)

# Create hex file
${TARGET}: ${OBJECTS}
	${MB_GCC} ${CFLAGS} -Wl,-T -Wl,${LINKER_SCRIPT} -o $@ ${OBJECTS} $(SHAREDMEMC) \
        $(foreach lib,$(USER_LIBS),-L$(WORKSPACE)/lib$(lib)/lib/ -l$(lib)_$(TILE_NAME)) $(USER_LINK_FLAGS)\
        $(foreach lib,$(SYS_LIBS),-L$(SYSVEP)/lib$(lib)/lib/ -l$(lib)_$(TILE_NAME))
	${MB_OBJCOPY} --remove-section='.publicmem' --remove-section='.mem*' -g -Obinary ${TARGET} out.hex

%.d %.o: %.c
	${MB_GCC} ${CFLAGS} -MMD -MP $< -c -o ${^:.c=.o}

ifeq (clean,$(MAKECMDGOALS))
else
ifeq (realclean,$(MAKECMDGOALS))
else
-include ${DEPENDENCIES}
endif
endif

realclean clean:
	-rm -f $(TARGET).objectdump ${LINKER_SCRIPT} $(GENERATED_MEMORY_MAP) out.hex ${OBJECTS} ${DEPENDENCIES} $(SHAREDMEMC:.c=.d) ${TARGET} $(SHAREDMEMO) dynload-clear.cmd ../shared_memories/dynload-clear.cmd

.PHONY: clean realclean all objdump objectdump
