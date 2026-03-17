#ifndef ZTFS_UTILS_H
#define ZTFS_UTILS_H

#include "ztfs.h"

/**
 * @brief Split a `full_path` string into two compnents, the `parent_path` and `entry_name`.
 * 
 * i.e. Full path `"/home/docs"`, Parent path `"/home"`, Entry name `"docs"`
 * 
 * @param full_path Full path input
 * @param parent_path Buffer to hold parent path
 * @param entry_name Buffer to hold entry name
 * 
 * @return 0 if successful, or -1 on failure.
 */
int split_path_from_entry(char *full_path, char *parent_path, char *entry_name);

#endif