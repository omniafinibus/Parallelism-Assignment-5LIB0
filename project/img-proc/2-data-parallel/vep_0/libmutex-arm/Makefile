WORKSPACE:=../
PREFIX:=
LIBNAME:=mutex-arm
SOURCES:=$(wildcard libsrc/*.[cS])
OBJECTS:=$(SOURCES:.c=.o)
CFLAGS += -DARM -I${WORKSPACE}/libbsp/include/ -I$(WORKSPACE)/tiles -Iinclude/ -O2
ARFLAGS := $(ARFLAGS)U

all: lib/libmutex-arm.a(${OBJECTS})
lib/libmutex-arm.a(${OBJECTS}): lib 

lib:
	mkdir -p $@

clean:
	@rm -f lib/libmutex-arm.a

