#ifndef HEADERS_H
#include "headers.h"
#endif

#ifndef _CORRECT_GEOMETRY_H_
#define _CORRECT_GEOMETRY_H_

void correct_geometry_by_sampling_image_gradient(vectorizer_data * vd);
void initialise_geometry_correction_matrices(void);
void geometry_correction(vectorizer_data * vd);

#endif








