#include "ztfs_utils.h"
#include <stddef.h>

int split_path_from_entry(char *full_path, char *parent_path, char *entry_name) {
    // Divide the last item of the path from the rest
    char *last = strrchr(full_path, '/');

    if (last == NULL) {
        printf("Error: Invalid directory name.\n");
        return -1;
    } else {
        size_t parent_len = last - full_path;

        if (parent_len == 0) {
            // Parent is (probably.) the root directory
            strcpy(parent_path, "/");
        } else {
            strncpy(parent_path, full_path, parent_len);
            parent_path[parent_len] = '\0';
        }

        strcpy(entry_name, last + 1);
    }

    return 0;
}