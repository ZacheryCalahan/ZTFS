#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "ztfs/ztfs_create.h"
#include "ztfs/ztfs_info.h"

int main(int argc, char **argv) {
    if (argc == 1) {
        // Provide usage
        FILE* help;
        help = fopen("usage.txt", "r");
        if (help == NULL) {
            printf("Error: No usage file found.\n");
            return -1;
        }
        char data[128];
        while (fgets(data, 128, help) != NULL) {
            printf("%s", data);
        }

        return -1;
    }

    char *command = argv[1];

    switch (command[1]) {
        case ('c'): {
            if (argc != 5) {
                printf("Error: Invalid number of arguments for -c.\n");
                return -1;
            }

            uint32_t block_size;
            uint32_t fs_size;
            char extra[32];
            
            if (sscanf(argv[3], "%i %c", &block_size, extra) != 1) {
                printf("Error: Must provide valid integer for block size.\n");
                return -1;
            }

            if (sscanf(argv[4], "%i %c", &fs_size, extra) != 1) {
                printf("Error: Must provide valid integer for file system size.\n");
                return -1;
            }
            
            int ret = ztfs_create_image(argv[2], fs_size, block_size);
            if (ret != 0) {
                printf("Error: Could not generate an image file.\n");
                return -1;
            }
            break;
        }

        case ('p'): {
            int ret = ztfs_print_info(argv[2]);
            if (ret != 0) {
                printf("Error: Could not get info on %s.\n", argv[2]);
                return -1;
            }
            break;
        }

        default:
            printf("Error: No command given.\n");
            return -1;
    }

    return 0;
}