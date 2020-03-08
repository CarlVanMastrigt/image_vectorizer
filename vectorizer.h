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

#ifndef VECTORIZER_H
#define VECTORIZER_H


typedef struct vectorizer_data vectorizer_data;
typedef union point point;
typedef struct shape shape;
typedef struct face face;
typedef struct bezier bezier;



typedef struct pixel_lab
{
    float l;
    float a;
    float b;

    float c;///must be calculated from lab, cannot be summed like other components
}
pixel_lab;

typedef enum {NO_DIAGONAL,TR_BL_DIAGONAL,TL_BR_DIAGONAL,BOTH_DIAGONAL} diagonal_sample_connection_type;

typedef struct base_point
{
    vec2f position;
    vec2f old_position;
    vec2f modified_position;

    point ** block_ptr;
    point * next;///in block
    shape * parent_shape;

    uint32_t is_link_point:1;
    uint32_t fixed_horizontally:1;
    uint32_t fixed_vertically:1;
    uint32_t position_invalid:1;///does changed position (as opposed to old_pos) cause segments this line is part of to cross other segments
}
base_point;

typedef struct link_point ///first point reserved as empty
{
    base_point base;

    point * next;
    point * prev;

    face * reverse_face;
    face * regular_face;
}
link_point;

typedef struct node_connection node_connection;

struct node_connection ///first point reserved as empty
{
    uint32_t traversal_direction:1;///  1/0 == forwards/backwards
    uint32_t smoothable:1;
    uint32_t chain_filtered:1;///only used by high_pass_filter

    face * in_face;
    face * out_face;

    /// change to have both incoming and outgoing face_id (incoming and outgoing along ccw face loops)

    point * point_index;
    bezier * bezier_index;

    node_connection * next;
    point * parent_node;
};

typedef struct node_point
{
    base_point base;

    node_connection * first_connection;

    point * next;
    point * prev;
}
node_point;

union point
{
    base_point b;

    link_point l;
    node_point n;
};

struct shape
{
    face * perimeter_face;///make perimeter face first face always ??? if too difficult nvm
    face * first_face;///singly LL

    shape * parent;
    shape * next;
    shape * first_child;
};

struct face
{
    point * start_point;

    uint32_t perimeter:1;

    pixel_lab colour;

    shape * parent_shape;
    face * next_in_shape;

    face * encapsulating_face;/// make perimeter only ?
    face * next_excapsulated;///first encapsulated if not perimeter, next in LL if perimeter

    int32_t line_render_offset;///int not uint for GL compatibility, make GLint ?
    int32_t line_render_count;///make GLsizei ?

    face * next;
    face * prev;
};

struct bezier
{
    vec2f p1;///start_points
    vec2f p2;

    vec2f c1;///control_points
    vec2f c2;

    uint32_t num_sections;

    bezier * next;
    bezier * prev;

    bool straight_line;
};

typedef enum
{
    VECTORIZER_STAGE_START=0,
    VECTORIZER_STAGE_LOAD_IMAGE,
    VECTORIZER_STAGE_IMAGE_PROCESSING,/// seperate from following
    VECTORIZER_STAGE_GENERATE_GEOMETRY,
    VECTORIZER_STAGE_CONVERT_TO_BEZIER,/// seperate from prev
    VECTORIZER_STAGE_END
}
vectorizer_stage;

typedef enum
{
    VECTORIZER_RENDER_MODE_BLANK=0,
    VECTORIZER_RENDER_MODE_IMAGE,
    VECTORIZER_RENDER_MODE_BLURRED,
    VECTORIZER_RENDER_MODE_CLUSTERISED,
    VECTORIZER_RENDER_MODE_LINES,
    VECTORIZER_RENDER_MODE_CURVES,
    VECTORIZER_RENDER_NUM_MODES
}
vectorizer_render_mode;

struct vectorizer_data
{
    ///SETTINGS
    vectorizer_stage current_stage;
    vectorizer_stage variant_stage;

    int blur_factor;
    int cluster_combination_threshold;
    int valid_cluster_size_factor;
    bool correct_clusters;

    bool adjust_beziers;

    bool fix_exported_boundaries;

    int initial_geometry_correction_passes;


    int back_r;
    int back_g;
    int back_b;



    vectorizer_render_mode render_mode;
    vectorizer_render_mode max_render_mode;
    bool render_lines;
    bool can_render_lines;
    bool render_curves;
    bool can_render_curves;

    widget * render_mode_buttons[VECTORIZER_RENDER_NUM_MODES];
    widget * render_lines_checkbox_widget;
    widget * render_curves_checkbox_widget;


    ///DATA
    GLuint images[VECTORIZER_RENDER_NUM_MODES];

    int32_t w;
    int32_t h;

    int32_t texture_w;
    int32_t texture_h;

    pixel_lab * origional;
    pixel_lab * blurred;
    pixel_lab * clusterised;

    pixel_lab * h_grad;
    pixel_lab * v_grad;

    face ** pixel_faces;
    diagonal_sample_connection_type * diagonals;



    point * first_node;
    point * available_point;///rename to available_point

    shape * first_shape;///not LL circle so dummy is unneeded
    shape * available_shape;///rename to available_shape

    face * first_face;
    face * available_face;

    bezier * available_bezier;

    node_connection * available_node_connection;

    point ** blocks;///LL of points on a given pixel

    GLuint line_display_array;
    uint32_t display_line_count;

    GLuint point_display_array;
    uint32_t display_point_count;

    GLuint face_line_display_array;
};


vectorizer_data * create_vectorizer_data(void);
void delete_vectorizer_data(vectorizer_data * vd);


void test_for_invalid_nodes(vectorizer_data * vd);
void remove_all_redundant_nodes(vectorizer_data * vd);
void connect_point_to_last(vectorizer_data * vd,point * previous,point * current,face * f_face,face * b_face);///make connect_point_to_previous
point * segment_bisects_points(vectorizer_data * vd,point * p1,point * p2);
void try_to_convert_link_to_node(vectorizer_data * vd,point * p);
bool try_to_convert_node_to_link(vectorizer_data * vd,point * p);



point * create_link_point(vectorizer_data * vd,vec2f pos);
point * create_node_point(vectorizer_data * vd,vec2f pos);
void remove_point_from_image(vectorizer_data * vd,point * p);
point * remove_chain_from_image(vectorizer_data * vd,node_connection * start_connection);
void remove_all_points_from_image(vectorizer_data * vd);
void remove_all_bezier_curves_from_image(vectorizer_data * vd);

shape * create_shape(vectorizer_data * vd);
void remove_all_shapes_from_image(vectorizer_data * vd);

face * create_face(vectorizer_data * vd);
void add_face_to_shape(shape * s,face * f);
void remove_all_faces_from_image(vectorizer_data * vd);
void remove_all_perimeter_faces_from_image_and_reset_heirarchy_data(vectorizer_data * vd);

bezier * create_bezier(vectorizer_data * vd);

void reset_vectorizer_display_variables(vectorizer_data * vd);
void perform_requested_vectorizer_steps(vectorizer_data * vd,vectorizer_stage intended_stage);

void update_all_point_blocks(vectorizer_data * vd);
void store_point_old_positions(vectorizer_data * vd);
void update_point_positions(vectorizer_data * vd);

void prepare_all_point_positions_for_modification(vectorizer_data *vd);
void update_all_point_positions_from_modification(vectorizer_data *vd);


#include "vectorizing/image_processing.h"
#include "vectorizing/cluster_pixels.h"
#include "vectorizing/smoothing.h"
#include "vectorizing/correct_geometry.h"
#include "vectorizing/generate_geometry.h"
#include "vectorizing/shapes.h"
#include "vectorizing/bezier.h"
#include "vectorizing/export.h"



#endif





