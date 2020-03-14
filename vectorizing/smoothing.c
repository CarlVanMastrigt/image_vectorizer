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


void low_pass_filter_geometry(vectorizer_data * vd)
{
    point *current_point,*current_node;
    node_connection *current_connection,*nc;
    vec2f delta,node_delta;
    int smoothable_connections;
    bool smooth_prev,smooth_next;

    prepare_all_point_positions_for_modification(vd);


    for(current_node=vd->first_node;current_node;current_node=current_node->n.next)
    {
        node_delta=(vec2f){0.0,0.0};
        smoothable_connections=0;

        for(current_connection=current_node->n.first_connection;current_connection;current_connection=current_connection->next)
        {
            if((smooth_prev=current_connection->smoothable))
            {
                node_delta=v2f_add(node_delta,current_connection->point_index->b.position);
                smoothable_connections++;
            }

            if(current_connection->traversal_direction) for(current_point=current_connection->point_index;current_point->b.is_link_point;current_point=current_point->l.next)
            {
                smooth_next=true; /// if next is node and it's connection to this point is unsmoothable dont smooth next
                if( ! current_point->l.next->b.is_link_point) for(nc=current_point->l.next->n.first_connection;nc;nc=nc->next) if(nc->point_index==current_point) smooth_next=nc->smoothable;

                delta=v2f_sub(v2f_mid(current_point->l.next->b.position,current_point->l.prev->b.position),current_point->b.position);

                current_point->b.modified_position=v2f_add(current_point->b.modified_position,v2f_mult(delta,0.25));

                if(smooth_next) current_point->l.next->b.modified_position=v2f_sub(current_point->l.next->b.modified_position,v2f_mult(delta,0.125));
                if(smooth_prev) current_point->l.prev->b.modified_position=v2f_sub(current_point->l.prev->b.modified_position,v2f_mult(delta,0.125));

                smooth_prev=true;///after first prev is always smoothable, set above using this chains starting connection's smoothable status;
            }
        }

        if(smoothable_connections)
        {
            node_delta=v2f_sub(v2f_mult(node_delta,1.0/((float)smoothable_connections)),current_node->b.position);
            current_node->b.modified_position=v2f_add(current_node->b.modified_position,v2f_mult(node_delta,0.25));

            node_delta=v2f_mult(node_delta,0.25/((float)smoothable_connections));

            for(current_connection=current_node->n.first_connection;current_connection;current_connection=current_connection->next) if(current_connection->smoothable)
                current_connection->point_index->b.modified_position=v2f_sub(current_connection->point_index->b.modified_position,node_delta);

        }
        else printf("NO SMOOTHABLE NODE CONNECTIONS");
    }

    update_all_point_positions_from_modification(vd);
}



static void adjust_segment_length(point * p1,point *p2)
{
    /// could instead make this split->combine segments outside 0.55 - 1.15 range ? (though this would be worse for sharp non-corner geometry)
    vec2f delta=v2f_sub(p2->b.position,p1->b.position);
    float d=v2f_mag_sq(delta);

    if(d<0.36)///delta mag < 0.6
    {
        d=sqrt(d);
        delta=v2f_mult(delta,(0.5*(0.6-d)+0.05)/d);///by adding 0.05 here, overshoot intended length and give a small buffer

        p1->b.modified_position=v2f_sub  (p1->b.modified_position,delta);
        p2->b.modified_position=v2f_add       (p2->b.modified_position,delta);
    }
    else if(d>1.21)///delta mag > 1.1
    {
        d=sqrt(d);
        delta=v2f_mult(delta,(0.5*(d-1.1)+0.05)/d);///by adding 0.05 here, overshoot intended length and gaie a small buffer

        p1->b.modified_position=v2f_add       (p1->b.modified_position,delta);
        p2->b.modified_position=v2f_sub  (p2->b.modified_position,delta);


    }
}

void adjust_segment_lengths(vectorizer_data * vd)
{
    point *current_point,*current_node;
    node_connection *current_connection;

    prepare_all_point_positions_for_modification(vd);

    ///set modified_position based on positions
    for(current_node=vd->first_node;current_node;current_node=current_node->n.next)
    {
        for(current_connection=current_node->n.first_connection;current_connection;current_connection=current_connection->next)
        {
            if(current_connection->traversal_direction)
            {
                adjust_segment_length(current_node,current_connection->point_index);

                for(current_point=current_connection->point_index;current_point->b.is_link_point;current_point=current_point->l.next)
                {
                    adjust_segment_length(current_point,current_point->l.next);
                }
            }
        }
    }

    update_all_point_positions_from_modification(vd);
}

/// best_nc1 & best_nc2 MUST be unique results for this node
void find_smoothest_node_connections(point * node,node_connection ** best_nc1,node_connection ** best_nc2)
{
    const uint32_t range=3;

    point *p1,*p2;
    uint32_t i;
    float d1,d2,o,min_o;
    vec2f p2_dir,n_dir;
    node_connection *nc1,*nc2;

    *best_nc1 = *best_nc2 = NULL;

    if((node->b.fixed_horizontally)||(node->b.fixed_vertically))return;///if border node then unsmoothable

    if(node->n.first_connection->next->next==NULL)///auto smooth when only 2 nodes
    {
        *best_nc1=node->n.first_connection;
        *best_nc2=node->n.first_connection->next;
        return;
    }

    //min_o=0.03;///corresponds to about 10 degrees
    min_o=0.067;///corresponds to about 15 degrees
    //min_o=0.0;

    for(nc1=node->n.first_connection;nc1;nc1=nc1->next)
    {
        for(i=1,p1=nc1->point_index  ;  (i<range) && (p1->b.is_link_point)  ;  i++,p1=nc1->traversal_direction?p1->l.next:p1->l.prev);///find link point 3 away or first node before that

        n_dir=v2f_sub(node->b.position,p1->b.position);
        d1=v2f_mag_sq(n_dir);

        for(nc2=nc1->next;nc2;nc2=nc2->next)/// angle measured depends on
        {
            for(i=1,p2=nc2->point_index  ;  (i<range) && (p2->b.is_link_point)  ;  i++,p2=nc2->traversal_direction?p2->l.next:p2->l.prev);///find link point 3 away or first node before that

            p2_dir=v2f_sub(p2->b.position,p1->b.position);

            d2=v2f_dist_sq(node->b.position,p2->b.position);

            o=v2f_dot(n_dir,p2_dir);
            o=(v2f_mag_sq(n_dir) - o*o/v2f_mag_sq(p2_dir));///functionally similar to tan(angle)^2

            o/= (d1<d2)?d1:d2;///effectively makes o sin(angle)^2

            if(o<min_o)
            {
                *best_nc1=nc1;
                *best_nc2=nc2;
                min_o=o;
            }
        }
    }
}


void set_connections_smoothable_status(vectorizer_data * vd)
{
    point *current_node;
    node_connection *nc1,*nc2,*current_connection;


    for(current_node=vd->first_node;current_node;current_node=current_node->n.next)
        for(current_connection=current_node->n.first_connection;current_connection;current_connection=current_connection->next)
            current_connection->smoothable=true;///reset all connections to smoothable


    for(current_node=vd->first_node;current_node;current_node=current_node->n.next)
    {
        find_smoothest_node_connections(current_node,&nc1,&nc2);

        if(nc1) for(current_connection=current_node->n.first_connection;current_connection;current_connection=current_connection->next)
        {
            if((current_connection!=nc1)&&(current_connection!=nc2)) current_connection->smoothable=false;///if 2 are specifically smoothable then set others to false
        }
    }
}

static inline float calculate_point_variance(vec2f prev,vec2f current,vec2f next)
{
    current=v2f_sub(current,prev);
    next=v2f_sub(next,prev);

    float dp=v2f_dot(current,next);

    return v2f_mag_sq(current)-dp*dp/v2f_mag_sq(next);
}

void relocate_loop_nodes_to_flattest_spot(vectorizer_data * vd)
{
    const uint32_t assessment_range=8;
    uint32_t i,j,m,off;
    float v,v_min,f;
    uint32_t loop_length,variance_space;
    variance_space=2;
    float * variances=malloc(sizeof(vec2f)*variance_space);
    point *current_node,*next_node,*current_point;
    vec2f prev_pos,current_pos,next_pos;

    for(current_node=vd->first_node;current_node;current_node=next_node)
    {
        next_node=current_node->n.next;///node will be removed from node LL if in loop and not at flattest spot, so record here

        ///if only 2 connections then this is true (assumes every node has at least 2 connections) and also only relocate if not border
        if((current_node->n.first_connection->next->next==NULL)&&( ! current_node->b.fixed_horizontally)&&( ! current_node->b.fixed_vertically))
        {
            variances[0]=calculate_point_variance(current_node->n.first_connection->point_index->b.position,current_node->b.position,current_node->n.first_connection->next->point_index->b.position);

            if(current_node->n.first_connection->traversal_direction)
            {
                current_point=current_node->n.first_connection->point_index;
                prev_pos=current_node->n.first_connection->next->point_index->b.position;
            }
            else
            {
                current_point=current_node->n.first_connection->next->point_index;
                prev_pos=current_node->n.first_connection->point_index->b.position;
            }

            current_pos=current_point->b.position;

            loop_length=1;
            while(current_point->b.is_link_point)
            {
                next_pos=current_point->l.next->b.position;

                if(loop_length==variance_space) variances=realloc(variances,sizeof(vec2f)*(variance_space*=2));

                variances[loop_length++]=calculate_point_variance(prev_pos,current_pos,next_pos);

                prev_pos=current_pos;
                current_pos=next_pos;

                current_point=current_point->l.next;
            }

            if(current_point!=current_node)puts("ERROR DETECTED LOOP DOES NOT REACH ORIGIONAL NODE");

            for(off=loop_length;off < assessment_range;off+=loop_length);///make sure theres enough of an offset to avoid negative indices

            v_min=10000000.0;
            m=0;

            for(i=0;i<loop_length;i++)
            {
                v=variances[i];
                for(j=1,f=0.64;j<assessment_range;j++,f*=0.8) v+=f*(variances[(i+j)%loop_length]+variances[(off+i-j)%loop_length]);

                if(v<v_min)m=i,v_min=v;
            }

            if(m)
            {
                if(current_node->n.first_connection->traversal_direction) current_point=current_node->n.first_connection->point_index;
                else current_point=current_node->n.first_connection->next->point_index;

                while(--m)current_point=current_point->l.next;

                try_to_convert_link_to_node(vd,current_point);
                if(!try_to_convert_node_to_link(vd,current_node))puts("ERROR ORIGIONAL LOOP NODE CONVERSION FAILED");
            }
        }
    }

    free(variances);
}










