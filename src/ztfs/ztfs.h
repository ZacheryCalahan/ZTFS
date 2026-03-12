#ifndef ZTFS_H
#define ZTFS_H

#include <stdint.h>

#define ZTFS_VERSION_MAJOR 1
#define ZTFS_VERSION_MINOR 0

#define ZTFS_BGDT_START_BADDR 2 // Block address where BGDT starts. This does not change.
#define ZTFS_MAGIC_SIGNATURE 0x5346545AUL

enum ENTRY_TYPE {
    NONE = 0,
    ENTRY_DIRECTORY = 1,
    ENTRY_FILE = 2,
};

enum PERMISSIONS {
    SUPER_READ = 1, // Super is locked to the kernel
    SUPER_WRITE = 2,
    SUPER_EXECUTE = 4,
    USER_READ = 8,  // User allows users to access this file
    USER_WRITE = 16,
    USER_EXECUTE = 32,
};

struct ztfs_blueprint {
    uint64_t signature;                 // "ZTFS" required to mark this is a ZTFS filesystem
    uint16_t version_major;             // Major version number
    uint16_t version_minor;             // Minor version number
    uint32_t block_size;                // Size of a block in bytes
    uint64_t size_bytes;                // Size of the filesystem in bytes
    uint64_t free_blocks;               // Number of free blocks available for use
    uint32_t baddr_root_entry;          // Block address of the root directory entry
    uint32_t block_group_table_size;    // Blocks in use by the block group table
    uint32_t baddr_bitmap_start;         // Start address of the preallocated bitmaps, for convience
};

struct ztfs_block_group_descriptor {
    uint32_t baddr_block_bitmap;        // Block address for the used block bitmap
    uint32_t baddr_first_entry;         // Block address for the first entry, for convience.
    uint32_t free_blocks;               // Number of free blocks available for use in this block group
    uint16_t num_entries;               // Number of entries this block group has, for convience
    uint16_t reserved;                  // Padding for version 1.0
};

struct ztfs_entry {
    char name[64];                      // Name of the entry, \0 terminated.
    uint64_t size;                      // Number of bytes the data is using if file, number of entries if dir.
    uint32_t size_blocks;               // Number of blocks the data is using
    uint8_t entry_type;                 // Type of entry, ie. directory, file, etc.
    uint8_t permissions;                // Permissions of the entry
    uint32_t baddr_indirect_block;      // Block address of the block holding pointers to data blocks
};

#endif