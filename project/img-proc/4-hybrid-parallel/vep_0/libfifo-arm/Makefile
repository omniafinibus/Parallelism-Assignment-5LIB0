WORKSPACE:=../
PREFIX:=
LIBNAME:=fifo-arm
SOURCES:=$(wildcard libsrc/*.[cS])
OBJECTS:=$(SOURCES:.c=.o)
CFLAGS += -DARM -I${WORKSPACE}/libbsp/include/ -I$(WORKSPACE)/tiles -Iinclude/ -O2
ARFLAGS := $(ARFLAGS)U

all: 
clean:
