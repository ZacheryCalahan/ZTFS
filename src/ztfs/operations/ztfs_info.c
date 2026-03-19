#include "ztfs_info.h"
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include "../ztfs_block.h"

int r_print_tree(FILE *image_file, struct ztfs_blueprint *bp, struct ztfs_entry *entry, int depth);

int ztfs_print_info(char *name) {
    FILE *image_file;
    image_file = fopen(name, "rb");
    if (image_file == NULL) {
        printf("Error: Unable to open file.\n");
        return -1;
    }

    // Read in data structures
    struct ztfs_blueprint blueprint;
    struct ztfs_block_group_descriptor root_bdesc;
    struct ztfs_entry root_entry;

    // Blueprint data first
    if (ztfs_read(image_file, &blueprint, sizeof(struct ztfs_blueprint), 1, 512)) {
        printf("Error: Could not read blueprint from image.\n");
        return -1;
    }

    // Ensure valid ZTFS image
    if (blueprint.signature != ZTFS_MAGIC_SIGNATURE) {
        printf("Error: Invalid signature 0x%lu, not a ZTFS image.\n", blueprint.signature);
        return -1;
    }
    
    // BGD0 (root)
    if (ztfs_read(image_file, &root_bdesc, sizeof(struct ztfs_block_group_descriptor), 1, ZTFS_BGDT_START_BADDR * blueprint.block_size)) {
        printf("Error: Could not read block group descriptor 0.\n");
        return -1;
    }

    // Root Entry
    if (ztfs_read(image_file, &root_entry, sizeof(struct ztfs_entry), 1, blueprint.baddr_root_entry * blueprint.block_size)) {
        printf("Error: Could not read \\ Entry.\n");
        return -1;
    }

    // Print useful blueprint data
    printf("Blueprint:\n");
    printf("\tVersion: %i.%i\n", blueprint.version_major, blueprint.version_minor);
    printf("\tBlock Size: %i\n", blueprint.block_size);
    printf("\tFile System Size: %lu\n", blueprint.size_bytes);
    printf("\tAvailable Blocks: %lu\n", blueprint.free_blocks);
    printf("\tBlock Address of \\ Entry: %i\n", blueprint.baddr_root_entry);
    printf("\tBGDT Size in Blocks: %i\n",blueprint.block_group_table_size);
    printf("\tBitmap Array Block Address: %i\n", blueprint.baddr_bitmap_start);
    printf("\tBlock Group Size in Blocks: %i\n", blueprint.block_group_size);
    printf("\tBlock Address of First Block Group: %i\n", blueprint.baddr_first_block_group);
    printf("\tNumber of Block Groups: %i\n", blueprint.block_group_count);
    printf("\n");

    // Blockdesc info
    printf("Block Group Descriptor 0:\n");
    printf("\tBitmap Block Address: %i\n", root_bdesc.baddr_block_bitmap);
    printf("\tFirst Entry Block Address: %i\n", root_bdesc.baddr_first_entry);
    printf("\tFree Blocks: %i\n", root_bdesc.free_blocks);
    printf("\tEntry Count: %i\n", root_bdesc.num_entries);
    printf("\n");

    // Root entry info
    printf("Root Directory Entry:\n");
    printf("\tName: \"%s\"\n", root_entry.name);
    printf("\tNumber of Entries: %lu\n", root_entry.size);
    printf("\tNumber of Blocks: %i\n", root_entry.size_blocks);
    printf("\tEntry Type: %i\n", root_entry.entry_type);
    printf("\tEntry Permissions: %i\n", root_entry.permissions);
    printf("\tBlock Address of Data: %i\n", root_entry.baddr_indirect_block);
    printf("\tBlock Address of Entry: %i\n", root_entry.entry_baddr);
    printf("\n");


    fclose(image_file);
    return 0;
}

int ztfs_print_entry(char *name, char *path) {
    FILE *image_file;
    image_file = fopen(name, "rb");
    if (image_file == NULL) {
        printf("Error: Unable to open file.\n");
        return -1;
    }
    
    struct ztfs_entry entry;
    if (ztfs_find_entry_via_path(image_file, &entry, path)) {
        printf("Error: Could not find entry at that path.\n");
        return -1;
    }

    printf("Entry:\n");
    printf("\tName: \"%s\"\n", entry.name);
    
    printf("\tEntry Type: %i\n", entry.entry_type);
    if (entry.entry_type == ENTRY_DIRECTORY) {
        printf("\tNumber of Entries: %lu\n", entry.size);
        printf("\tNumber of Blocks: %i\n", entry.size_blocks);
        printf("\tBlock Address of Data: %i\n", entry.baddr_indirect_block);
    } else if (entry.entry_type == ENTRY_REF) {
        printf("\tReferencing Entry Index: %lu\n", entry.size);
        printf("\tOf Block Address: %i\n", entry.baddr_indirect_block);
    } else {
        printf("\tFile size: %lu\n", entry.size);
        printf("\tNumber of Blocks: %i\n", entry.size_blocks);
        printf("\tBlock Address of Data: %i\n", entry.baddr_indirect_block);
    }

    printf("\tEntry Permissions: %i\n", entry.permissions);
    printf("\tParent Entry Block Index: %i\n", entry.data_block_idx);
    printf("\tParent Entry Array Index: %i\n", entry.entry_idx);
    printf("\tBlock Address of Entry: %i\n", entry.entry_baddr);
    printf("\n");

    return 0;
}

int ztfs_print_tree(char* name) {
    FILE *image_file;
    image_file = fopen(name, "rb");
    if (image_file == NULL) {
        printf("Error: Unable to open file.\n");
        return -1;
    }

        // Read in data structures
    struct ztfs_blueprint blueprint;
    struct ztfs_block_group_descriptor root_bdesc;
    struct ztfs_entry root_entry;

    // Blueprint
    if (ztfs_read(image_file, &blueprint, sizeof(struct ztfs_blueprint), 1, 512)) {
        printf("Error: Could not read blueprint from image.\n");
        return -1;
    }

    // Ensure valid ZTFS image
    if (blueprint.signature != ZTFS_MAGIC_SIGNATURE) {
        printf("Error: Invalid signature 0x%lu, not a ZTFS image.\n", blueprint.signature);
        return -1;
    }
    
    // BGD0 (root)
    if (ztfs_read(image_file, &root_bdesc, sizeof(struct ztfs_block_group_descriptor), 1, ZTFS_BGDT_START_BADDR * blueprint.block_size)) {
        printf("Error: Could not read block group descriptor 0.\n");
        return -1;
    }

    // Root Entry
    if (ztfs_read(image_file, &root_entry, sizeof(struct ztfs_entry), 1, blueprint.baddr_root_entry * blueprint.block_size)) {
        printf("Error: Could not read \\ Entry.\n");
        return -1;
    }

    // Traverse the directories
    r_print_tree(image_file, &blueprint, &root_entry, 0);
    
    fclose(image_file);
    return 0;
}

int r_print_tree(FILE *image_file, struct ztfs_blueprint *bp, struct ztfs_entry *entry, int depth) {
    // Print current entry with indent
    for (int i = 0; i < depth; i++) printf("  ");

    if (depth != 0) printf("/"); // If not root dir, print '/' first to indicate directory.
    
    printf("%s\n", entry->name);
    

    // If directory, read children
    
    if (entry->entry_type == ENTRY_DIRECTORY) {
        struct ztfs_entry *subdirs = malloc(bp->block_size);
        baddr_t *entry_indir_block = malloc(bp->block_size); // Entry's indirect block
        uint32_t entry_count = entry->size; // Number of entries to find
        
        if (ztfs_read(image_file, entry_indir_block, bp->block_size, 1, entry->baddr_indirect_block * bp->block_size)) {
            printf("Error: Could not read \"%s\" data.\n", entry->name);
            free(subdirs);
            free(entry_indir_block);
            return -1;
        }
        
        for (uint32_t entry_block_num = 0; entry_block_num < bp->block_size / sizeof(baddr_t); entry_block_num++) {
            if (entry_indir_block[entry_block_num] == 0) continue; // Skip unallocated blocks
            ztfs_read(image_file, subdirs, bp->block_size, 1, entry_indir_block[entry_block_num] * bp->block_size);
            // Iterate through entry array
            for (uint32_t i = 0; i < bp->block_size / sizeof(struct ztfs_entry); i++) {
                struct ztfs_entry cur_entry = subdirs[i];
                if (cur_entry.entry_type == NONE) continue; // Skip empty entries
                
                entry_count--; // Must be a valid entry!
                if (cur_entry.entry_type == ENTRY_DIRECTORY) {
                    if (r_print_tree(image_file, bp, &cur_entry, depth + 1)) {
                        printf("Error: Could not traverse \"%s\".\n", cur_entry.name);
                        free(entry_indir_block);
                        free(subdirs);
                        return -1;
                    }
                    
                } else if (subdirs[i].entry_type == ENTRY_FILE) {
                    for (int i = 0; i < depth; i++) printf("  ");
                    printf("%s\n", subdirs[i].name);
                }
                
                if (entry_count == 0) break;
            }

            if (entry_count == 0) break;
        }

        free(entry_indir_block);
        free(subdirs);
    }
    return 0;
}