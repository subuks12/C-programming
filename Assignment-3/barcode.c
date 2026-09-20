#include "barcode.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define WHITE 255
#define BLACK 0
#define QUIET_ZONE 9
#define DIGIT_WIDTH 7

static void set_bar(struct image* image, int x, int is_white) {
    if (x < 0 || x >= image->width) {
        return;
    }

    for (int y = 0; y < image->height; y++) {
        struct pixel* p = &image->pixels[y * image->width + x];
        if (is_white) {
            p->r = WHITE;
            p->g = WHITE;
            p->b = WHITE;
        } else {
            p->r = BLACK;
            p->g = BLACK;
            p->b = BLACK;
        }
    }
}

static int get_bit(int pattern, int pos) {
    return (pattern >> (DIGIT_WIDTH - 1 - pos)) & 1;
}

static int left_digit_pattern(char digit) {
    switch (digit) {
        case '0': return 0x72;
        case '1': return 0x66;
        case '2': return 0x6C;
        case '3': return 0x50;
        case '4': return 0x5C;
        case '5': return 0x4E;
        case '6': return 0x42;
        case '7': return 0x44;
        case '8': return 0x48;
        case '9': return 0x74;
        default: return 0;
    }
}

static int right_digit_pattern(char digit) {
    return (~left_digit_pattern(digit)) & 0x7F;
}

struct image* barcode(char* data, int width, int height) {
    if (data == NULL || width <= 0 || height <= 0) {
        return NULL;
    }

    size_t len = strlen(data);
    if (len != 12) {
        return NULL;
    }

    struct image* image = malloc(sizeof(struct image));
    if (image == NULL) {
        return NULL;
    }

    image->width = width;
    image->height = height;
    image->pixels = malloc(sizeof(struct pixel) * (size_t)width * (size_t)height);
    if (image->pixels == NULL) {
        free(image);
        return NULL;
    }

    for (int i = 0; i < width * height; i++) {
        image->pixels[i].r = WHITE;
        image->pixels[i].g = WHITE;
        image->pixels[i].b = WHITE;
    }

    int x = 0;
    const int expected_pattern_width = QUIET_ZONE * 2 + 3 + 6 * DIGIT_WIDTH + 5 + 6 * DIGIT_WIDTH + 3;
    if (width < expected_pattern_width) {
        free(image->pixels);
        free(image);
        return NULL;
    }

    for (int i = 0; i < QUIET_ZONE; i++) {
        set_bar(image, x++, 1);
    }

    const int start_bits[3] = {0, 1, 0};
    for (int i = 0; i < 3; i++) {
        set_bar(image, x++, start_bits[i] == 1);
    }

    for (int i = 0; i < 6; i++) {
        int pattern = left_digit_pattern(data[i]);
        for (int bit = 0; bit < DIGIT_WIDTH; bit++) {
            set_bar(image, x++, get_bit(pattern, bit) == 1);
        }
    }

    const int middle_bits[5] = {1, 0, 1, 0, 1};
    for (int i = 0; i < 5; i++) {
        set_bar(image, x++, middle_bits[i] == 1);
    }

    for (int i = 6; i < 12; i++) {
        int pattern = right_digit_pattern(data[i]);
        for (int bit = 0; bit < DIGIT_WIDTH; bit++) {
            set_bar(image, x++, get_bit(pattern, bit) == 1);
        }
    }

    const int end_bits[3] = {0, 1, 0};
    for (int i = 0; i < 3; i++) {
        set_bar(image, x++, end_bits[i] == 1);
    }

    for (int i = 0; i < QUIET_ZONE; i++) {
        set_bar(image, x++, 1);
    }

    return image;
}

