WORKSPACE:=../
PREFIX:=
LIBNAME:=armshared-arm
SOURCES:=$(wildcard libsrc/*.[cS])
OBJECTS:=$(SOURCES:.c=.o)
CFLAGS += -I${WORKSPACE}/libbsp/include/ -I$(WORKSPACE)/tiles -Iinclude/ -O2

all:

# we don't precompile this library
# - for our TOOLS we could do it for the *system.[ch] but it's just as easy to just refer to the sources
# - for the user ARM applications we can only compile together with the vep_memory_map.h 

#all: lib/libarmshared-arm.a(${OBJECTS})
#lib/libarmshared-arm.a(${OBJECTS}): lib 

lib:
	mkdir -p $@

clean:
	@rm -f lib/libarmshared-arm.a