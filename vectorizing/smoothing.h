#ifndef HEADERS_H
#include "headers.h"
#endif

#ifndef _SMOOTHING_H_
#define _SMOOTHING_H_

void low_pass_filter_geometry(vectorizer_data * vd);
void adjust_segment_lengths(vectorizer_data * vd);
void set_connections_smoothable_status(vectorizer_data * vd);
void find_smoothest_node_connections(point * node,node_connection ** best_nc1,node_connection ** best_nc2);
void relocate_loop_nodes_to_flattest_spot(vectorizer_data * vd);

#endif







