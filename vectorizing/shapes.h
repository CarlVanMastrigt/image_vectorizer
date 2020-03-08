/**
Copyright 2020 Carl van Mastrigt

This file is part of image_vectorizer.

image_vectorizer is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

image_vectorizer is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with image_vectorizer.  If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef HEADERS_H
#include "headers.h"
#endif

#ifndef SHAPES_H
#define SHAPES_H

node_connection * get_next_ccw_connection(point * node,point * prev);
node_connection * get_face_start_connection(face * f);

void identify_shapes(vectorizer_data * vd,face * outermost_perimeter,point * outermost_shape_start_point);

void try_to_trace_all_faces(vectorizer_data * vd);

#endif









