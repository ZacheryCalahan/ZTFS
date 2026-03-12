#include "ztfs_info.h"
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>

int ztfs_print_info(char* name) {
    FILE *image_file;
    image_file = fopen(name, "rb");
    if (name == NULL) {
        printf("Error: Unable to open file.\n");
        return -1;
    }

    // Read in data structures
    struct ztfs_blueprint blueprint;
    struct ztfs_block_group_descriptor root_bdesc;
    struct ztfs_entry root_entry;

    // Blueprint data first
    fseek(image_file, 512, 0);
    int flag = fread(&blueprint, sizeof(struct ztfs_blueprint), 1, image_file);
    if (!flag) {
        printf("Error: Could not read blueprint from image.\n");
        return -1;
    }

    // Ensure valid ZTFS image
    if (blueprint.signature != ZTFS_MAGIC_SIGNATURE) {
        printf("Error: Invalid signature 0x%lu, not a ZTFS image.\n", blueprint.signature);
        return -1;
    }
    printf("Signature found: 0x%lx\n", blueprint.signature);

    // Print useful blueprint data
    printf("Blueprint:\n");
    printf("\tVersion: %i.%i\n", blueprint.version_major, blueprint.version_minor);
    printf("\tBlock Size: %i\n", blueprint.block_size);
    printf("\tFile System Size: %lu\n", blueprint.size_bytes);
    printf("\tAvailable Blocks: %lu\n", blueprint.free_blocks);
    printf("\tBlock Address of \\: %i\n", blueprint.baddr_root_entry);
    printf("\tBGDT Size in Blocks: %i\n",blueprint.block_group_table_size);
    printf("\tBitmap Block Address: %i\n\n", blueprint.baddr_bitmap_start);

    // Blockdesc info
    fseek(image_file, ZTFS_BGDT_START_BADDR * blueprint.block_size, 0);
    flag = fread(&root_bdesc, sizeof(struct ztfs_block_group_descriptor), 1, image_file);
    if (!flag) {
        printf("Error: Could not read block group descriptor 0.\n");
        return -1;
    }
    printf("Block Group Descriptor 0:\n");
    printf("\tBitmap Block Address: %i\n", root_bdesc.baddr_block_bitmap);
    printf("\tFirst Entry Block Address: %i\n", root_bdesc.baddr_first_entry);
    printf("\tFree Blocks: %i\n", root_bdesc.free_blocks);
    printf("\tEntry Count: %i\n\n", root_bdesc.num_entries);

    // Root entry info
    fseek(image_file, blueprint.baddr_root_entry * blueprint.block_size, 0);
    flag = fread(&root_entry, sizeof(struct ztfs_entry), 1, image_file);
    if (!flag) {
        printf("Error: Could not read \\ Entry.\n");
        return -1;
    }
    printf("Root Directory Entry:\n");
    printf("\tName: \"%s\"\n", root_entry.name);
    printf("\tNumber of Entries: %lu\n", root_entry.size);
    printf("\tNumber of Blocks: %i\n", root_entry.size_blocks);
    printf("\tEntry Type: %i\n", root_entry.entry_type);
    printf("\tEntry Permissions: %i\n", root_entry.permissions);
    printf("\tBlock Address of Data: %i\n\n", root_entry.baddr_indirect_block);

    return 0;
}