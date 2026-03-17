#include "../src/ztfs/ztfs.h"
#include "../src/ztfs/ztfs_block.h"
#include "../src/ztfs/operations/ztfs_create.h"
#include <stdio.h>
#include <stddef.h>

int main(int argc, char **argv) {
    printf("Creating image...\n");
    ztfs_create_image("disk.img", 4194304, 4096);

    printf("Loading created image...\n");
    FILE *image_file;
    image_file = fopen("disk.img", "rb+");
    
    if (image_file == NULL) {
        printf("Error: Could not open image.\n");
        return -1;
    }

    printf("Searching for root dir!\n");
    struct ztfs_entry ent;
    if (ztfs_find_entry_via_path(image_file, &ent, "/")) {
        printf("Entry not found?\n");
        return -1;
    }
    
    printf("\"%s\" Found!\n", ent.name);

}

