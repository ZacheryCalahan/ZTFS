#include "ztfs_insert.h"
#include "../ztfs_block.h"
#include "../ztfs_utils.h"
#include <string.h>

int ztfs_insert_file(char *name, char *file_path, char *path) {
    FILE *image_file;
    image_file = fopen(name, "rb+");
    if (image_file == NULL) {
        printf("Error: Unable to open image.\n");
        return -1;
    }

    FILE *data_file;
    data_file = fopen(file_path, "rb");
    if (data_file == NULL) {
        printf("Error: Unable to open data file.\n");
        return -1;
    }
    
    struct ztfs_blueprint blueprint;
    if (ztfs_read_blueprint(image_file, &blueprint)) {
        printf("Error: Could not read blueprint.\n");
        return -1;
    }

    // Get statistics of the data file
    fseek(data_file, 0, SEEK_END);
    uint64_t data_file_size = ftell(data_file);
    uint32_t blocks_needed = (data_file_size + blueprint.block_size - 1) / blueprint.block_size;

    // Determine parent path and file name
    char *parent_path = malloc(strlen(path) + 1);
    char *entry_name = malloc(strlen(path) + 1);
    if (split_path_from_entry(path, parent_path, entry_name)) {
        printf("Error: Could not split parent and new file.\n");
        free(parent_path);
        free(entry_name);
        return -1;
    }

    // Get parent entry
    struct ztfs_entry parent_entry;
    if (ztfs_find_entry_via_path(image_file, &parent_entry, parent_path)) {
        printf("Error: Could not find parent entry \"%s\"\n", parent_path);
        free(parent_path);
        free(entry_name);
        return -1;
    }

    // Get parent directory indirect block
    baddr_t *parent_idb = malloc(blueprint.block_size);
    ztfs_read(image_file, parent_idb, blueprint.block_size, 1, parent_entry.baddr_indirect_block * blueprint.block_size);

    // Determine where parent entry is on disk
    uint64_t parent_entry_offset = (parent_entry.entry_baddr * blueprint.block_size) + (parent_entry.entry_idx * sizeof(struct ztfs_entry));

    // Search for free entry slot in parent entry data
    uint32_t data_block_index = 0;
    uint32_t entry_index = 0;
    if (ztfs_find_free_entry(image_file, &parent_entry, &data_block_index, &entry_index)) {
        printf("Error: Too many entries in this directory.\n");
        free(parent_idb);
        free(entry_name);
        free(parent_path);
    }

    baddr_t entry_block = parent_idb[data_block_index];

    // TODO: Check if entry exist with this name.

    // Create (incomplete) file entry
    struct ztfs_entry new_file = {
        .name = "",
        .file_size_bytes = data_file_size,
        .size_blocks = blocks_needed,
        .entry_type = ENTRY_FILE,
        .permissions = ALL_RW,
        .baddr_indirect_block = 0,
        .baddr_double_indir_block = 0, // Unallocated.
        .entry_idx = entry_index,
        .data_block_idx = data_block_index,
        .entry_baddr = entry_block
    };
    strcpy(new_file.name, entry_name);

    // Allocate block for indirect block
    baddr_t new_file_indir_block_baddr = ztfs_find_unused_block(image_file);
    if (new_file_indir_block_baddr == -1) {
        printf("Error: Could not allocate a free block for new file's indirect block.\n");
        free(parent_idb);
        free(entry_name);
        free(parent_path);
        return -1;
    }
    new_file.baddr_indirect_block = new_file_indir_block_baddr;
    ztfs_mark_block_used_bitmap(image_file, new_file_indir_block_baddr);

    // Data for new file's indirect block
    baddr_t *new_file_indir_block = malloc(blueprint.block_size);

    // Insert data into the file
    uint8_t *buf = malloc(blueprint.block_size);
    for (int i = 0; i < blocks_needed; i++) {
        // Allocate block
        baddr_t data_block_baddr = ztfs_find_unused_block(image_file);
        if (data_block_baddr == -1) {
            printf("Error: Could not allocate a free block for new file's data.\n");
            free(parent_idb);
            free(entry_name);
            free(parent_path);
            free(new_file_indir_block);
            return -1;
        }
        new_file_indir_block[i] = data_block_baddr;
        ztfs_mark_block_used_bitmap(image_file, data_block_baddr);

        // Update block group
        struct ztfs_block_group_descriptor bdesc;
        uint32_t bg_num = ztfs_find_block_group_from_baddr(image_file, data_block_baddr);
        ztfs_read_bgd(image_file, &bdesc, bg_num);
        bdesc.free_blocks--;
        ztfs_write_bgd(image_file, &bdesc, bg_num);

        // Insert file data
        uint64_t file_offset = (i * blueprint.block_size);
        fseek(data_file, file_offset, 0);
        memset(buf, 0, blueprint.block_size); // Ensure non-needed data in buffer is null.
        fread(buf, blueprint.block_size, 1, data_file);
        ztfs_write(image_file, buf, blueprint.block_size, 1, data_block_baddr * blueprint.block_size);
    }
    free(buf);

    // Insert entry into parent
    uint64_t new_ent_loc = (entry_block * blueprint.block_size) + (sizeof(struct ztfs_entry) * entry_index);
    ztfs_write(image_file, &new_file, sizeof(struct ztfs_entry), 1, new_ent_loc);
    
    // Update block group for file indir block
    struct ztfs_block_group_descriptor bdesc;
    uint32_t bg_num = ztfs_find_block_group_from_baddr(image_file, new_file_indir_block_baddr);
    bdesc.free_blocks--;
    ztfs_write_bgd(image_file, &bdesc, bg_num);

    // Update blueprint
    blueprint.free_blocks -= blocks_needed + 1; // indir and data blocks
    ztfs_write_blueprint(image_file, &blueprint);

    // Update parent entry information
    parent_entry.entry_count++;
    ztfs_write(image_file, &parent_entry, sizeof(struct ztfs_entry), 1, parent_entry_offset);

    free(new_file_indir_block);
    free(parent_idb);
    free(parent_path);
    free(entry_name);
    return 0;
}