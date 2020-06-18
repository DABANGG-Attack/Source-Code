#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>


#ifndef UTIL_H_
#define UTIL_H_

#define ADDR_PTR void*
#define CYCLES uint64_t

#define CACHE_BLOCK_SIZE	64
#define MAX_BUFFER_LEN	1024

typedef struct map_handle_s {
  int fd;
  size_t range;
  ADDR_PTR mapping;
} map_handle_t;

CYCLES measure_one_block_access_time(ADDR_PTR addr);
CYCLES rdtscp(void);
void clflush(ADDR_PTR addr);
void maccess(ADDR_PTR addr);
void* map_file(const char* filename, map_handle_t** handle);
void unmap_file(map_handle_t* handle);
char *string_to_binary(char *s);
char *binary_to_string(char *data);
#endif

