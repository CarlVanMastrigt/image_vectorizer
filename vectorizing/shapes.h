#ifndef HEADERS_H
#include "headers.h"
#endif

#ifndef _SHAPES_H_
#define _SHAPES_H_

node_connection * get_next_ccw_connection(point * node,point * prev);
node_connection * get_face_start_connection(face * f);

void identify_shapes(vectorizer_data * vd,face * outermost_perimeter,point * outermost_shape_start_point);

void try_to_trace_all_faces(vectorizer_data * vd);

#endif









