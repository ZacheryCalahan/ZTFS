#ifndef ZTFS_CREATE_H
#define ZTFS_CREATE_H

#include "ztfs.h"
#include <stdint.h>

int ztfs_create_image(char* name, uint64_t size, uint32_t block_size);

#endif