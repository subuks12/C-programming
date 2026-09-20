#include "barcode.h"
#include "loader.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char** argv) {
    if (argc != 5) {
        printf("Incorrect number of arguments. Expected: ./build/image_calc <barcode_value> <width> <height> <output_image_path>\n");
        return -1;
    }

    char* barcode_value = argv[1];
    char* endptr = NULL;
    long width_value = strtol(argv[2], &endptr, 10);
    if (argv[2][0] == '\0' || *endptr != '\0' || width_value <= 0) {
        printf("Invalid width value: %s\n", argv[2]);
        return -1;
    }

    long height_value = strtol(argv[3], &endptr, 10);
    if (argv[3][0] == '\0' || *endptr != '\0' || height_value <= 0) {
        printf("Invalid height value: %s\n", argv[3]);
        return -1;
    }

    char* output_filepath = argv[4];
    int width = (int) width_value;
    int height = (int) height_value;

    struct image* image = barcode(barcode_value, width, height);
    if (image == NULL) {
        printf("Failed to generate barcode image.\n");
        return -1;
    }

    if (saveimage(output_filepath, image) != 0) {
        printf("Failed to save image to %s\n", output_filepath);
        free(image->pixels);
        free(image);
        return -1;
    }

    free(image->pixels);
    free(image);
    return 0;
}
