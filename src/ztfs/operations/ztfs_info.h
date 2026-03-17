#ifndef ZTFS_INFO_H
#define ZTFS_INFO_H

#include <stdint.h>
#include "../ztfs.h"

int ztfs_print_info(char* name);
int ztfs_print_entry(char *name, char *path);
int ztfs_print_tree(char* name);

#endif