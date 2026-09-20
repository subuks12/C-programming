#include "common.h"
#include "loader.h"
#include <stdio.h>
#include <time.h>


int filter1(struct image* image) {
    printf("running filter1\n");
   int counter = 0;
    
    for(int j=0; j< image->width; j++) {
        for(int i=0; i< image->height; i++) {
            if(image->pixels[i * image->width + j].r > image->pixels[i * image->width + j].g 
            &&  image->pixels[i * image->width + j].r > image->pixels[i * image->width + j].b) {
                counter += 1;
            }
        }
    }

    return counter;
}

int filter2(struct image* image) {
    printf("running filter2\n");
    int counter = 0;
    
    for(int i=0; i< image->height; i++) {
        for(int j=0; j< image->width; j++) {
            if(image->pixels[i * image->width + j].r > image->pixels[i * image->width + j].g 
            &&  image->pixels[i * image->width + j].r > image->pixels[i * image->width + j].b) {
                counter += 1;
            }
        }
    }

    return counter;
}

int main(int argv, char** argc) {
    if(argv != 5) {
        printf("Incorrect number of arguments. Expected: ./build/filter <input_image> <mode> <width> <height> \n");
        return -1;
    }

    char* filepath = argc[1];
    int index = atoi(argc[2]);
    int width = atoi(argc[3]);
    int height = atoi(argc[4]);
  
    struct image* image = malloc(sizeof(struct image));
    image->width = width;
    image->height = height;
    image->pixels = malloc(sizeof(struct pixel) * width * height);

    int err = loadimage(filepath, image);

    if(err == -1) {
        printf("ERROR loading image. Is your filepath correct?\n");
        return -1;
    }

    int res;
    struct timespec starttime;
    struct timespec endtime;

    clock_gettime(CLOCK_MONOTONIC, &starttime);

    if(index == 1) {
        res = filter1(image);
    } else {
        res = filter2(image);
    }

    clock_gettime(CLOCK_MONOTONIC, &endtime);

    // calculate the difference in nanoseconds between start time and end time
    double diff = ((double) 1000000000) * (endtime.tv_sec - starttime.tv_sec) + endtime.tv_nsec - starttime.tv_nsec;
	
    // print the difference in seconds
    printf("time = %lf seconds\n", (double) diff / ((double) 1000000000));


    free(image->pixels);
    free(image);

    printf("Result: %d\n",res);
}