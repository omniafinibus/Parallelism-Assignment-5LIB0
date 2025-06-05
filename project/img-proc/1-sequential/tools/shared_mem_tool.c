#include <stdlib.h>
#include <errno.h>
#include <fcntl.h> //open
#include <unistd.h> //close
#include <sys/mman.h> //mmap
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "cheapout.h"
#include "arm_shared_memory_system.h"

CommandType command_type = CT_NONE;

uint32_t start = 0x0;
uint32_t length = 0x0;
uint32_t pattern = 0x0; 
FILE *input_fp = NULL;

static void print_help()
{
	printf("USAGE: shared_mem_tool <command> [options]\n");
	printf("\n");
	printf("Commands:\n");
	printf(" * clear <start> <length>\n");
	printf(" * write|pattern <start> <length> <pattern>\n");
	printf(" * increment <start> <length>\n");
	printf(" * file <start> <file>\n");
	printf(" * read <start> <length>\n");
	printf("notes:\n");
	printf(" * pattern is a word, i.e. 32-bit value\n");
	printf(" * length is in bytes\n");
	printf(" * start & length must be word aligned, i.e. multiples of 4\n");
	printf(" * ARM and RISC-V are both little-endian\n");
	printf(" * increment writes 0, 1, ... in successive words\n");
	printf(" * tip: view binary output with xxd\n");
}

// Setup to use normal malloc and free
void * ( *_dynmalloc )( size_t ) = malloc;
void   ( *_dynfree )( void* )    = free;


static int parse_commandline ( int argc, char **argv )
{
	if ( argc < 3 ) {
		print_help(argv[0]);
		return(EXIT_SUCCESS);
	}
	if ( strcmp(argv[0], "clear") == 0 ) {
		command_type = CT_CLEAR;
		if ( argc != 3 ) {
			fprintf(stderr, "The command *clear* requires 2 arguments\n");
			return(EXIT_FAILURE);
		}
		start  = strtoul(argv[1], NULL, 0);
		length = strtoul(argv[2], NULL, 0);
	} else if ( strcmp(argv[0], "read") == 0 ) {
		command_type = CT_READ;
		if ( argc != 3 ) {
			fprintf(stderr, "The command *read* requires 2 arguments\n");
			return(EXIT_FAILURE);
		}
		start  = strtoul(argv[1], NULL, 0);
		length = strtoul(argv[2], NULL, 0);
	} else if ( strcmp ( argv[0], "pattern" ) == 0 || strcmp ( argv[0], "write" ) == 0 ) {
		command_type = CT_PATTERN;
		if ( argc != 4 ) {
			fprintf(stderr, "The command *pattern* requires 3 arguments\n");
			return(EXIT_FAILURE);
		}
		start   = strtoul(argv[1], NULL, 0);
		length  = strtoul(argv[2], NULL, 0);
		pattern = strtoul(argv[3], NULL, 0);
	} else if ( strcmp ( argv[0], "increment" ) == 0 ) {
		command_type = CT_INCREMENT;
		if ( argc != 3 ) {
			fprintf(stderr, "The command *increment* requires 2 arguments\n");
			return(EXIT_FAILURE);
		}
		start  = strtoul(argv[1], NULL, 0);
		length = strtoul(argv[2], NULL, 0);
	} else if ( strcmp ( argv[0], "file" ) == 0 ) {
		command_type = CT_FILE;
		if ( argc != 3 ) {
			fprintf(stderr, "The command *file* requires 2 arguments\n");
			return(EXIT_FAILURE);
		}
		start  = strtoul(argv[1], NULL, 0);
		input_fp = fopen(argv[2], "r");
		if ( input_fp == NULL ) {
			fprintf(stderr, "Failed to open file: '%s' - '%s'\n", argv[2], strerror(errno));
			return (EXIT_FAILURE);
		}
		fseek(input_fp, 0L, SEEK_END);
		length = ftell(input_fp);
		rewind(input_fp);
	}
	else {
		print_help(argv[0]);
		return(EXIT_FAILURE);
	}

	if ( length == 0 ) {
		fprintf(stderr, "Length is zero, nothing to do.\n\n");
		if ( input_fp != NULL ) {
			fclose(input_fp);
		}
		return (EXIT_SUCCESS);
	}
	if ( command_type < CT_FILE && length%4 != 0 ) {
		fprintf(stderr, "The length needs to be multiple of 4 (one word).\n");
		if ( input_fp != NULL ) {
			fclose(input_fp);
		}
		return (EXIT_FAILURE);
	}
	return (EXIT_SUCCESS);
}

int main( int argc , char **argv ) 
{
	if (parse_commandline(argc-1, &argv[1]) == EXIT_FAILURE) exit(EXIT_FAILURE);
	if ( argc < 2 ) {
		print_help();
		return EXIT_FAILURE;
	}
	uint8_t values[length];
	if (command_type == CT_FILE) {
	  unsigned int i = 0;
	  int f;
	  while (i < length && (f = getc(input_fp)) != EOF) values[i++] = f;
	  fclose(input_fp);
	}
	int ret = shared_mem_rw(1 /*verbose*/, command_type, start, length, pattern, values);
	if (ret == EXIT_FAILURE) return ret;
	if (command_type == CT_READ) {
		for (unsigned int i=0; i < length; i++) putchar(values[i]);
	}
	return EXIT_SUCCESS;
}