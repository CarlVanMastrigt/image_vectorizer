#ifndef HEADERS_H
#include "headers.h"
#endif

#ifndef _IMAGE_PROCESSING_H_
#define _IMAGE_PROCESSING_H_

bool load_image(vectorizer_data * vd,char * filename);
void process_image(vectorizer_data * vd);

void upload_modified_image_colour(vectorizer_data * vd,pixel_lab * data,int image_id);

pixel_lab convert_rgb_to_cielab(uint8_t R,uint8_t G,uint8_t B);
void convert_cielab_to_rgb(pixel_lab lab,uint8_t * R,uint8_t * G,uint8_t * B);
void convert_cielab_to_rgb_float(pixel_lab lab,float * R,float * G,float * B);
uint32_t rgb_hash_from_lab(pixel_lab lab);
float cie00_difference(pixel_lab p1,pixel_lab p2);
vec3f convert_pixel_lab_to_vec3f(pixel_lab p);


static inline float cie00_difference_approximation_squared(pixel_lab p1,pixel_lab p2)
{
    float SC=1.0+0.0225*(p1.c + p2.c);
    return 0.29*((p2.l-p1.l)*(p2.l-p1.l) + (p2.c-p1.c)*(p2.c-p1.c)/(SC*SC));///0.29 is below the minimum ratio between this approximation and the actual cie00 difference such that this can pre-crop some expensive cie00 tests
}

static inline float cie76_difference_squared(pixel_lab p1,pixel_lab p2)
{
    return (p2.l-p1.l)*(p2.l-p1.l) + (p2.a-p1.a)*(p2.a-p1.a) + (p2.b-p1.b)*(p2.b-p1.b);
}

#endif






