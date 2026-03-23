#include "ztfs_mkdir.h"

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "../ztfs_block.h"
#include "../ztfs_utils.h"

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

        // Assuming blueprint is correctly read, image_file is a valid ZTFS file system. 
        // This means all calls to read/write are safe, and don't need to be checked.
    }

    // Divide the last item of the path from the rest
    char *parent_path = malloc(strlen(path) + 1);
    char *dir_name = malloc(strlen(path) + 1);
    
    if (split_path_from_entry(path, parent_path, dir_name)) {
        printf("Error: Could not split parent entry from new directory name.\n");
        free(dir_name);
        free(parent_path);
        return -1;
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
    ztfs_read(image_file, parent_idb, blueprint.block_size, 1, parent_entry.baddr_indirect_block * blueprint.block_size);

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
        .entry_count = 2, // `./` and `../` entries
        .size_blocks = 1,
        .entry_type = ENTRY_DIRECTORY,
        .permissions = ALL_RW,
        .baddr_indirect_block = 0,
        .baddr_double_indir_block = 0, // Unallocated.
        .entry_idx = entry_index,
        .data_block_idx = data_block_index,
        .entry_baddr = entry_block,
        .ref_count = 2
    };
    strcpy(new_directory.name, dir_name);

    // Allocate block for indirect block
    baddr_t new_dir_indir_block = ztfs_find_unused_block(image_file);
    if (!new_dir_indir_block) {
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
    if (!new_dir_entry_block) {
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
        .entry_count = entry_index,
        .size_blocks = 0,
        .entry_type = ENTRY_REF,
        .permissions = ALL_RW,
        .baddr_indirect_block = new_directory.entry_baddr,
        .baddr_double_indir_block = 0, // Not required.
        .entry_idx = 0,
        .data_block_idx = 0,
        .entry_baddr = new_dir_entry_block,
        .ref_count = 0
    };

    struct ztfs_entry ent_par = {
        .name = "../",
        .entry_count = parent_entry.entry_idx,
        .size_blocks = 0,
        .entry_type = ENTRY_REF,
        .permissions = ALL_RW,
        .baddr_indirect_block = parent_entry.entry_baddr,
        .baddr_double_indir_block = 0, // Not required.
        .entry_idx = 1,
        .data_block_idx = 0,
        .entry_baddr = new_dir_entry_block,
        .ref_count = 0
    };
    
    // Insert new directory entry into parent
    uint32_t new_ent_loc = (entry_block * blueprint.block_size) + (sizeof(struct ztfs_entry) * entry_index);
    ztfs_write(image_file, &new_directory, sizeof(struct ztfs_entry), 1, new_ent_loc);
    
    // Insert "current directory" entry
    ztfs_write(image_file, &ent_cur, sizeof(struct ztfs_entry), 1, new_dir_entry_block * blueprint.block_size);
    
    // Insert "parent directory" entry
    ztfs_write(image_file, &ent_par, sizeof(struct ztfs_entry), 1, (new_dir_entry_block * blueprint.block_size) + sizeof(struct ztfs_entry));

    // Update block group for indirect block
    struct ztfs_block_group_descriptor bdesc;
    uint32_t bg_num = ztfs_find_block_group_from_baddr(image_file, new_dir_indir_block);
    ztfs_read_bgd(image_file, &bdesc, bg_num);
    bdesc.free_blocks--;
    ztfs_write_bgd(image_file, &bdesc, bg_num);
    
    // Update block group for entry block
    bg_num = ztfs_find_block_group_from_baddr(image_file, new_dir_entry_block);
    ztfs_read_bgd(image_file, &bdesc, bg_num);
    bdesc.free_blocks--;
    bdesc.num_entries += 2;
    ztfs_write_bgd(image_file, &bdesc, bg_num);

    // Update the blueprint
    blueprint.free_blocks -= 2;
    ztfs_write_blueprint(image_file, &blueprint);

    // Update parent entry information
    parent_entry.entry_count++;
    ztfs_write(image_file, &parent_entry, sizeof(struct ztfs_entry), 1, parent_entry_offset);
    
    free(parent_idb);
    free(dir_name);
    free(parent_path);
    return 0;
}