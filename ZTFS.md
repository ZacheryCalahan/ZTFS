# ZTFS Documentation

## Documentation Assumptions
- **may** refers to non-required but encouraged implementation details. **must** or **only** refers to required implementation details.
- If not explicitly stated, implementation details may differ with the same results. Block allocation is an example, where the file system does not care from where a free block is allocated, simply that its block address is tied to its respective structure.

## Important Assumptions
- All values are required to be in little endian format.
- Major version changes are breaking changes, indicating a new version of the spec. These changes are not guaranteed to be backwards compatible. 
- Minor version changes are non-breaking changes, such as non-required additions to the spec.
- `baddr_t` is a `uint32_t` typedef, denoting that that number is a block number. All `baddr_t` **must** be absolute block addresses on disk.

> [!CAUTION]
> - ZTFS is inherently not a perfectly safe file system. There are no concrete copies of important data, no journaling, no fixes for partial writes. Though this may pose an an issue to real usage, this file system is intended to be used for education and simplifies away these contingencies. 

# Blueprint
The most vital structure, holding data that is required to interface with the file system. This is the ZTFS equivalent of the superblock in EXT. The blueprint is always located at byte 512 on disk. 

Almost all values are (treated as) constants, and only a few are strictly required to interface with the file system. Some of these values are solely for your convenience, as to not require constant calculation.

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

> [!NOTE]
> - The block size MUST be a multiple of 1024, and not lower. 4096 is typical, and ideal. Values larger than 8192 may have unintended consequences, as they are not error checked on creation.

# Block Group Descriptor
Similar to EXT, this structure represents the state of a block group. Each block group descriptor is placed sequentially starting from block address 2. The count of the block group descriptor table (BGDT) is the number of block groups the file system holds.

| Size  | Offset| Name                | Notes                                       |
| ----- | ----- | ------------------- | ------------------------------------------- |
| u32   | 0     | baddr_block_bitmap  | Block address for the used block bitmap     |
| u32   | 4     | baddr_first_entry   | Block address for the first entry           |
| u32   | 8     | free_blocks         | Number of free blocks in this block group   |
| u16   | 12    | num_entries         | Number of entries this block group has      |
| u16   | 14    | reserved            | Padding/Reserved                            |

### Bitmaps
The bitmaps are one block in size, and are ordered by MSB. If block 0 in the group is allocated, the first byte will read 0b10000000. The block immediately following the BGDT contains a contiguous block array, one for each bitmap.

# Entry
An entry represents a file, directory, or any other type of file. These are fixed in size, and are treated like an array. All file types are declared by an entry, including directories. Each entry is placed in a block sequentially without padding, until there is no space left for a full entry. The remaining bytes are unused. 

> [!NOTE]
> - It is not guaranteed that entries are sequential, nor that they may be in the same block. To check for a valid entry, `entry_type` **must** not equal 0.
> - Duplicate entry names are not allowed within the same directory.
> - The root entry is located in the first entry of the block determined by `baddr_root_entry` in the blueprint.

| Size  | Offset| Name                      | Notes                                                                                                     |
| ----- | ----- | ------------------------- | --------------------------------------------------------------------------------------------------------- |
| u8[64]| 0     | name                      | Name of the entry, null-terminated. Max 63 chars.                                                         |
| u64   | 64    | **file_size_bytes**       | Size in bytes if file                                                                                     |
| u64   | 64    | **entry_count**           | Number of entries if directory                                                                            |
| u64   | 64    | **ref_target_entry_index**| Index into the entry array of block pointed to by ref_target_block                                        |
| u32   | 72    | size_blocks               | Number of blocks the data is using, not including either of the indirect and double indirect blocks.      |
| u8    | 76    | entry_type                | Type of entry (see Entry Type)                                                                            |
| u8    | 77    | permissions               | Permissions of the entry (see Permissions)                                                                |
| u32   | 78    | **baddr_indirect_block**  | Block address of indirect block                                                                           |
| u32   | 78    | **ref_target_block**      | Block address of referred entry                                                                           |
| u32   | 82    | baddr_double_indir_block  | Block address of double indirect block, **only** for regular file use.                                    |
| u8    | 83    | entry_idx                 | Entry's index into its parent's entry array                                                               |
| u8    | 84    | data_block_idx            | Entry's index into its parent's indirect block                                                            |
| u32   | 88    | entry_baddr               | Block address where the entry is stored                                                                   |
| u16   | 92    | ref_count                 | Number of times this entry is referenced                                                                  |

> [!IMPORTANT] 
> - Bold items are unions, and are located within the same offset.
> - `entry_idx`, `data_block_idx`, and `entry_baddr` are concrete, and **must** be the true location of the entry.

### Indirect Blocks
Unlike EXT, each file ONLY has one indirect block and one double indirect block. This does limit the size of a file to `(block_size / 4) * (1 + (block_size / 4)) * block_size` (~4GB with 4096 block size). An indirect block holds an array of `baddr_t` pointers to blocks of data. A double indirect block holds an array of `baddr_t` pointers to indirect blocks.

> [!NOTE]
> - Data is in order from the first block address pointer in the single indirect block, to the last block address pointer in the double indirect block. 
> - Because an entry only allocated more blocks when required for data, sparse files are *technically* allowed, but are heavily discouraged. An unallocated block has a block address of 0.

### Entry Type
| Name              | Notes                         |
| ----------------- | ----------------------------- |
| NONE              | Nonexistent Entry             |
| ENTRY_DIRECTORY   | Directory                     |
| ENTRY_FILE        | General File                  |
| ENTRY_REF         | Reference to Entry            |
| ENTRY_CHAR        | Character device              |
| ENTRY_BLOCK       | Block device                  |
| ENTRY_SOCKET      | Net Socket                    |

### References
Reference files are hard links, which point directly to an entry on disk. Each entry has a reference count `ref_count` which is required to be updated on creation/deletion of any reference. An entry cannot be deleted unless its `ref_count` is equal to 0.

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
| APPEND_ONLY   | Writing **must** only append to end of file       |
| PERM_RESERVED | Unused                                            |

# Directories
ZTFS does not differentiate between a directory and a file, meaning a directory is an entry with data pointing to more entry arrays.

Each directory **must** have 2 entries on creation, which are the current entry `./` and the previous entry `../`. The root directory entry is the only exception, which has the current entry for both `./` and `../`. Both the `./` and `../` entries are links of type `ENTRY_REF`.

# Allocation Rules
It is up to the driver on how blocks are allocated, but general convention is first fit. 