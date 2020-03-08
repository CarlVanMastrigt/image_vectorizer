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


node_connection * get_face_start_connection(face * f)
{
    node_connection * current_connection;
    point * current_point=f->start_point;

    while(current_point->b.is_link_point)
    {
        current_point=current_point->l.next;
    }

    for(current_connection=current_point->n.first_connection;current_connection;current_connection=current_connection->next)
    {
        if(current_connection->out_face==f)
        {
            return current_connection;
        }
    }

    puts("START CONNECTION NOT FOUND");
    return NULL;
}

static bool compare_connections_ccw(vec2f incoming_orthogonal,vec2f current_pos,point * p1,point * p2)
{
    /// if p2 is ccw of p1 returns true

    vec2f p1_dir,p2_dir;

    p1_dir=v2f_sub(p1->b.position,current_pos);
    p2_dir=v2f_sub(p2->b.position,current_pos);

    if(v2f_dot(incoming_orthogonal,p1_dir)<0.0)
    {
        return ((v2f_dot(incoming_orthogonal,p2_dir)>0.0)||(v2f_dot(v2f_orth(p1_dir),p2_dir)>0.0));
    }
    else
    {
        return ((v2f_dot(incoming_orthogonal,p2_dir)>0.0)&&(v2f_dot(v2f_orth(p1_dir),p2_dir)>0.0));
    }
}

node_connection * get_next_ccw_connection(point * node,point * prev)
{
    node_connection *current_connection,*most_ccw_connection;

    if(node->n.first_connection->point_index==prev)most_ccw_connection=node->n.first_connection->next;
    else most_ccw_connection=node->n.first_connection;

    vec2f node_pos=node->b.position;
    vec2f incoming_orthogonal=v2f_orth(v2f_sub(node_pos,prev->b.position));

    for(current_connection=node->n.first_connection;current_connection;current_connection=current_connection->next)
    {
        if((current_connection->point_index != prev)&&(compare_connections_ccw(incoming_orthogonal,node_pos,most_ccw_connection->point_index,current_connection->point_index)))
        {
            most_ccw_connection=current_connection;
        }
    }

    return most_ccw_connection;
}


void try_to_trace_all_faces(vectorizer_data * vd)
{
    point *current_node,*current,*prev;
    node_connection *current_connection,*most_ccw_connection,*start_connection,*test_connection;
    uint32_t traversal_direction,i;
    face * f;


    for(current_node=vd->first_node;current_node;current_node=current_node->n.next)for(current_connection=current_node->n.first_connection;current_connection;current_connection=current_connection->next)
    {
        start_connection=most_ccw_connection=current_connection;
        i=0;
        prev=current_node;
        f=current_connection->out_face;

        do
        {
            if(i++ > 1000000) puts("caught in face checking loop (1 million points in loop)"),exit(7);

            current=most_ccw_connection->point_index;
            traversal_direction=most_ccw_connection->traversal_direction;

            while(current->b.is_link_point)
            {
                prev=current;

                if(traversal_direction)
                {
                    //if(current->l.regular_face!=f)printf("FACE WRONG %u %u %u\n",f,current->l.regular_face,current->l.reverse_face),exit(8);
                    if(current->l.regular_face!=f)puts("FACE WRONG"),exit(8);
                    current=current->l.next;
                }
                else
                {
                    //if(current->l.reverse_face!=f)printf("FACE WRONG %u %u %u\n",f,current->l.regular_face,current->l.reverse_face),exit(9);
                    if(current->l.reverse_face!=f)puts("FACE WRONG"),exit(9);
                    current=current->l.prev;
                }
                i++;
            }

            for(test_connection=current->n.first_connection;test_connection;test_connection=test_connection->next) if((test_connection->point_index==prev)&&(test_connection->in_face!=f)) puts("NODE CONNECTION INCOMING FACE WRONG"),exit(10);

            most_ccw_connection=get_next_ccw_connection(current,prev);
            prev=current;
        }
        while(most_ccw_connection!=start_connection);
    }
}


static void recursively_set_shape(point * current_point,uint32_t traversal_direction,shape * parent_shape)///aslo adds face to shape and sets starting points for faces
{
    node_connection * current_connection;

    if(current_point->b.parent_shape)
    {
        if(current_point->b.parent_shape != parent_shape)
        {
            puts("error: parent_shape overlap");
        }

        return;
    }

    while(current_point->b.is_link_point)
    {
        current_point->b.parent_shape=parent_shape;

        current_point = traversal_direction ? current_point->l.next : current_point->l.prev;
    }

    current_point->b.parent_shape=parent_shape;

    for(current_connection=current_point->n.first_connection;current_connection;current_connection=current_connection->next)
    {
        if(current_connection->in_face->start_point==NULL)
        {
            current_connection->in_face->start_point=current_point;
            add_face_to_shape(parent_shape,current_connection->in_face);
        }
        if(current_connection->out_face->start_point==NULL)
        {
            current_connection->out_face->start_point=current_point;
            add_face_to_shape(parent_shape,current_connection->out_face);
        }

        recursively_set_shape(current_connection->point_index,current_connection->traversal_direction,parent_shape);
    }
}

static shape * set_shape(vectorizer_data * vd,shape * parent_shape,face * perimeter_face,point * start_point)
{
    shape * s;

    s=create_shape(vd);
    s->perimeter_face=perimeter_face;

    if(parent_shape)
    {
        s->parent=parent_shape;
        s->next=parent_shape->first_child;
        parent_shape->first_child=s;
    }
    else vd->first_shape=s;

    recursively_set_shape(start_point,0,s);

    return s;
}

/// could/should probably move this part to recursively_set_shape (check and replace face while setting shape)
static face * replace_face_with_perimeter(vectorizer_data * vd,node_connection * start_connection,face * old_face)
{
    point *current,*prev;
    node_connection *nc;
    face * new_face;

    new_face=create_face(vd);
    new_face->perimeter=true;
    new_face->encapsulating_face=old_face;
    new_face->next_excapsulated=old_face->next_excapsulated;
    old_face->next_excapsulated=new_face;

    prev=start_connection->parent_node;
    nc=start_connection;

    do
    {
        current=nc->point_index;

        while(current->b.is_link_point)
        {
            if(nc->traversal_direction)
            {
                if(current->l.regular_face!=old_face)puts("ERROR REPLACING WRONG FACE ");///==========================
                current->l.regular_face=new_face;

                prev=current;
                current=current->l.next;
            }
            else
            {
                if(current->l.reverse_face!=old_face)puts("ERROR REPLACING WRONG FACE ");///==========================
                current->l.reverse_face=new_face;

                prev=current;
                current=current->l.prev;
            }
        }

        for(nc=current->n.first_connection;nc;nc=nc->next) if(nc->point_index==prev)
        {
            if(nc->in_face!=old_face)puts("ERROR REPLACING WRONG FACE ");///==========================
            nc->in_face=new_face;
            break;
        }

        if(nc==NULL)puts("IN FACE NOT FOUND");

        nc=get_next_ccw_connection(current,prev);

        if(nc->out_face!=old_face)puts("ERROR REPLACING WRONG FACE ");///==========================
        nc->out_face=new_face;

        prev=current;
    }
    while(nc!=start_connection);

    return new_face;
}

void identify_shapes(vectorizer_data * vd,face * outermost_perimeter,point * outermost_shape_start_point)
{
    ///this relies on the bound for all faces being set (

    point *current_node;
    node_connection *current_connection;
    shape *parent_shape;
    face *perimeter,*current_face;
    bool undefined_shapes_remain;

    set_shape(vd,NULL,outermost_perimeter,outermost_shape_start_point);


    do
    {
        undefined_shapes_remain=false;

        for(current_node=vd->first_node;current_node;current_node=current_node->n.next) if(current_node->b.parent_shape==NULL)
        {
            undefined_shapes_remain=true;

            for(current_connection=current_node->n.first_connection;current_connection;current_connection=current_connection->next) if((parent_shape=current_connection->out_face->parent_shape))
            {
                ///need to wait for parent(encapsulating) shape to be defined for this part to work
                ///face is part of shape but node is not, ergo this face is perimeter of a new shape face inside "actual" (non-perimeter) version of defined face (current_connection->out_face)
                perimeter=replace_face_with_perimeter(vd,current_connection,current_connection->out_face);
                set_shape(vd,parent_shape,perimeter,current_node);
                break;
            }
        }
    }
    while(undefined_shapes_remain);

    for(current_node=vd->first_node;current_node;current_node=current_node->n.next)if(current_node->b.parent_shape==NULL)puts("ERROR SOME SHAPES NOT DETECTED (NODE)");

    for(current_face=vd->first_face;current_face;current_face=current_face->next) if(current_face->start_point->b.parent_shape==NULL) puts("ERROR SOME SHAPES NOT DETECTED (FACE)");

    ///check all shapes parents are their perimeter's encapsulating_face's shape
}




