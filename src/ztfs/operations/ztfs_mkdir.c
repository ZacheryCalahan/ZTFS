#include "ztfs_mkdir.h"

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "../ztfs_block.h"

int ztfs_mkdir(char* name, char* path) {
    FILE *image_file;
    image_file = fopen(name, "rb+");
    if (image_file == NULL) {
        printf("Error: Unable to open file.\n");
        return -1;
    }

    // Read in data structures
    struct ztfs_blueprint blueprint;

    if (ztfs_read_blueprint(image_file, &blueprint)) {
        printf("Error: Could not read blueprint.\n");
        return -1;
    }

    // Divide the last item of the path from the rest
    char *parent_path = malloc(strlen(path) + 1);
    char *dir_name = malloc(strlen(path) + 1);

    char *last = strrchr(path, '/');

    if (last == NULL) {
        printf("Error: Invalid directory name.\n");
        return -1;
    } else {
        size_t parent_len = last - path;

        if (parent_len == 0) {
            // Parent is (probably.) the root directory
            strcpy(parent_path, "/");
        } else {
            strncpy(parent_path, path, parent_len);
            parent_path[parent_len] = '\0';
        }

        strcpy(dir_name, last + 1);
    }

    // Find parent entry via its path
    struct ztfs_entry parent_entry;
    if (ztfs_find_entry_via_path(image_file, &parent_entry, parent_path)) {
        printf("Error: Could not find path to \"%s\"\n", parent_path);
        free(dir_name);
        free(parent_path);
        return -1;
    }
    uint64_t parent_entry_offset = (parent_entry.entry_baddr * blueprint.block_size) + (parent_entry.entry_idx * sizeof(struct ztfs_entry));

    // Get parent directory indirect block
    baddr_t *parent_idb = malloc(blueprint.block_size);
    if (ztfs_read(image_file, parent_idb, blueprint.block_size, 1, parent_entry.baddr_indirect_block * blueprint.block_size)) {
        printf("Error: Could not get parent directory indirect block pointer.\n");
        free(parent_idb);
        free(dir_name);
        free(parent_path);
        return -1;
    }

    // Search for free entry slot in parent entry data
    uint32_t data_block_index = 0;
    uint32_t entry_index = 0;
    if (ztfs_find_free_entry(image_file, &parent_entry, &data_block_index, &entry_index)) {
        printf("Error: Too many entries in this directory.\n");
        free(parent_idb);
        free(dir_name);
        free(parent_path);
        return -1;
    }

    baddr_t entry_block = parent_idb[data_block_index]; // Block number for new entry

    // Create data structures for new entry
    struct ztfs_entry new_directory = {
        .name = "",
        .size = 2, // `./` and `../` entries
        .size_blocks = 1,
        .entry_type = ENTRY_DIRECTORY,
        .permissions = ALL_RW,
        .baddr_indirect_block = 0,
        .entry_idx = entry_index,
        .data_block_idx = data_block_index,
        .entry_baddr = entry_block,
    };
    strcpy(new_directory.name, dir_name);

    // Allocate block for indirect block
    baddr_t new_dir_indir_block = ztfs_find_unused_block(image_file);
    if (new_dir_indir_block == -1) {
        printf("Error: Could not allocate a free block for indirect block.\n");
        free(parent_idb);
        free(dir_name);
        free(parent_path);
        return -1;
    }
    new_directory.baddr_indirect_block = new_dir_indir_block;
    ztfs_mark_block_used_bitmap(image_file, new_dir_indir_block);

    // Allocate block for new directory entries
    baddr_t new_dir_entry_block = ztfs_find_unused_block(image_file);
    if (new_dir_entry_block == -1) {
        printf("Error: Could not allocate a free block for new directory's entries.\n");
        free(parent_idb);
        free(dir_name);
        free(parent_path);
        return -1;
    }
    ztfs_mark_block_used_bitmap(image_file, new_dir_entry_block);

    // Write entry array baddr into first indirect block pointer
    ztfs_write(image_file, &new_dir_entry_block, sizeof(baddr_t), 1, new_directory.baddr_indirect_block * blueprint.block_size);

    // Parent and current directory entries for new dir
    struct ztfs_entry ent_cur = {
        .name = "./",
        .size = entry_index,
        .size_blocks = 0,
        .entry_type = ENTRY_REF,
        .permissions = ALL_RW,
        .baddr_indirect_block = new_directory.entry_baddr,
        .entry_idx = 0,
        .data_block_idx = 0,
        .entry_baddr = new_dir_entry_block
    };

    struct ztfs_entry ent_par = {
        .name = "../",
        .size = parent_entry.entry_idx,
        .size_blocks = 0,
        .entry_type = ENTRY_REF,
        .permissions = ALL_RW,
        .baddr_indirect_block = parent_entry.entry_baddr,
        .entry_idx = 1,
        .data_block_idx = 0,
        .entry_baddr = new_dir_entry_block
    };
    
    // Insert new directory entry into parent
    uint32_t new_ent_loc = (entry_block * blueprint.block_size) + (sizeof(struct ztfs_entry) * entry_index);
    if (ztfs_write(image_file, &new_directory, sizeof(struct ztfs_entry), 1, new_ent_loc)) {
        printf("Error: Could not write to new directory's parent entry list.\n");
        free(parent_idb);
        free(dir_name);
        free(parent_path);
        return -1;
    }

    // Insert "current directory" entry
    ztfs_write(image_file, &ent_cur, sizeof(struct ztfs_entry), 1, new_dir_entry_block * blueprint.block_size);
    
    // Insert "parent directory" entry
    ztfs_write(image_file, &ent_par, sizeof(struct ztfs_entry), 1, (new_dir_entry_block * blueprint.block_size) + sizeof(struct ztfs_entry));

    // Update block group

    // Update parent entry information
    parent_entry.size++;
    ztfs_write(image_file, &parent_entry, sizeof(struct ztfs_entry), 1, parent_entry_offset);
    

    free(parent_idb);
    free(dir_name);
    free(parent_path);
    return 0;
}