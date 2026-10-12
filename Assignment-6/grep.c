#include "engine.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char** argv) {
    if (argc != 4) {
        fprintf(stderr,
                "Usage: %s <count|instance> <input_file> <target_word>\n",
                argv[0]);
        return 1;
    }

    if (strcmp(argv[1], "count") == 0) {
        int count = search_count(argv[2], argv[3]);
        printf("Found: %d of %s in %s\n", count, argv[3], argv[2]);
    } else if (strcmp(argv[1], "instance") == 0) {
        struct count_result result = search_instance(argv[2], argv[3]);
        printf("Found: %d of %s in %s\n", result.count, argv[3], argv[2]);
        for (int index = 0; index < result.count; index++) {
            printf("res.instances[%d]: %s\n", index, result.instances[index]);
            free(result.instances[index]);
        }
        free(result.instances);
    } else {
        fprintf(stderr,
                "Usage: %s <count|instance> <input_file> <target_word>\n",
                argv[0]);
        return 1;
    }

    return 0;
}