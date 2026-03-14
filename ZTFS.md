# ZTFS Documentation
This document lays out the on disk structures representing any data on disk.

# Blueprint
The most vital structure, holding data that is required to interface with the file system. This is the ZTFS equivalent of the superblock in EXT. `baddr` is a `uint32_t` typedef, denoting that that number is a block number. All `baddr` are absolute on disk. 

Almost all values are constants, and only a few are strictly required to interface with the file system. Some of these values are solely for your convenience, as to not require constant calculation.

| Size  | Offset| Name                   | Notes                                 | Const |
| ----- | ----- | ---------------------- | ------------------------------------- | ----- |
| u64   | 0     | signature              | Equal to 5346545A on valid ZTFS fs.   | Yes   |
| u16   | 8     | version_major          | Major version of fs                   | Yes   |
| u16   | 10    | version_minor          | Minor version of fs                   | Yes   |
| u32   | 12    | block_size             | Size of a block in bytes              | Yes   |
| u64   | 16    | size_bytes             | Size of the file system in bytes      | Yes   |
| u64   | 24    | free_blocks            | Number of free blocks available       | No    |
| u32   | 32    | baddr_root_entry       | Block address of root directory       | Yes   |
| u32   | 36    | block_group_table_size | Blocks in use by block group table    | Yes   |
| u32   | 40    | baddr_bitmap_start     | Start address of preallocated bitmaps | Yes   |
| u32   | 44    | block_group_size       | Size of a block group in blocks       | Yes   |
| u32   | 48    | baddr_first_block_group| Block address of first block group    | Yes   |
| u32   | 52    | block_group_count      | Number of block groups in file system | Yes   |

### Useful things to note:
- The block size MUST be a multiple of 1024, and not lower. 4096 is typical, and ideal. Other values may have unintended consequences, as they are not error checked on creation.

# Block Group Descriptor
Similar to EXT, this structure represents the state of a block group. Starting at block address 2, an array is created with items for each block group in the file system.

| Size  | Offset| Name                | Notes                                       |
| ----- | ----- | ------------------- | ------------------------------------------- |
| u32   | 0     | baddr_block_bitmap  | Block address for the used block bitmap     |
| u32   | 4     | baddr_first_entry   | Block address for the first entry           |
| u32   | 8     | free_blocks         | Number of free blocks in this block group   |
| u16   | 12    | num_entries         | Number of entries this block group has      |
| u16   | 14    | reserved            |                                             |

# Entry
An entry represents a file, directory, or any other type of file. These are fixed in size, and are treated like an array.

### Note!
- It is not guaranteed that entries are sequential, nor that they may be in the same block. To check for a valid entry, `entry_type` must not equal 0.

| Size  | Offset| Name                  | Notes                                             |
| ----- | ----- | --------------------- | ------------------------------------------------- |
| u8[64]| 0     | name                  | Name of the entry, null-terminated. Max 63 chars. |
| u64   | 64    | size                  | Size in bytes if file, entry count for directory  |
| u32   | 72    | size_blocks           | Number of blocks the data is using                |
| u8    | 76    | entry_type            | Type of entry (see Entry Type)                    |
| u8    | 77    | permissions           | Permissions of the entry (see Permissions)        |
| u32   | 78    | baddr_indirect_block  | Block address of indirect block                   |

### Indirect Block
Unlike EXT, each file ONLY has one indirect block. This does limit the size of a file to `((block_size / 4) * block_size) * block_size` (~17GB with 4096 block size), but traversing data is much easier. This block holds an array of `baddr[block_size / sizeof(baddr)]`, which are block addresses to the data of the entry.

### Entry Type
| Name              | Notes                         |
| ----------------- | ----------------------------- |
| NONE              | Nonexistent Entry             |
| ENTRY_DIRECTORY   | Directory                     |
| ENTRY_FILE        | General File                  |

### Permissions
Super meaning administrator. This file system does not protect against multiple users in version 1.0. For convenience, the mixed values are included in `ztfs.h`.
| Name          | Notes                                             |
| ------------- | ------------------------------------------------- |
| SUPER_READ    | Super read                                        |
| SUPER_WRITE   | Super write                                       |
| SUPER_RW      | Super read and write                              |
| SUPER_EXECUTE | Super execute                                     |
| SUPER_RWX     | Super read, write, and execute                    |
| USER_READ     | User read                                         |
| ALL_READ      | Super and user read                               |
| USER_WRITE    | User write                                        |
| ALL_WRITE     | Super and user write                              |
| USER_RW       | User read and write                               |
| USER_EXECUTE  | User execute                                      |
| ALL_EXECUTE   | Super and user execute                            |
| USER_RWX      | User read, write, and execute                     |
| ALL_RW        | Super and user read and write                     |
| ALL_RWX       | Super and user read, write, and execute           |

# Disk Layout

### Boot Sector
512-byte area for traditional boot sector.

### Blueprint
Always at offset `512` from the start of the disk.

### Block Group Descriptor Table
Starting at block 2, regardless of block size. This is an array of `ztfs_block_group_descriptor`, where the index is the block group.

### BG Bitmaps
Preallocated on disk on the next block after the BGDT, spanning as many blocks as there are block groups. The first bitmap represents BG0, and so on.

### Data Start
The first block in data start. Represented in the `blueprint` as `baddr_first_block_group`, it typically holds the root directory entry. This entry is very important, as it is from where all other entries are derived from. All subsequent blocks after this block is free for storage, until the end of the disk.
