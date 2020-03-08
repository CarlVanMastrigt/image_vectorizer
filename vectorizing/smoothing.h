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

#ifndef SMOOTHING_H
#define SMOOTHING_H

void low_pass_filter_geometry(vectorizer_data * vd);
void adjust_segment_lengths(vectorizer_data * vd);
void set_connections_smoothable_status(vectorizer_data * vd);
void find_smoothest_node_connections(point * node,node_connection ** best_nc1,node_connection ** best_nc2);
void relocate_loop_nodes_to_flattest_spot(vectorizer_data * vd);

#endif







