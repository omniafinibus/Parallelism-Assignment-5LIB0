#ifndef SHARED_DATATYPES_H
#define SHARED_DATATYPES_H

#include <stdint.h>

#define PRODUCER_IS_OWNER 0
#define CONSUMER_IS_OWNER 1
#define BUFFER_SIZE       5

typedef struct {
    int32_t data[BUFFER_SIZE];    // the data
    uint64_t timestamp; // indicates when the data was produced
} token_t;

#endif
