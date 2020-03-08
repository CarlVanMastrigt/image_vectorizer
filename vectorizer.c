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

#include "headers.h"


///for testing all malloced memory is freed
static uint32_t test_connection_count=0;
static uint32_t test_point_count=0;
static uint32_t test_face_count=0;
static uint32_t test_shape_count=0;
static uint32_t test_bezier_count=0;
static uint32_t test_colour_count=0;
static uint32_t test_adjacent_count=0;

vectorizer_data * create_vectorizer_data(void)
{
    initialise_geometry_correction_matrices();///has to be called before use, appropriate to call as part of general data initialisation (this func)

    vectorizer_data * vd=malloc(sizeof(vectorizer_data));
    int i;

    vd->blur_factor=0;
    vd->cluster_combination_threshold=8;
    vd->valid_cluster_size_factor=20;
    vd->correct_clusters=false;

    vd->fix_exported_boundaries=true;
    vd->adjust_beziers=false;


    vd->initial_geometry_correction_passes=8;


    vd->back_r=255;
    vd->back_g=255;
    vd->back_b=255;


    vd->current_stage=VECTORIZER_STAGE_START;
    vd->variant_stage=VECTORIZER_STAGE_START;


    vd->render_mode=vd->max_render_mode=VECTORIZER_RENDER_MODE_BLANK;
    vd->render_lines=true;
    vd->can_render_lines=false;
    vd->render_curves=true;
    vd->can_render_curves=false;


    glGenTextures(10,vd->images);

    for(i=0;i<10;i++)
    {
        glBindTexture(GL_TEXTURE_2D,vd->images[i]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glBindTexture(GL_TEXTURE_2D,0);
    }

    vd->h=0;
    vd->w=0;

    vd->blocks=NULL;

    vd->origional=NULL;
    vd->blurred=NULL;
    vd->clusterised=NULL;

    vd->pixel_faces=NULL;
    vd->diagonals=NULL;

    vd->h_grad=NULL;
    vd->v_grad=NULL;



    ///initialise point structure base
    vd->available_point=NULL;
    vd->first_node=NULL;

    ///initialise shape structure base
    vd->available_shape=NULL;
    vd->first_shape=NULL;


    vd->available_bezier=NULL;


    vd->available_node_connection=NULL;

    vd->available_face=NULL;
    vd->first_face=NULL;

    vd->display_line_count=0;
    vd->line_display_array=0;
    glGenBuffers_ptr(1, &vd->line_display_array);


    vd->display_point_count=0;
    vd->point_display_array=0;
    glGenBuffers_ptr(1, &vd->point_display_array);

    vd->face_line_display_array=0;
    glGenBuffers_ptr(1, &vd->face_line_display_array);

    return vd;
}

void delete_vectorizer_data(vectorizer_data * vd)
{
    remove_all_points_from_image(vd);
    remove_all_faces_from_image(vd);
    remove_all_shapes_from_image(vd);

    void * ptr;

    while(vd->available_point)
    {
        test_point_count--;
        ptr=vd->available_point;
        vd->available_point=vd->available_point->b.next;
        free(ptr);
    }

    while(vd->available_node_connection)
    {
        test_connection_count--;
        ptr=vd->available_node_connection;
        vd->available_node_connection=vd->available_node_connection->next;
        free(ptr);
    }

    while(vd->available_bezier)
    {
        test_bezier_count--;
        ptr=vd->available_bezier;
        vd->available_bezier=vd->available_bezier->next;
        free(ptr);
    }

    while(vd->available_face)
    {
        test_face_count--;
        ptr=vd->available_face;
        vd->available_face=vd->available_face->next;
        free(ptr);
    }

    while(vd->available_shape)
    {
        test_shape_count--;
        ptr=vd->available_shape;
        vd->available_shape=vd->available_shape->next;
        free(ptr);
    }

    printf("remaining points: %u\n",test_point_count);
    printf("remaining connections: %u\n",test_connection_count);
    printf("remaining faces: %u\n",test_face_count);
    printf("remaining shapes: %u\n",test_shape_count);
    printf("remaining colours: %u\n",test_colour_count);
    printf("remaining beziers: %u\n",test_bezier_count);
    printf("remaining adjacents: %u\n",test_adjacent_count);



    free(vd->blocks);

    free(vd->origional);
    free(vd->blurred);
    free(vd->clusterised);

    free(vd->pixel_faces);
    free(vd->diagonals);

    free(vd->h_grad);
    free(vd->v_grad);

    #warning clear/free opengl textures

    free(vd);
}

static point * create_point(vectorizer_data * vd,vec2f pos)
{
    #warning move block pointer finding to another function so it can be shared w/ update_point_positions

    point *p,**block_ptr;

    int32_t lix,liy;

    lix=(int32_t)pos.x;
    liy=(int32_t)pos.y;

    if(lix<0)lix=0;
    if(liy<0)liy=0;
    if(lix>=vd->w)lix=vd->w-1;
    if(liy>=vd->h)liy=vd->h-1;

    if(vd->available_point)
    {
        p=vd->available_point;
        vd->available_point=vd->available_point->b.next;
    }
    else
    {
        test_point_count++;
        p=malloc(sizeof(point));
    }

    block_ptr=vd->blocks+(liy*vd->w+lix);

    ///set point_data
    p->b.position=pos;
    p->b.block_ptr=block_ptr;

    p->b.parent_shape=NULL;

    p->b.fixed_horizontally=false;
    p->b.fixed_vertically=false;
    p->b.position_invalid=false;


    ///add to block
    p->b.next = *block_ptr;
    *block_ptr = p;

    return p;
}

static void initialise_link_point(point * p)
{
    p->b.is_link_point=true;

    p->l.prev=NULL;
    p->l.next=NULL;
    p->l.regular_face=NULL;
    p->l.reverse_face=NULL;
}

point * create_link_point(vectorizer_data * vd,vec2f pos)
{
    point * p=create_point(vd,pos);
    initialise_link_point(p);

    return p;
}

static void initialise_node_point(vectorizer_data * vd,point * p)
{
    p->b.is_link_point=false;
    p->n.first_connection=NULL;

    p->n.next=vd->first_node;
    p->n.prev=NULL;

    if(vd->first_node)vd->first_node->n.prev=p;

    vd->first_node=p;
}

point * create_node_point(vectorizer_data * vd,vec2f pos)
{
    point * p=create_point(vd,pos);
    initialise_node_point(vd,p);

    return p;
}


static void add_connection_to_node(vectorizer_data * vd,point * node,point * connected_point,uint32_t traversal_direction,face * f_face,face * b_face)
{
    node_connection * nc;

    if(vd->available_node_connection)
    {
        nc=vd->available_node_connection;
        vd->available_node_connection=vd->available_node_connection->next;
    }
    else
    {
        test_connection_count++;
        nc=malloc(sizeof(node_connection));
    }

    nc->point_index=connected_point;
    nc->traversal_direction=traversal_direction;

    nc->in_face=traversal_direction?b_face:f_face;
    nc->out_face=traversal_direction?f_face:b_face;

    nc->parent_node=node;

    nc->bezier_index=NULL;

    nc->next=node->n.first_connection;
    node->n.first_connection=nc;
}

static void remove_node_from_linked_list(vectorizer_data * vd,point * p)
{
    if(p->n.next)p->n.next->n.prev=p->n.prev;
    if(p->n.prev)p->n.prev->n.next=p->n.next;
    else vd->first_node=p->n.next;
}



void try_to_convert_link_to_node(vectorizer_data * vd,point * p)
{
    if(p->b.is_link_point)
    {
        ///convert p_existing to node_point
        link_point old=p->l;

        initialise_node_point(vd,p);

        add_connection_to_node(vd,p,old.next,1,old.regular_face,old.reverse_face);
        add_connection_to_node(vd,p,old.prev,0,old.regular_face,old.reverse_face);
    }
}

bool try_to_convert_node_to_link(vectorizer_data * vd,point * p)
{
    node_connection * current_connection;
    face * tmp_face;
    point *prev,*tmp_p,*current;
    node_connection con_0,con_1;


    if((p->n.first_connection==NULL)||(p->n.first_connection->next==NULL)||(p->n.first_connection->next->next))/// ( last test is != ) , checks that node has exactly 2 connections ( || is sequence point )
    {
        return false;///node does not have correct number of connections
    }

    if(p->n.first_connection->traversal_direction != p->n.first_connection->next->traversal_direction)///first and second connections have different traversal_direction (requirement of loop)
    {
        if(p->n.first_connection->traversal_direction) current=p->n.first_connection->point_index;///first connection
        else current=p->n.first_connection->next->point_index;///second connection

        while(current->b.is_link_point) current=current->l.next;

        if(current==p) return false;///trying to remove only node in loop,ergo fail
    }

    remove_node_from_linked_list(vd,p);

    con_0= *p->n.first_connection;/// maintain con_0 directionality
    con_1= *p->n.first_connection->next;

    /// remove both connections
    p->n.first_connection->next->next=vd->available_node_connection;
    vd->available_node_connection=p->n.first_connection;


    initialise_link_point(p);

    if(con_0.traversal_direction)
    {
        p->l.next=con_0.point_index;
        p->l.regular_face=con_0.out_face;
        p->l.prev=con_1.point_index;
        p->l.reverse_face=con_1.out_face;
    }
    else
    {
        p->l.next=con_1.point_index;
        p->l.regular_face=con_1.out_face;
        p->l.prev=con_0.point_index;
        p->l.reverse_face=con_0.out_face;
    }

    if(con_0.traversal_direction == con_1.traversal_direction)///convert rest of chain if necessary
    {
        current=con_1.point_index;
        prev=p;

        while(current->b.is_link_point)
        {
            tmp_p=current->l.next;
            current->l.next=current->l.prev;
            current->l.prev=tmp_p;

            tmp_face=current->l.regular_face;///=========
            current->l.regular_face=current->l.reverse_face;///=========
            current->l.reverse_face=tmp_face;///=========

            prev=current;

            if(con_0.traversal_direction) current=current->l.prev;///this should be correct (remember prev & next were just switched), going backwards from node that was converted
            else current=current->l.next;
        }

        for(current_connection=current->n.first_connection;current_connection;current_connection=current_connection->next)
        {
            if(current_connection->point_index==prev)
            {
                current_connection->traversal_direction^=1;

                break;
            }
        }
    }

    return true;
}


void connect_point_to_last(vectorizer_data * vd,point * previous,point * current,face * f_face,face * b_face)///assumes traversal_direction of 1
{
    if(previous->b.is_link_point)
    {
        previous->l.next=current;
        previous->l.regular_face=f_face;
    }
    else add_connection_to_node(vd,previous,current,1,f_face,b_face);


    if(current->b.is_link_point)
    {
        current->l.prev=previous;
        current->l.reverse_face=b_face;
    }
    else add_connection_to_node(vd,current,previous,0,f_face,b_face);
}


shape * create_shape(vectorizer_data * vd)
{
    shape * s;

    if(vd->available_shape)
    {
        s=vd->available_shape;
        vd->available_shape=vd->available_shape->next;
    }
    else
    {
        test_shape_count++;
        s=malloc(sizeof(shape));
    }

    s->perimeter_face=NULL;
    s->first_face=NULL;

    s->first_child=NULL;
    s->parent=NULL;

    s->next=NULL;

    return s;
}

void remove_all_shapes_from_image(vectorizer_data * vd)
{
    shape *current_shape,*prev_shape;
    bool has_valid_children;

    current_shape=vd->first_shape;
    prev_shape=NULL;

    while(current_shape)
    {
        has_valid_children=((current_shape->parent==prev_shape)  ||  ((prev_shape)&&(prev_shape->next==current_shape))) && (current_shape->first_child);///coming from parent or sibling and has children

        if(current_shape->parent!=prev_shape)///add prev to available, only occurs if coming from sibling or child (already removed nodes), not if coming from parent (yet to be removed)
        {
            prev_shape->next=vd->available_shape;
            vd->available_shape=prev_shape;
        }

        prev_shape=current_shape;
        if(has_valid_children)current_shape=current_shape->first_child;
        else if(current_shape->next) current_shape=current_shape->next;
        else current_shape=current_shape->parent;
    }

    if(prev_shape)///if only 1 shape/last shape
    {
        prev_shape->next=vd->available_shape;
        vd->available_shape=prev_shape;
    }

    vd->first_shape=NULL;
}

face * create_face(vectorizer_data * vd)
{
    face * f;
    if(vd->available_face)
    {
        f=vd->available_face;
        vd->available_face=vd->available_face->next;
    }
    else
    {
        test_face_count++;
        f=malloc(sizeof(face));
    }

    f->start_point=NULL;
    f->perimeter=false;

    f->next=vd->first_face;
    f->prev=NULL;

    if(vd->first_face)vd->first_face->prev=f;
    vd->first_face=f;

    f->encapsulating_face=NULL;
    f->next_excapsulated=NULL;

    f->parent_shape=NULL;
    f->next_in_shape=NULL;

    f->line_render_count=0;
    f->line_render_offset=0;

    return f;
}


void add_face_to_shape(shape * s,face * f)
{
    f->parent_shape=s;
    f->next_in_shape=s->first_face;

    s->first_face=f;
}

void remove_all_faces_from_image(vectorizer_data * vd)
{
    face *f;

    while(vd->first_face)
    {
        f=vd->first_face;
        vd->first_face=f->next;

        f->next=vd->available_face;
        vd->available_face=f;
    }
}

void remove_all_perimeter_faces_from_image_and_reset_heirarchy_data(vectorizer_data * vd)
{
    face *f,*n;

    for(f=vd->first_face;f;f=n)
    {
        n=f->next;

        f->start_point=NULL;

        f->encapsulating_face=NULL;
        f->next_excapsulated=NULL;

        f->next_in_shape=NULL;
        f->parent_shape=NULL;

        f->line_render_count=0;
        f->line_render_offset=0;

        if(f->perimeter)///remove
        {
            if(f->next)f->next->prev=f->prev;
            if(f->prev)f->prev->next=f->next;
            else vd->first_face=f->next;

            f->prev=NULL;
            f->next=vd->available_face;
            vd->available_face=f;
        }
    }
}


bezier * create_bezier(vectorizer_data * vd)
{
    bezier * b;

    if(vd->available_bezier)
    {
        b=vd->available_bezier;
        vd->available_bezier=vd->available_bezier->next;
    }
    else
    {
        test_bezier_count++;
        b=malloc(sizeof(bezier));
    }

    return b;
}



void remove_point_from_image(vectorizer_data * vd,point * p)
{
    point ** next_link=p->b.block_ptr;

    while((*next_link) != p) next_link = &((*next_link)->b.next);
    *next_link = p->b.next;

    if(!p->b.is_link_point) remove_node_from_linked_list(vd,p);

    p->b.next=vd->available_point;
    vd->available_point=p;
}

point * remove_chain_from_image(vectorizer_data * vd,node_connection * start_connection)///returns index of node at end of chain
{
    point *prev_point,*current_point;
    node_connection *end_connection,**next_connection_pointer;
    bezier *next_bezier,*current_bezier;


    ///remove beziers (if they have been created) first
    for(current_bezier=start_connection->bezier_index;current_bezier;current_bezier=next_bezier)
    {
        if(start_connection->traversal_direction) next_bezier=current_bezier->next;
        else next_bezier=current_bezier->prev;

        current_bezier->next=vd->available_bezier;
        vd->available_bezier=current_bezier;
    }


    next_connection_pointer= &start_connection->parent_node->n.first_connection;
    while(*next_connection_pointer != start_connection) next_connection_pointer= &((*next_connection_pointer)->next);
    *next_connection_pointer=start_connection->next;


    prev_point=start_connection->parent_node;
    current_point=start_connection->point_index;

    while(current_point->b.is_link_point)
    {
        prev_point=current_point;

        if(start_connection->traversal_direction) current_point=current_point->l.next;
        else current_point=current_point->l.prev;

        remove_point_from_image(vd,prev_point);
    }


    next_connection_pointer= &current_point->n.first_connection;
    while((*next_connection_pointer)->point_index != prev_point) next_connection_pointer= &((*next_connection_pointer)->next);
    end_connection= *next_connection_pointer;
    *next_connection_pointer=end_connection->next;

    /// add start_connection and end_connection to available_connection LL
    end_connection->next=vd->available_node_connection;
    start_connection->next=end_connection;
    vd->available_node_connection=start_connection;

    return current_point;
}

void remove_all_points_from_image(vectorizer_data * vd)
{
    while(vd->first_node)
    {
        while(vd->first_node->n.first_connection)
        {
            remove_chain_from_image(vd,vd->first_node->n.first_connection);
        }

        remove_point_from_image(vd,vd->first_node);
    }
}


void remove_all_bezier_curves_from_image(vectorizer_data * vd)
{
    point *current_node;
    node_connection *current_connection;
    bezier *next_bezier,*current_bezier;

    for(current_node=vd->first_node;current_node;current_node=current_node->n.next)
    {
        for(current_connection=current_node->n.first_connection;current_connection;current_connection=current_connection->next)
        {
            if(current_connection->traversal_direction) for(current_bezier=current_connection->bezier_index;current_bezier;current_bezier=next_bezier)
            {
                next_bezier=current_bezier->next;

                current_bezier->next=vd->available_bezier;
                vd->available_bezier=current_bezier;
            }

            current_connection->bezier_index=NULL;
        }
    }
}


void test_for_invalid_nodes(vectorizer_data * vd)
{
    point *current_node;

    for(current_node=vd->first_node;current_node;current_node=current_node->n.next)
    {
        if(current_node->n.first_connection==NULL)puts("ERROR EMPTY NODE");//remove_point_from_image(vd,current_node);///node has no connections
        if(current_node->n.first_connection->next==NULL)puts("ERROR SINGULAR NODE");
    }
}

void remove_all_redundant_nodes(vectorizer_data * vd)
{
    point *current_node,*next_node;

    for(current_node=vd->first_node;current_node;current_node=next_node)
    {
        next_node=current_node->n.next;///node may be deleted or changed to link so store this now
        try_to_convert_node_to_link(vd,current_node);
    }
}





static void update_point_block(vectorizer_data * vd,point * p)
{
    point **new_block_ptr,**ptr_to;
    int32_t lix,liy;

    lix=(int32_t)p->b.position.x;
    liy=(int32_t)p->b.position.y;

    if(lix<0)lix=0;
    if(liy<0)liy=0;
    if(lix>=vd->w)lix=vd->w-1;
    if(liy>=vd->h)liy=vd->h-1;

    new_block_ptr=vd->blocks+(liy*vd->w+lix);
    ptr_to=p->b.block_ptr;

    if(new_block_ptr != ptr_to)
    {
        while(*ptr_to != p)
        {
            if(*ptr_to==NULL)puts("ERROR POINT BEING REMOVED FROM BLOCK IT IS NOT IN !S");
             ptr_to= &((*ptr_to)->b.next);
        }

        *ptr_to=p->b.next;

        p->b.next= *new_block_ptr;
        *new_block_ptr=p;
        p->b.block_ptr=new_block_ptr;
    }
}

void update_all_point_blocks(vectorizer_data * vd)
{
    point *current_node,*current_point;
    node_connection * current_connection;

    for(current_node=vd->first_node;current_node;current_node=current_node->n.next)/// recalculate block for all points and move between as appropriate
    {
        update_point_block(vd,current_node);

        for(current_connection=current_node->n.first_connection;current_connection;current_connection=current_connection->next)
        {
            if(current_connection->traversal_direction)for(current_point=current_connection->point_index;current_point->b.is_link_point;current_point=current_point->l.next)
            {
                update_point_block(vd,current_point);
            }
        }
    }
}



void store_point_old_positions(vectorizer_data * vd)
{
    point *current_node,*current_point;
    node_connection * current_connection;

    for(current_node=vd->first_node;current_node;current_node=current_node->n.next)
    {
        current_node->b.old_position=current_node->b.position;

        for(current_connection=current_node->n.first_connection;current_connection;current_connection=current_connection->next)
        {
            if(current_connection->traversal_direction)for(current_point=current_connection->point_index;current_point->b.is_link_point;current_point=current_point->l.next)
            {
                current_point->b.old_position=current_point->b.position;
            }
        }
    }
}

static inline bool point_within_distance_of_segment(vec2f p1,vec2f p2,vec2f p_test,float distance_squared)
{
    vec2f v=v2f_sub(p2,p1);
    vec2f vt=v2f_sub(p_test,p1);
    float dp=v2f_dot(v,vt);

    if((dp<0.0)||(v2f_dot(v,v2f_sub(p_test,p2))>0.0))return false;

    return ((v2f_mag_sq(vt)-dp*dp/v2f_mag_sq(v)) < distance_squared);
}

static inline bool nearby_segment_crossed(vec2f p1,vec2f p2,point ** np)///checks if segment formed by p1-p2 is crossed by any nearby segments and marks every point involved as invalid if it is
{
    vec2f orth_a,orth_b;
    node_connection * current_connection;

    orth_a=v2f_orth(v2f_sub(p2,p1));

    ///need to check that p2 != *np ? (technically not as dp would be 0.0 (and thus not <0.0) but still might be a good idea ?)

    while(*np)
    {
        if((*np)->b.is_link_point)
        {
            if( (v2f_dot(orth_a,v2f_sub((*np)->b.position,p1))*v2f_dot(orth_a,v2f_sub((*np)->l.next->b.position,p1))) < 0.0)
            {
                orth_b=v2f_orth(v2f_sub((*np)->l.next->b.position,(*np)->b.position));

                if( (v2f_dot(orth_b,v2f_sub(p1,(*np)->b.position))*v2f_dot(orth_b,v2f_sub(p2,(*np)->b.position))) < 0.0) return true;
            }
        }
        else
        {
            for(current_connection=(*np)->n.first_connection;current_connection;current_connection=current_connection->next)
            if((current_connection->traversal_direction)&&( (v2f_dot(orth_a,v2f_sub((*np)->b.position,p1))*v2f_dot(orth_a,v2f_sub(current_connection->point_index->b.position,p1))) < 0.0))
            {
                orth_b=v2f_orth(v2f_sub(current_connection->point_index->b.position,(*np)->b.position));

                if( (v2f_dot(orth_b,v2f_sub(p1,(*np)->b.position))*v2f_dot(orth_b,v2f_sub(p2,(*np)->b.position))) < 0.0) return true;
            }
        }

        np++;
    }

    return false;
}

static void get_nearby_points(vectorizer_data * vd,point * p_base,point ** nearby_point_buffer)
{
    const int32_t block_search_range=5;///enough? 5/6?
    vec2i min,max,l;
    point *p;
    uint32_t nearby_point_count=0;

    l=v2f_to_v2i(p_base->b.position);

    min=v2i_max(v2i_sub(l,(vec2i){block_search_range,block_search_range}),(vec2i){0,0});
    max=v2i_min(v2i_add(l,(vec2i){block_search_range,block_search_range}),(vec2i){.x=vd->w-1,.y=vd->h-1});

    for(l.x=min.x;l.x<=max.x;l.x++)for(l.y=min.y;l.y<=max.y;l.y++)
    {
        for(p=vd->blocks[l.y*vd->w+l.x];p;p=p->b.next)
        {
            if(p!=p_base)nearby_point_buffer[nearby_point_count++]=p;
        }
    }

    nearby_point_buffer[nearby_point_count]=NULL;
}

static bool test_if_point_position_invalid(vectorizer_data * vd,point * p,point ** nearby_point_buffer)
{
    //const float er_2=0.0025;///exclusion range squared
    const float er_2=0.01;///exclusion range squared

    point ** np;
    node_connection * current_connection;

    get_nearby_points(vd,p,nearby_point_buffer);
    vec2f p_pos,pn_pos,pp_pos;

    p_pos=p->b.position;

    ///need to test all segments connected, as this function needs to completely test validity of this point
    ///thus it isnt necessary to set other points involved in interaction and can benefit from performance increase of using vec2f w/o dereferences
    ///especially given very few points will (ordinarily) be invalid so operations saved by marking other points invalid (and thus not needing to test their validity later) will be minimal

    ///test if point is too close to any nearby points
    for(np=nearby_point_buffer;*np;np++)if(v2f_dist_sq(p_pos,(*np)->b.position)<er_2) return true;

    ///test if point is too close to any nearby points segments
    for(np=nearby_point_buffer;*np;np++)
    {
        if((*np)->b.is_link_point)
        {
            if((p!=(*np)->l.next) && (point_within_distance_of_segment((*np)->b.position,(*np)->l.next->b.position,p_pos,er_2)))return true;
        }
        else
        {
            for(current_connection=(*np)->n.first_connection;current_connection;current_connection=current_connection->next)
            {
                if((p!=current_connection->point_index) && (current_connection->traversal_direction) && (point_within_distance_of_segment((*np)->b.position,current_connection->point_index->b.position,p_pos,er_2)))return true;
            }
        }
    }

    ///test segments this point is part of for: points that are too close AND overlap with other segments
    if(p->b.is_link_point)
    {
        pn_pos=p->l.next->b.position;
        pp_pos=p->l.prev->b.position;

        for(np=nearby_point_buffer;*np;np++)
        {
            if((*np != p->l.next) && (point_within_distance_of_segment(p_pos,pn_pos,(*np)->b.position,er_2)))return true;
            if((*np != p->l.prev) && (point_within_distance_of_segment(p_pos,pp_pos,(*np)->b.position,er_2)))return true;
        }

        if(nearby_segment_crossed(p_pos,pn_pos,nearby_point_buffer))return true;
        if(nearby_segment_crossed(p_pos,pp_pos,nearby_point_buffer))return true;
    }
    else
    {
        for(current_connection=p->n.first_connection;current_connection;current_connection=current_connection->next)
        {
            pn_pos=current_connection->point_index->b.position;

            for(np=nearby_point_buffer;*np;np++) if((*np != current_connection->point_index)&&(point_within_distance_of_segment(p_pos,pn_pos,(*np)->b.position,er_2))) return true;

            if(nearby_segment_crossed(p_pos,pn_pos,nearby_point_buffer))return true;
        }
    }

    return false;
}

void update_point_positions(vectorizer_data * vd)///revert positions if invalid, moves "block" as appropriate and (potentially) enforcing fixed horizontally & vertically conditions
{
    point *current_node,*current_point,**nearby_point_buffer;
    node_connection * current_connection;
    bool any_invalid;
    int revert_count;

    vec2f min,max;
    min=(vec2f){0.0,0.0};
    max=(vec2f){.x=(float)vd->w,.y=(float)vd->h};

    nearby_point_buffer=malloc(sizeof(point*)*4096);///allows space for 50 points per pixel (assuming block_search_range is 4), should be more than enough

    for(current_node=vd->first_node;current_node;current_node=current_node->n.next)///enforce fixed horizontally & vertically conditions and limit to image bounds
    {
        if(current_node->b.fixed_horizontally)current_node->b.position.x=current_node->b.old_position.x;
        if(current_node->b.fixed_vertically)current_node->b.position.y=current_node->b.old_position.y;
        current_node->b.position=v2f_clamp(current_node->b.position,min,max);

        for(current_connection=current_node->n.first_connection;current_connection;current_connection=current_connection->next)
        {
            if(current_connection->traversal_direction)
            {
                for(current_point=current_connection->point_index;current_point->b.is_link_point;current_point=current_point->l.next)
                {
                    if(current_point->b.fixed_horizontally)current_point->b.position.x=current_point->b.old_position.x;
                    if(current_point->b.fixed_vertically)current_point->b.position.y=current_point->b.old_position.y;
                    current_point->b.position=v2f_clamp(current_point->b.position,min,max);
                }
            }
        }
    }

    revert_count=0;

    uint32_t tt=SDL_GetTicks();

    do ///gradually revert all "broken" points (those that form part of segments that cross other segments) by gradually approaching old_position (currently just automatically fully reverts)
    {
        any_invalid=false;

        for(current_node=vd->first_node;current_node;current_node=current_node->n.next)
        {
            any_invalid |= current_node->b.position_invalid |= test_if_point_position_invalid(vd,current_node,nearby_point_buffer);

            for(current_connection=current_node->n.first_connection;current_connection;current_connection=current_connection->next)
            {
                if(current_connection->traversal_direction)for(current_point=current_connection->point_index;current_point->b.is_link_point;current_point=current_point->l.next)
                {
                    any_invalid |= current_point->b.position_invalid = test_if_point_position_invalid(vd,current_point,nearby_point_buffer);
                }
            }
        }

        //if(any_invalid)exit(7);

        for(current_node=vd->first_node;current_node;current_node=current_node->n.next)
        {
            if(current_node->b.position_invalid)
            {
                current_node->b.position_invalid=false;
                current_node->b.position=current_node->b.old_position;
                ///current_node->b.position=vec2f_subdivide_vectors(current_node->b.position,current_node->b.old_position);
            }

            for(current_connection=current_node->n.first_connection;current_connection;current_connection=current_connection->next)
            {
                if(current_connection->traversal_direction)for(current_point=current_connection->point_index;current_point->b.is_link_point;current_point=current_point->l.next)
                {
                    if(current_point->b.position_invalid)
                    {
                        current_point->b.position_invalid=false;
                        current_point->b.position=current_point->b.old_position;
                        ///current_point->b.position=vec2f_subdivide_vectors(current_point->b.position,current_point->b.old_position);
                    }
                }
            }
        }

        if(any_invalid)revert_count++;
    }
    while(any_invalid);

    printf("revert test time: %d\n",SDL_GetTicks()-tt);

    printf("revert count: %d\n",revert_count);

    update_all_point_blocks(vd);

    free(nearby_point_buffer);
}

void prepare_all_point_positions_for_modification(vectorizer_data *vd)
{
    point *current_point,*current_node;
    node_connection *current_connection;

    for(current_node=vd->first_node;current_node;current_node=current_node->n.next)
    {
        current_node->b.modified_position=current_node->b.position;
        for(current_connection=current_node->n.first_connection;current_connection;current_connection=current_connection->next)
        {
            if(current_connection->traversal_direction)for(current_point=current_connection->point_index;current_point->b.is_link_point;current_point=current_point->l.next)
            {
                current_point->b.modified_position=current_point->b.position;
            }
        }
    }
}

void update_all_point_positions_from_modification(vectorizer_data *vd)
{
    point *current_point,*current_node;
    node_connection *current_connection;

    for(current_node=vd->first_node;current_node;current_node=current_node->n.next)
    {
        if( ! current_node->b.fixed_horizontally)current_node->b.position.x=current_node->b.modified_position.x;
        if( ! current_node->b.fixed_vertically)current_node->b.position.y=current_node->b.modified_position.y;

        for(current_connection=current_node->n.first_connection;current_connection;current_connection=current_connection->next)
        {
            if(current_connection->traversal_direction)for(current_point=current_connection->point_index;current_point->b.is_link_point;current_point=current_point->l.next)
            {
                if( ! current_point->b.fixed_horizontally)current_point->b.position.x=current_point->b.modified_position.x;
                if( ! current_point->b.fixed_vertically)current_point->b.position.y=current_point->b.modified_position.y;
            }
        }
    }
}

void reset_vectorizer_display_variables(vectorizer_data * vd)
{
    int i;

    for(i=1;i<=vd->max_render_mode;i++)vd->render_mode_buttons[i]->base.status&= ~WIDGET_ACTIVE;
    vd->render_mode_buttons[1]->base.status|= WIDGET_ACTIVE;

    vd->render_lines_checkbox_widget->base.status&= ~WIDGET_ACTIVE;
    vd->render_curves_checkbox_widget->base.status&= ~WIDGET_ACTIVE;

    vd->render_mode=vd->max_render_mode=VECTORIZER_RENDER_MODE_IMAGE;
    vd->can_render_curves=false;
    vd->can_render_lines=false;
}

static void upload_vector_line_render_data(vectorizer_data * vd)
{
    point *current_node,*current,*prev;
    node_connection * current_connection;

    float wf=1.0/((float)vd->w);
    float hf=1.0/((float)vd->h);

    uint32_t point_space=2;
    GLfloat * display_point_buffer=malloc(sizeof(GLfloat)*2*point_space);

    uint32_t line_space=2;
    GLfloat * display_line_buffer=malloc(sizeof(GLfloat)*4*line_space);

    vd->display_line_count=0;
    vd->display_point_count=0;

    for(current_node=vd->first_node;current_node;current_node=current_node->n.next)
    {
        if(vd->display_point_count==point_space)display_point_buffer=realloc(display_point_buffer,sizeof(GLfloat)*2*(point_space*=2));

        display_point_buffer[vd->display_point_count*2+0]=current_node->b.position.x*wf;
        display_point_buffer[vd->display_point_count*2+1]=current_node->b.position.y*hf;
        vd->display_point_count++;

        for(current_connection=current_node->n.first_connection;current_connection;current_connection=current_connection->next) if(current_connection->traversal_direction)
        {
            prev=current_node;
            current=current_connection->point_index;

            do
            {
                if(vd->display_line_count==line_space)display_line_buffer=realloc(display_line_buffer,sizeof(GLfloat)*4*(line_space*=2));

                display_line_buffer[vd->display_line_count*4+0]=prev->b.position.x*wf;
                display_line_buffer[vd->display_line_count*4+1]=prev->b.position.y*hf;

                display_line_buffer[vd->display_line_count*4+2]=current->b.position.x*wf;
                display_line_buffer[vd->display_line_count*4+3]=current->b.position.y*hf;

                vd->display_line_count++;

                prev=current;
                current=current->l.next;
            }
            while(prev->b.is_link_point);
        }
    }

    glBindBuffer_ptr(GL_ARRAY_BUFFER, vd->line_display_array);
    glBufferData_ptr(GL_ARRAY_BUFFER, sizeof(GLfloat)*4*vd->display_line_count,display_line_buffer, GL_STATIC_DRAW);
    glBindBuffer_ptr(GL_ARRAY_BUFFER, 0);

    glBindBuffer_ptr(GL_ARRAY_BUFFER, vd->point_display_array);
    glBufferData_ptr(GL_ARRAY_BUFFER, sizeof(GLfloat)*2*vd->display_point_count,display_point_buffer, GL_STATIC_DRAW);
    glBindBuffer_ptr(GL_ARRAY_BUFFER, 0);

    free(display_line_buffer);
    free(display_point_buffer);
}

static void upload_vector_face_render_data(vectorizer_data * vd)
{
    node_connection *current_connection,*start_connection;
    point *current,*prev;
    face *current_face;

    int32_t count;


    float wf=1.0/((float)vd->w);
    float hf=1.0/((float)vd->h);

    uint32_t display_face_buffer_size=65536;
    GLfloat * display_face_buffer=malloc(sizeof(GLfloat)*2*display_face_buffer_size);///  2 floats per point, points used up to MAX_POINT_NODE_CONNECTIONS times (???) and lines used twice (face each side of line)

    count=0;

    for(current_face=vd->first_face;current_face;current_face=current_face->next)
    {
        current_face->line_render_offset=count;

        current_connection=start_connection=get_face_start_connection(current_face);
        current=start_connection->parent_node;

        do
        {
            if(display_face_buffer_size==count)display_face_buffer=realloc(display_face_buffer,sizeof(GLfloat)*2*(display_face_buffer_size*=2));

            display_face_buffer[count*2+0]=current->b.position.x*wf;
            display_face_buffer[count*2+1]=current->b.position.y*hf;
            count++;

            prev=current;
            current=current_connection->point_index;

            while(current->b.is_link_point)
            {
                if(display_face_buffer_size==count)display_face_buffer=realloc(display_face_buffer,sizeof(GLfloat)*2*(display_face_buffer_size*=2));

                display_face_buffer[count*2+0]=current->b.position.x*wf;
                display_face_buffer[count*2+1]=current->b.position.y*hf;
                count++;

                prev=current;

                if(current_connection->traversal_direction) current=current->l.next;
                else current=current->l.prev;
            }

            current_connection=get_next_ccw_connection(current,prev);
        }
        while(current_connection!=start_connection);

        current_face->line_render_count = count - current_face->line_render_offset;
    }


    glBindBuffer_ptr(GL_ARRAY_BUFFER, vd->face_line_display_array);
    glBufferData_ptr(GL_ARRAY_BUFFER, sizeof(GLfloat)*2*count,display_face_buffer, GL_STATIC_DRAW);
    glBindBuffer_ptr(GL_ARRAY_BUFFER, 0);

    free(display_face_buffer);
}


void perform_requested_vectorizer_steps(vectorizer_data * vd,vectorizer_stage intended_stage)
{
    int i;

    if(((intended_stage==vd->current_stage)&&(vd->variant_stage==(intended_stage+1)))||(vd->current_stage==VECTORIZER_STAGE_START))return;///operation already at this point

    reset_vectorizer_display_variables(vd);

    #warning conditional break/return if asking for current_stage w/ start_stage not previous ????

    if(intended_stage < vd->variant_stage) vd->variant_stage=intended_stage;

    switch(vd->variant_stage)
    {
        case VECTORIZER_STAGE_IMAGE_PROCESSING:
        process_image(vd);
        cluster_pixels(vd);
        if(intended_stage==VECTORIZER_STAGE_IMAGE_PROCESSING)break;


        case VECTORIZER_STAGE_GENERATE_GEOMETRY:
        generate_geometry_from_cluster_indices(vd);
        geometry_correction(vd);
        try_to_trace_all_faces(vd);
        if(intended_stage==VECTORIZER_STAGE_GENERATE_GEOMETRY)break;


        case VECTORIZER_STAGE_CONVERT_TO_BEZIER:
        convert_image_to_bezier_curves(vd);
        if(intended_stage==VECTORIZER_STAGE_CONVERT_TO_BEZIER)break;


        default :
        puts("ERROR UNHANDLED VECTORIZER STAGE");
    }

    ///vd->render_mode=vd->max_render_mode

    if(intended_stage>=VECTORIZER_STAGE_IMAGE_PROCESSING)   vd->render_mode=vd->max_render_mode=VECTORIZER_RENDER_MODE_CLUSTERISED;///processed images
    if(intended_stage>=VECTORIZER_STAGE_GENERATE_GEOMETRY)
    {
        vd->can_render_lines=true;///VECTORIZER_STAGE_GENERATE_GEOMETRY
        vd->render_mode=vd->max_render_mode=VECTORIZER_RENDER_MODE_LINES;
    }
    if(intended_stage>=VECTORIZER_STAGE_CONVERT_TO_BEZIER)
    {
        vd->can_render_curves=true;
        vd->render_mode=vd->max_render_mode=VECTORIZER_RENDER_MODE_CURVES;///bezier faces
    }

    for(i=1;i<=vd->max_render_mode;i++)vd->render_mode_buttons[i]->base.status|= WIDGET_ACTIVE;

    for(i=1;i<=vd->max_render_mode;i++)puts(vd->render_mode_buttons[i]->button.text);


    if(vd->can_render_lines)
    {
        upload_vector_line_render_data(vd);
        vd->render_lines_checkbox_widget->base.status|= WIDGET_ACTIVE;
    }
    if(vd->max_render_mode>=VECTORIZER_RENDER_MODE_LINES)
    {
        upload_vector_face_render_data(vd);
    }
    if(vd->can_render_curves)
    {
        vd->render_curves_checkbox_widget->base.status|= WIDGET_ACTIVE;
    }

    vd->current_stage=intended_stage;
    vd->variant_stage=intended_stage+1;

    organise_toplevel_widget(vd->render_mode_buttons[0]);
}






