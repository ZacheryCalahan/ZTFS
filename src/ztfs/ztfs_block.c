#include "ztfs_block.h"

#include <string.h>

int ztfs_write(FILE *image_file, void *buffer, size_t size, size_t n, size_t offset) {
    if (image_file == NULL) {
        printf("Error: Invalid image.\n");
        return -1;
    }

    if (buffer == NULL) {
        printf("Error: Invalid buffer on write.\n");
        return -1;
    }

    if (size <= 0 || n <= 0) {
        return 0; // Do nothing
    }

    fseek(image_file, 0L, SEEK_END);
    long file_size = ftell(image_file);
    if (offset + (size * n) > (size_t) file_size) {
        printf("Error: Cannot write past file size.\n");
        return -1;
    }

    fseek(image_file, offset, 0);
    fwrite(buffer, size, n, image_file);

    return 0;

}

int ztfs_read(FILE *image_file, void *buffer, size_t size, size_t n, size_t offset) {
    if (image_file == NULL) {
        printf("Error: Invalid image.\n");
        return -1;
    }

    if (buffer == NULL) {
        printf("Error: Invalid buffer on read.\n");
        return -1;
    }

    if (size <= 0 || n <= 0) {
        return 0; // Do nothing
    }

    fseek(image_file, 0L, SEEK_END);
    long file_size = ftell(image_file);
    if (offset + (size * n) > (size_t) file_size) {
        printf("Error: Cannot read past file size.\n");
        return -1;
    }

    fseek(image_file, offset, 0);
    fread(buffer, size, n, image_file);

    return 0;
}

int ztfs_write_blueprint(FILE *image_file, struct ztfs_blueprint *blueprint) {
    if (image_file == NULL) {
        printf("Error: Invalid image.\n");
        return -1;
    }

    if (blueprint == NULL) {
        printf("Error: NULL BGD buffer.\n");
        return -1;
    }

    if (blueprint->signature != ZTFS_MAGIC_SIGNATURE) {
        printf("Error: Invalid ZTFS Signature.\n");
        return -1;
    }

    fseek(image_file, 512, 0);
    fwrite(blueprint, sizeof(struct ztfs_blueprint), 1, image_file);

    return 0;
}

int ztfs_read_blueprint(FILE *image_file, struct ztfs_blueprint *blueprint) {
    if (image_file == NULL) {
        printf("Error: Invalid image.\n");
        return -1;
    }

    if (blueprint == NULL) {
        printf("Error: NULL BGD buffer.\n");
        return -1;
    }

    fseek(image_file, 512, 0);
    fread(blueprint, sizeof(struct ztfs_blueprint), 1, image_file);

    if (blueprint->signature != ZTFS_MAGIC_SIGNATURE) {
        printf("Error: Invalid ZTFS Signature.\n");
        return -1;
    }

    return 0;
}

int ztfs_write_bgd(FILE *image_file, struct ztfs_block_group_descriptor *desc, uint32_t bg_num) {
    if (image_file == NULL) {
        printf("Error: Invalid image.\n");
        return -1;
    }

    if (desc == NULL) {
        printf("Error: NULL BGD buffer.\n");
        return -1;
    }

    struct ztfs_blueprint blueprint;

    if (ztfs_read_blueprint(image_file, &blueprint)) {
        printf("Error: Could not read blueprint.\n");
        return -1;
    }

    if (bg_num > ((blueprint.size_bytes / blueprint.block_size) / (blueprint.block_size / 4))) {
        printf("Error: Attempt to write to BGD number higher than exists.\n");
        return -1;
    }

    uint32_t bgx_offset = (blueprint.block_size * ZTFS_BGDT_START_BADDR) + (bg_num * sizeof(struct ztfs_block_group_descriptor));

    fseek(image_file, bgx_offset, 0);
    fwrite(desc, sizeof(struct ztfs_blueprint), 1, image_file);

    return 0;
}

int ztfs_read_bgd(FILE *image_file, struct ztfs_block_group_descriptor *desc, uint32_t bg_num) {
    if (image_file == NULL) {
        printf("Error: Invalid image.\n");
        return -1;
    }

    if (desc == NULL) {
        printf("Error: NULL BGD buffer.\n");
        return -1;
    }

    struct ztfs_blueprint blueprint;

    if (ztfs_read_blueprint(image_file, &blueprint)) {
        printf("Error: Could not read blueprint.\n");
        return -1;
    }

    if (bg_num > ((blueprint.size_bytes / blueprint.block_size) / (blueprint.block_size / 4))) {
        printf("Error: Attempt to read from BGD number higher than exists.\n");
        return -1;
    }

    uint32_t bgx_offset = (blueprint.block_size * ZTFS_BGDT_START_BADDR) + (bg_num * sizeof(struct ztfs_block_group_descriptor));

    fseek(image_file, bgx_offset, 0);
    fread(desc, sizeof(struct ztfs_block_group_descriptor), 1, image_file);

    return 0;
}

int ztfs_read_bitmap(FILE *image_file, void* bitmap_block, uint32_t bg_num) {
    if (image_file == NULL) {
        printf("Error: Invalid image.\n");
        return -1;
    }

    struct ztfs_blueprint blueprint;

    if (ztfs_read_blueprint(image_file, &blueprint)) {
        printf("Error: Could not read blueprint.\n");
        return -1;
    }

    struct ztfs_block_group_descriptor bgd;

    if (ztfs_read_bgd(image_file, &bgd, bg_num)) {
        printf("Error: Could not read BGD.\n");
        return -1;
    }

    if (ztfs_read(image_file, bitmap_block, blueprint.block_size, 1, bgd.baddr_block_bitmap * blueprint.block_size)) {
        printf("Error: Could not read bitmap.\n");
        return -1;
    }

    return 0;
}

int ztfs_mark_block_used_bitmap(FILE *image_file, baddr_t baddr_block) {
    if (image_file == NULL) {
        printf("Error: Invalid image.\n");
        return -1;
    }

    struct ztfs_blueprint blueprint;

    if (ztfs_read_blueprint(image_file, &blueprint)) {
        printf("Error: Could not read blueprint.\n");
        return -1;
    }

    // Determine which block group block belongs to (Data blocks don't start at baddr 0.)
    if (baddr_block < blueprint.baddr_first_block_group) {
        printf("Error: Cannot mark block before first block group.\n");
        return -1;
    }

    uint32_t block_idx_from_start = baddr_block - blueprint.baddr_first_block_group;
    uint32_t bg_num = block_idx_from_start / blueprint.block_group_size;
    struct ztfs_block_group_descriptor bgd;

    if (ztfs_read_bgd(image_file, &bgd, bg_num)) {
        printf("Error: Could not read BG%i.\n", bg_num);
        return -1;
    }

    uint32_t bnum_in_bg = block_idx_from_start % blueprint.block_group_size;
    uint32_t byte = bnum_in_bg / 8;
    uint32_t bit = bnum_in_bg % 8;
    uint8_t mask = 0b10000000 >> bit;

    uint8_t bitmap_byte;

    if (ztfs_read(image_file, &bitmap_byte, sizeof(uint8_t), 1, (bgd.baddr_block_bitmap * blueprint.block_size) + byte)) {
        printf("Error: Could not read bitmap.\n");
        return -1;
    }

    bitmap_byte |= mask;

    if (ztfs_write(image_file, &bitmap_byte, sizeof(uint8_t), 1, (bgd.baddr_block_bitmap * blueprint.block_size) + byte)) {
        printf("Error: Could not update bitmap.\n");
        return -1;
    }
    
    return 0;
}

baddr_t ztfs_find_unused_block(FILE *image_file) {
    if (image_file == NULL) {
        printf("Error: Invalid file on write.\n");
        return -1;
    }

    struct ztfs_blueprint blueprint;

    if (ztfs_read_blueprint(image_file, &blueprint)) {
        printf("Error: Could not read blueprint.\n");
        return -1;
    }

    // Iterate through bitmaps in each block group
    uint8_t *bitmap = malloc(blueprint.block_size);
    for (uint32_t bg = 0; bg < blueprint.block_group_count; bg++) {
        struct ztfs_block_group_descriptor bgd;
        if (ztfs_read_bgd(image_file, &bgd, bg)) {
            printf("Error: Could not read Block Group %i.\n", bg);
            free(bitmap);
            return -1;
        }
        
        ztfs_read(image_file, bitmap, blueprint.block_size, 1, bgd.baddr_block_bitmap * blueprint.block_size);
        for (uint32_t bitmap_byte = 0; bitmap_byte < blueprint.block_size; bitmap_byte++) {
            
            if (bitmap[bitmap_byte] == 0xFF) continue; // Skip full bytes

            // Iterate through the bits
            for (int i = 0; i < 8; i++) {
                uint8_t mask = 0b10000000 >> i; // Ordered by MSB!
                if ((bitmap[bitmap_byte] & mask) == 0) {
                    // Calculate block number
                    uint32_t local_block_num = (bitmap_byte * 8) + i;
                    // (offset from start of the block groups) + (offset of what block group we're in) + (local block offset in bg)
                    uint32_t absolute_block_num = (blueprint.baddr_first_block_group) + (bg * (blueprint.block_size * 8)) + local_block_num;
                    free(bitmap);
                    return absolute_block_num;
                }
            }
        }
    }

    free(bitmap);
    return -1; // Did not find unused block.
}

int ztfs_find_entry_via_path(FILE *image_file, struct ztfs_entry *entry, char *path) {
    if (image_file == NULL) {
        printf("Error: Invalid file on write.\n");
        return -1;
    }

    if (entry == NULL) {
        printf("Error: Null entry buffer supplied.\n");
        return -1;
    }

    struct ztfs_blueprint blueprint;
    if (ztfs_read_blueprint(image_file, &blueprint)) {
        printf("Error: Could not read blueprint.\n");
        return -1;
    }

    struct ztfs_entry root_entry;
    if (ztfs_read(image_file, &root_entry, sizeof(struct ztfs_entry), 1, blueprint.baddr_root_entry * blueprint.block_size)) {
        printf("Error: Could not read in root entry.\n");
        return -1;
    }

    // Handle case of searching for root entry
    if (strcmp(path, "/") == 0) {
        memcpy(entry, &root_entry, sizeof(struct ztfs_entry));
        return 0;
    }

    // Count the directories to travel
    int dircount = 0;
    for (size_t i = 0; i < strlen(path); i++) if (path[i] == '/') dircount++;

    // Split the path by `/` and search directory by directory
    char* token;
    char* saveptr = path;

    struct ztfs_entry entry_to_search = root_entry;
    baddr_t *block_pointers = malloc(blueprint.block_size);
    struct ztfs_entry *entries = malloc(blueprint.block_size);

    token = __strtok_r(path, "/", &saveptr);
    while (token != NULL) {
        // Get the indirect block of the current entry
        uint32_t bp_addr = entry_to_search.baddr_indirect_block * blueprint.block_size;
        if (ztfs_read(image_file, block_pointers, blueprint.block_size, 1, bp_addr)) {
            printf("Error: Could not read indirect block.\n");
            return -1;
        }

        // Traverse each block in the indirect block
        int entry_found = 0;
        for (uint32_t bp_idx = 0; bp_idx < blueprint.block_size / sizeof(baddr_t); bp_idx++) {
            if (block_pointers[bp_idx] == 0) continue; // Skip unallocated blocks
            
            uint32_t entry_addr = block_pointers[bp_idx] * blueprint.block_size; // Entry array block address
            if (ztfs_read(image_file, entries, blueprint.block_size, 1, entry_addr)) {
                printf("Error: Could not read entry array.\n");
                return -1;
            }

            // Traverse each entry in this block
            uint32_t entry_count = entry_to_search.size;

            for (uint32_t entry_idx = 0; entry_idx < blueprint.block_size / sizeof(struct ztfs_entry); entry_idx++) {
                if (entry_count == 0) break;
                if (entries[entry_idx].entry_type == 0) continue; // Skip null entries
                
                entry_count--; // Entry must be valid.
                if (strcmp(entries[entry_idx].name, token) == 0) {
                    // Entry found, determine what to do next
                    if (entries[entry_idx].entry_type == ENTRY_DIRECTORY && dircount != 1) {
                        // Subdir, go deeper
                        entry_to_search = entries[entry_idx];
                        entry_found = 1;
                        dircount--;
                        break;
                    }

                    // Entry is not a directory, so if not at end of path return with error.
                    if (dircount == 1) {
                        memcpy(entry, &entries[entry_idx], sizeof(struct ztfs_entry));
                        free(block_pointers);
                        free(entries);
                        return 0; // Found item!
                    }

                    printf("Error: \"%s\" exists as a non-directory already.\n", token);
                    return -1;
                }
            }

            if (entry_found) {
                token = __strtok_r(NULL, "/", &saveptr);
                break;
            }

            if (entry_count == 0) {
                printf("Error: Could not find path item \"%s\"\n", token);
                return -1;
            }

            token = __strtok_r(NULL, "/", &saveptr);
        }

    }
    
    free(block_pointers);
    free(entries);
    return -1; // Entry not found.
}

int ztfs_find_free_entry(FILE *image_file, struct ztfs_entry *parent, uint32_t *data_block_idx, uint32_t *entry_idx) {
    if (image_file == NULL) {
        printf("Error: Invalid file on write.\n");
        return -1;
    }

    if (data_block_idx == NULL || entry_idx == NULL) {
        printf("Error: Null entry buffer(s) supplied.\n");
        return -1;
    }

    struct ztfs_blueprint blueprint;
    if (ztfs_read_blueprint(image_file, &blueprint)) {
        printf("Error: Could not read blueprint.\n");
        return -1;
    }

    // Get the entry's indirect bp
    baddr_t *ibp = malloc(blueprint.block_size);
    if (ztfs_read(image_file, ibp, blueprint.block_size, 1, parent->baddr_indirect_block * blueprint.block_size)) {
        printf("Error: Could not read \"%s\"s data.\n", parent->name);
        return -1;
    }

    // Search through each entry's data blocks
    struct ztfs_entry *parent_entries = malloc(blueprint.block_size);
    for (uint32_t dbi = 0; dbi < blueprint.block_size / sizeof(baddr_t); dbi++) {
        if (ztfs_read(image_file, parent_entries, blueprint.block_size, 1, ibp[dbi] * blueprint.block_size)) {
            printf("Error: Could not read \"%s\"s block pointer data.\n", parent->name);
            return -1;
        }

        // Search through this block's entries
        for (uint32_t ei = 0; ei < blueprint.block_size / sizeof(struct ztfs_entry); ei++) {
            if (parent_entries[ei].entry_type == NONE) {
                *entry_idx = ei;
                *data_block_idx = dbi;
                free(parent_entries);
                free(ibp);
                return 0;
            } 
        }
    }
    
    free(parent_entries);
    free(ibp);
    return -1; // No space found
}

int ztfs_free_block_bitmap(FILE *image_file, baddr_t baddr_block) {
    
    return 0;
}

int ztfs_place_entry(FILE *image_file, struct ztfs_entry *parent, struct ztfs_entry *child) {

    return 0;
}

