#include "ztfs_create.h"
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#define CHUNK_SIZE 4096

int ztfs_create_image(char* name, uint64_t size, uint32_t block_size) {
    // Ensure size is feasable
    if (size < 4194303) {
        printf("This will generate a file less than 4MiB, which will likely cause corruption. Aborting.\n");
        return -1;
    }
    
    if (size > 1073741824) {
        // Ensure user wants to make a 1GB or larger file
        printf("This will generate a file greater than 1GiB, do you want to continue? (y/n)\n");
        while (1) {
            char input[4];
            scanf("%s", input);
            if (input[0] == 'n') {
                printf("Aborting file creation.\n");
                return -1;
            } else if (input[0] == 'y') {
                printf("Continuing!\n");
                break;
            }
        }
    }
    
    // Ensure block size is greater than 1024, and a multiple of 1024.
    if (block_size < 1024 || block_size % 1024 != 0) {
        printf("Error: Invalid block size. Must be a multiple of 1024.\n");
        return -1;
    }

    // Create the file
    FILE *image_file;
    image_file = fopen(name, "wb+");
    if (name == NULL) {
        printf("Error: Unable to open/create file.\n");
        return -1;
    }
    
    char buffer[CHUNK_SIZE];
    memset(buffer, 0, CHUNK_SIZE);
    size_t written_size = 0;

    while (written_size < size) {
        // Calculate bytes to write
        size_t bytes_to_write = CHUNK_SIZE;
        if (written_size + CHUNK_SIZE > size) {
            bytes_to_write = size - written_size;
        }

        // Write bytes
        if (fwrite(buffer, 1, bytes_to_write, image_file) != bytes_to_write) {
            printf("Error: Unable to write to image file.");
            fclose(image_file);
            return -1;
        }
        written_size += bytes_to_write;
    }

    // Useful variables
    uint32_t blocks_per_bgroup = (block_size / 4);
    uint32_t blocks_in_filesystem = (size / block_size);
    uint32_t num_bgd = (blocks_in_filesystem / blocks_per_bgroup);
    uint32_t num_bgd_per_block = block_size / sizeof(struct ztfs_block_group_descriptor);
    uint32_t num_bgd_blocks = (num_bgd + num_bgd_per_block - 1) / num_bgd_per_block;
    uint32_t baddr_start_data = ZTFS_BGDT_START_BADDR + num_bgd_blocks; // Start of the first data block
    uint32_t baddr_root_node = baddr_start_data + num_bgd; // Root block starts after all bitmaps

    // Create data structures
    struct ztfs_blueprint blueprint = {
        .signature = ZTFS_MAGIC_SIGNATURE,
        .version_major = ZTFS_VERSION_MAJOR,
        .version_minor = ZTFS_VERSION_MINOR,
        .block_size = block_size,
        .size_bytes = size,
        .free_blocks = (blocks_per_bgroup * num_bgd) - num_bgd - 1, // All block groups - bitmap blocks - root entry
        .baddr_root_entry = baddr_root_node,
        .block_group_table_size = num_bgd_blocks,
        .baddr_bitmap_start = baddr_start_data
    };

    struct ztfs_block_group_descriptor root_bdesc = {
        .baddr_block_bitmap = baddr_start_data,
        .baddr_first_entry = baddr_root_node,
        .free_blocks = blocks_per_bgroup - 2, // Root entry block, root data block.
        .num_entries = 1,
        .reserved = 0
    };

    // This is a VERY important entry, as it's the root of all other entries. No writing allowed!
    struct ztfs_entry root_entry = {
        .name = "/",
        .size = 0,
        .size_blocks = 1,
        .entry_type = ENTRY_DIRECTORY,
        .permissions = SUPER_READ | USER_READ,
        .baddr_indirect_block = baddr_root_node + 1
    };

    // Place blueprint in file
    fseek(image_file, 512, 0);
    int flag = fwrite(&blueprint, sizeof(struct ztfs_blueprint), 1, image_file);
    if (!flag) {
        printf("Error: Could not write blueprint to file.\n");
        return -1;
    }

    // Place initial BGD in file
    fseek(image_file, ZTFS_BGDT_START_BADDR * block_size, 0);
    flag = fwrite(&root_bdesc, sizeof(struct ztfs_block_group_descriptor), 1, image_file);
    if (!flag) {
        printf("Error: Could not write root block group descriptor to file.\n");
        return -1;
    }

    // Place root entry in file
    fseek(image_file, baddr_root_node * block_size, 0);
    flag = fwrite(&root_entry, sizeof(struct ztfs_entry), 1, image_file);
    if (!flag) {
        printf("Error: Could not write root entry to file.\n");
        return -1;
    }

    // Mark usage in the bitmaps
    uint32_t baddr_bitmap = root_bdesc.baddr_block_bitmap;
    fseek(image_file, baddr_bitmap * block_size, 0);
    uint8_t bitmap_marking = 0b11000000;
    flag = fwrite(&bitmap_marking, sizeof(uint8_t), 1, image_file);
    if (!flag) {
        printf("Error: Could not write bitmaps to file.\n");
        return -1;
    }

    fclose(image_file);
    return 0;
}