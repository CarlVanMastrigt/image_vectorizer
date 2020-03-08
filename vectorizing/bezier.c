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


static const float min_control_point_test_delta=0.04;
static const float increased_variance_factor=10000.0;///variances within reduced_variance_factor_range are multiplied by this, lower this is, less of a factor variances within reduced_variance_factor_range_squared play to total variance
static const float increased_variance_factor_range_squared=0.09;/// (0.3) ^2
static const uint32_t minimum_bezier_segment_count=3;
static const float variance_limit_factor_squared=0.0225;///limit of average variance for combining segments (0.15) ^2
static const float bezier_control_point_magnitude_factor=0.8;
static const float starting_gradient_correction_delta=0.09;///about 0.09->15 degress total range (7.5 either way),
static const float min_gradient_correction_delta=0.007;///this results in 3 levels angles checked, with min delta 0.6 degrees (0.01 radians)
static const uint32_t bezier_alteration_offset_range=81;///4 levels of movement to test

typedef struct bezier_section bezier_section;
typedef struct bezier_alteration bezier_alteration;




struct bezier_alteration
{
    ///used by all
    float relative_variance;

    bezier_section *bs_1,*bs_2;
    uint32_t ta_1,ta_2;

    float variance_1,m1_1,m2_1,l_1;///recycled for prev/next in combination_complex
    float variance_2,m1_2,m2_2,l_2;///recycled for prev/next in combination_complex

    ///used by   find_best_section_alteration_centre_movement
    uint32_t offset_1,offset_2;///need 2 variants so same code can be recycled for closed-loop-node changes to gradient w/o extra tests
    float variance,m1,m2;///for combined usage, technically not needed, could use variave_1,m1_1,m2_1
    vec2f grad;

};


struct bezier_section
{
    float variance;///functionally compensated average distance, (adjusted to be lower when section is longer)

    uint32_t times_altered;///is this still being used for the section that it was when associated bezier_alteration was created
    ///must change when deleted or used to represent different geometry

    float m1,m2,total_length;///distance along join gradient to place control points
    node_connection *start_connection,*end_connection;
    uint32_t start_offset,end_offset;
    vec2f start_grad,end_grad;
    //vec2f start_gradient_original,end_gradient_original;///try to avoid using end_gradient
    //vec2f start_gradient,end_gradient;
    bezier_section *prev,*next;
};



static inline bezier_section * get_bezier_section(bezier_section ** first_available_section)
{
    bezier_section * bs;

    if(*first_available_section)
    {
        bs= *first_available_section;
        *first_available_section=bs->next;
    }
    else
    {
        bs=malloc(sizeof(bezier_section));
        bs->times_altered=0;
    }

    return bs;
}

static inline void relinquish_bezier_section(bezier_section ** first_available_section,bezier_section *bs)
{
    bs->times_altered++;
    bs->next= *first_available_section;
    *first_available_section=bs;
}







///may need something better to allow more complex curves?
static inline float calculate_bezier_section_variance(vec2f p1,vec2f p2,vec2f c1,vec2f c2,uint32_t s,uint32_t e,vec2f * points,vec2f * segment_directions,float * segment_lengths)
{
    float total_v=0.0;
    uint32_t i,o;
    float f,g,v,offset_dp,inv_c;
    vec2f p,offset;


    if((e-s)<2)return 0.0;///if no samples (chain between nodes is 1 segment, ergo e-s < 2) just return 0, also avoids div by 0 later

    total_v=0.0;///add small extra offset for each point

    inv_c=1.0/((float)(e-s));

    o=s;

    for(i=1 ; i<(e-s) ; i++)
    {
        f=((float)i)*inv_c;
        g=1.0-f;

        ///bezier interpolate
        p.x= f*f*(f*p2.x+3.0*g*c2.x) + g*g*(3.0*f*c1.x+g*p1.x);
        p.y= f*f*(f*p2.y+3.0*g*c2.y) + g*g*(3.0*f*c1.y+g*p1.y);

        while( (o<(e-1)) && (  (offset_dp=v2f_dot((offset=v2f_sub(p,points[o])),segment_directions[o]) )  > segment_lengths[o]  ) )o++;
        ///move forward while p is beyond this segments end, setting d & dp after each step forward as required

        //if((offset_dp>0.0)&&(offset_dp*offset_dp>v2f_mag_sq(offset)))printf("%f - %f %f\n",v2f_mag_sq(offset)-offset_dp*offset_dp,offset_dp,segment_lengths[o]);

        v=v2f_mag_sq(offset);
        if((offset_dp>0.0)&&(v>(offset_dp*offset_dp)))v-=offset_dp*offset_dp;
        #warning instead of above set v to a vey small value if dp>0 and v-dp*dp is less than that small value

        if(v>increased_variance_factor_range_squared)total_v += (v-increased_variance_factor_range_squared)*increased_variance_factor + increased_variance_factor_range_squared;
        else total_v += v;
    }

    return total_v;
}

///change this to have gradients,start_index and end_index as input rather than bezier_section * bs ( allows use when cosidering replacement bezier sections without allocating them )
static float find_bezier_section_lowest_variance(vec2f * points,vec2f * segment_directions,float * segment_lengths,float * best_m1,float * best_m2,float total_length,uint32_t s,uint32_t e,vec2f g1,vec2f g2)
{
    vec2f p1,p2;
    float m1,m2,f;
    float v,min_v;

    *best_m1 = *best_m2 = 0.5*total_length;

    p1=points[s];
    p2=points[e];

    min_v=calculate_bezier_section_variance(p1,p2,v2f_add(p1,v2f_mult(g1,*best_m1)),v2f_sub(p2,v2f_mult(g2,*best_m2)),s,e,points,segment_directions,segment_lengths);

    ///0.25 below b/c sum(1/3^n)=0.5, and this is only affecting values over half total range (+/- checks below)
    for(f=total_length*0.333333*bezier_control_point_magnitude_factor  ;  f>min_control_point_test_delta  ;  f*=0.33333333)
    {
        m1= *best_m1;
        m2= *best_m2;

        ///varying 1 side

        v=calculate_bezier_section_variance(p1,p2,v2f_add(p1,v2f_mult(g1,m1+f)),v2f_sub(p2,v2f_mult(g2,m2)),s,e,points,segment_directions,segment_lengths);
        if(v<min_v) min_v=v , *best_m1=m1+f , *best_m2=m2;

        v=calculate_bezier_section_variance(p1,p2,v2f_add(p1,v2f_mult(g1,m1-f)),v2f_sub(p2,v2f_mult(g2,m2)),s,e,points,segment_directions,segment_lengths);
        if(v<min_v) min_v=v , *best_m1=m1-f , *best_m2=m2;

        v=calculate_bezier_section_variance(p1,p2,v2f_add(p1,v2f_mult(g1,m1)),v2f_sub(p2,v2f_mult(g2,m2+f)),s,e,points,segment_directions,segment_lengths);
        if(v<min_v) min_v=v , *best_m1=m1  , *best_m2=m2+f;

        v=calculate_bezier_section_variance(p1,p2,v2f_add(p1,v2f_mult(g1,m1)),v2f_sub(p2,v2f_mult(g2,m2-f)),s,e,points,segment_directions,segment_lengths);
        if(v<min_v) min_v=v , *best_m1=m1  , *best_m2=m2-f;

        ///varying both

        v=calculate_bezier_section_variance(p1,p2,v2f_add(p1,v2f_mult(g1,m1+f)),v2f_sub(p2,v2f_mult(g2,m2+f)),s,e,points,segment_directions,segment_lengths);
        if(v<min_v) min_v=v , *best_m1=m1+f , *best_m2=m2+f;

        v=calculate_bezier_section_variance(p1,p2,v2f_add(p1,v2f_mult(g1,m1+f)),v2f_sub(p2,v2f_mult(g2,m2-f)),s,e,points,segment_directions,segment_lengths);
        if(v<min_v) min_v=v , *best_m1=m1+f , *best_m2=m2-f;

        v=calculate_bezier_section_variance(p1,p2,v2f_add(p1,v2f_mult(g1,m1-f)),v2f_sub(p2,v2f_mult(g2,m2+f)),s,e,points,segment_directions,segment_lengths);
        if(v<min_v) min_v=v , *best_m1=m1-f , *best_m2=m2+f;

        v=calculate_bezier_section_variance(p1,p2,v2f_add(p1,v2f_mult(g1,m1-f)),v2f_sub(p2,v2f_mult(g2,m2-f)),s,e,points,segment_directions,segment_lengths);
        if(v<min_v) min_v=v , *best_m1=m1-f , *best_m2=m2-f;
    }

    ///if(min_v<-0.001)printf("%f\n",min_v),exit(11);

    //if(min_v<0.0000001)min_v=0.0000001;

    return min_v;
}

static void subdivide_bezier_section(bezier_section * old_section,vec2f * points,vec2f * segment_directions,float * segment_lengths,bezier_section ** first_available_section)
{
    uint32_t i;

    if((old_section->end_offset - old_section->start_offset)<(2*minimum_bezier_segment_count))
    {
        /// need to sum length of all segments in this section
        old_section->total_length=0.0;
        for(i=old_section->start_offset ; i<old_section->end_offset ; i++)old_section->total_length+=segment_lengths[i];
    }
    else
    {
        bezier_section * new_section=get_bezier_section(first_available_section);

        new_section->end_connection=old_section->end_connection;
        new_section->end_offset=old_section->end_offset;
        new_section->next=old_section->next;

        new_section->start_connection=NULL;
        new_section->start_offset= (old_section->start_offset + old_section->end_offset)>>1;///midpoint
        new_section->prev=old_section;

        old_section->end_connection=NULL;
        old_section->end_offset=new_section->start_offset;
        old_section->next=new_section;


        if(new_section->next)new_section->next->prev=new_section;


        subdivide_bezier_section(old_section,points,segment_directions,segment_lengths,first_available_section);
        subdivide_bezier_section(new_section,points,segment_directions,segment_lengths,first_available_section);
    }
}


static bezier_alteration get_top_section_alteration(bezier_alteration * alterations,uint32_t last_alteration)///remove top of binary heap
{
    uint32_t u,d;
    bezier_alteration rtrn=alterations[0];

    u=0;

    while(  (d=(u<<1)+1)  <  last_alteration  )
    {
        d+=(((d+1)<last_alteration)&&(alterations[d+1].relative_variance < alterations[d].relative_variance));

        if(alterations[d].relative_variance > alterations[last_alteration].relative_variance)break;

        alterations[u]=alterations[d];
        u=d;
    }

    alterations[u]=alterations[last_alteration];

    return rtrn;
}

static void add_section_alteration(bezier_alteration * alterations,uint32_t d,bezier_alteration a)
{
    uint32_t u;

    while(d)
    {
        u=(d-1)>>1;

        if(a.relative_variance > alterations[u].relative_variance) break;
        alterations[d]=alterations[u];

        d=u;
    }

    alterations[d]=a;
}





static uint32_t find_best_section_alteration_combination_simple(vec2f * points,vec2f * segment_directions,float * segment_lengths,bezier_alteration * alterations,uint32_t alteration_count,bezier_section * bs_1,bezier_section * bs_2)
{
    bezier_alteration a;

    if((bs_1->end_connection)||(bs_2->start_connection))return alteration_count;
    if((bs_1->prev==bs_2)||(bs_2->next==bs_1)) return alteration_count;///dont make "1 bezier" loop
    ///in above tests both conditions are the same, but w/e

    a.bs_1=bs_1;
    a.bs_2=bs_2;
    a.ta_1=bs_1->times_altered;
    a.ta_2=bs_2->times_altered;

    a.variance=find_bezier_section_lowest_variance(points,segment_directions,segment_lengths,&a.m1,&a.m2,bs_1->total_length+bs_2->total_length,bs_1->start_offset,bs_2->end_offset,bs_1->start_grad,bs_2->end_grad);
    a.relative_variance=a.variance/(bs_1->variance + bs_2->variance);

    if(a.variance < variance_limit_factor_squared*(bs_2->end_offset-bs_1->start_offset-1)) add_section_alteration(alterations,alteration_count++,a);

    return alteration_count;
}

static uint32_t perform_section_alteration_combination_simple(vec2f * points,vec2f * segment_directions,float * segment_lengths,bezier_alteration a,bezier_section ** first_available_section,bezier_section ** first_section,bezier_alteration * alterations,uint32_t alteration_count)
{
    if((a.bs_1->times_altered!=a.ta_1)||(a.bs_2->times_altered!=a.ta_2))return alteration_count;

    bezier_section * new_section;

    ///first_section is ptr head of active section LL, needed for when first section is combined

    new_section=get_bezier_section(first_available_section);

    new_section->start_connection=a.bs_1->start_connection;
    new_section->start_offset=a.bs_1->start_offset;
    new_section->start_grad=a.bs_1->start_grad;
    new_section->prev=a.bs_1->prev;

    new_section->end_connection=a.bs_2->end_connection;
    new_section->end_offset=a.bs_2->end_offset;
    new_section->end_grad=a.bs_2->end_grad;
    new_section->next=a.bs_2->next;

    new_section->total_length=a.bs_1->total_length+a.bs_2->total_length;
    new_section->m1=a.m1;
    new_section->m2=a.m2;
    new_section->variance=a.variance;

    if(*first_section == a.bs_1) *first_section=new_section;

    if(new_section->prev)new_section->prev->next=new_section;
    if(new_section->next)new_section->next->prev=new_section;

    relinquish_bezier_section(first_available_section,a.bs_1);
    relinquish_bezier_section(first_available_section,a.bs_2);


    ///both above pre/next changes must be made before this is called, otherwise to-be 2-bezier loops would break as prev/next not yet set correctly
    if(new_section->prev) alteration_count=find_best_section_alteration_combination_simple(points,segment_directions,segment_lengths,alterations,alteration_count,new_section->prev,new_section);
    if(new_section->next) alteration_count=find_best_section_alteration_combination_simple(points,segment_directions,segment_lengths,alterations,alteration_count,new_section,new_section->next);

    return alteration_count;
}



static bezier_alteration test_bezier_alteration_adjacent_gradients(vec2f * points,vec2f * segment_directions,float * segment_lengths,bezier_alteration a,float gradient_delta,float inv_old_variance)
{
    float m1_1,m2_1,m1_2,m2_2,v_1,v_2;
    vec2f p_grad,n_grad;

    p_grad=v2f_rotate(a.grad,+gradient_delta);
    n_grad=v2f_rotate(a.grad,-gradient_delta);

    if(a.bs_1) v_1=find_bezier_section_lowest_variance(points,segment_directions,segment_lengths,&m1_1,&m2_1,a.l_1,a.bs_1->start_offset,a.offset_1,a.bs_1->start_grad,p_grad);
    else v_1=0.0;
    if(a.bs_2) v_2=find_bezier_section_lowest_variance(points,segment_directions,segment_lengths,&m1_2,&m2_2,a.l_2,a.offset_2,a.bs_2->end_offset,p_grad,a.bs_2->end_grad);
    else v_2=0.0;

    if((v_1+v_2)*inv_old_variance<a.relative_variance)
    {
        a.relative_variance=(v_1+v_2)*inv_old_variance;
        a.variance_1=v_1 , a.m1_1=m1_1 , a.m2_1=m2_1;
        a.variance_2=v_2 , a.m1_2=m1_2 , a.m2_2=m2_2;
        a.grad=p_grad;
    }

    if(a.bs_1) v_1=find_bezier_section_lowest_variance(points,segment_directions,segment_lengths,&m1_1,&m2_1,a.l_1,a.bs_1->start_offset,a.offset_1,a.bs_1->start_grad,n_grad);
    else v_1=0.0;
    if(a.bs_2) v_2=find_bezier_section_lowest_variance(points,segment_directions,segment_lengths,&m1_2,&m2_2,a.l_2,a.offset_2,a.bs_2->end_offset,n_grad,a.bs_2->end_grad);
    else v_2=0.0;

    if((v_1+v_2)*inv_old_variance<a.relative_variance)
    {
        a.relative_variance=(v_1+v_2)*inv_old_variance;
        a.variance_1=v_1 , a.m1_1=m1_1 , a.m2_1=m2_1;
        a.variance_2=v_2 , a.m1_2=m1_2 , a.m2_2=m2_2;
        a.grad=n_grad;
    }

    return a;
}

static uint32_t find_best_section_alteration_adjustment(vec2f * points,vec2f * segment_directions,float * segment_lengths,bezier_alteration * alterations,uint32_t alteration_count,bezier_section * bs_1,bezier_section * bs_2)
{
    bezier_alteration a,a_pos,a_neg;
    float inv_old_variance;

    float g_delta;
    uint32_t i,o_delta;


    a.bs_1=bs_1;
    a.bs_2=bs_2;

    inv_old_variance=0.0;

    if((bs_1==NULL)&&(bs_2==NULL))puts("FFFF");


    if(bs_1)///only adjust bs_2 grad using fixed_gradient_correction_range
    {
        a.offset_1=bs_1->end_offset;///also == bs_2->start_offset
        a.ta_1=bs_1->times_altered;
        a.l_1=bs_1->total_length;
        inv_old_variance+=bs_1->variance;
        a.grad=segment_directions[bs_1->end_offset-1];
    }

    if(bs_2)///only adjust bs_1 grad using fixed_gradient_correction_range
    {
        a.offset_2=bs_2->start_offset;///also == bs_1->end_offset
        a.ta_2=bs_2->times_altered;
        a.l_2=bs_2->total_length;
        inv_old_variance+=bs_2->variance;
        a.grad=segment_directions[bs_2->start_offset];
    }

    if((bs_1)&&(bs_2))
    {
        a.grad=v2f_norm(v2f_sub(points[a.offset_2+1],points[a.offset_1-1]));
    }


    ///set variables applicable to both variants

    inv_old_variance=1.0/inv_old_variance;///this should still be a viable variance, will (should) be tested again, but is impossible to know which test it represents (but will (should) definitely be one of first set of gradients)

    if(bs_1) a.variance_1=find_bezier_section_lowest_variance(points,segment_directions,segment_lengths,&a.m1_1,&a.m2_1,a.l_1,bs_1->start_offset,a.offset_1,bs_1->start_grad,a.grad);
    else a.variance_1=0.0;

    if(bs_2) a.variance_2=find_bezier_section_lowest_variance(points,segment_directions,segment_lengths,&a.m1_2,&a.m2_2,a.l_2,a.offset_2,bs_2->end_offset,a.grad,bs_2->end_grad);
    else a.variance_2=0.0;

    a.relative_variance=(a.variance_1+a.variance_2)*inv_old_variance;

    for(g_delta=starting_gradient_correction_delta;g_delta>min_gradient_correction_delta;g_delta*=0.33333333) ///finds best grad at initial offset
        a=test_bezier_alteration_adjacent_gradients(points,segment_directions,segment_lengths,a,g_delta,inv_old_variance);

    if((bs_1) && (bs_2) && (bs_1->end_connection==NULL) && (bs_2->start_connection==NULL)) for(o_delta=bezier_alteration_offset_range;o_delta;o_delta/=3)///if centre not fixed (not node) try moving as well
    {
        a_pos=a_neg=a;

        if(a_pos.offset_2+minimum_bezier_segment_count+o_delta <= bs_2->end_offset)
        {
            a_pos.offset_1+=o_delta;
            a_pos.offset_2+=o_delta;

            for(a_pos.l_1=0.0,i=bs_1->start_offset;i<a_pos.offset_1;i++)a_pos.l_1+=segment_lengths[i];
            for(a_pos.l_2=0.0,i=a_pos.offset_2;i<bs_2->end_offset;i++)a_pos.l_2+=segment_lengths[i];
            a_pos.grad=v2f_norm(v2f_sub(points[a_pos.offset_2+1],points[a_pos.offset_1-1]));
            a_pos.variance_1=find_bezier_section_lowest_variance(points,segment_directions,segment_lengths,&a_pos.m1_1,&a_pos.m2_1,a_pos.l_1,bs_1->start_offset,a_pos.offset_1,bs_1->start_grad,a_pos.grad);
            a_pos.variance_2=find_bezier_section_lowest_variance(points,segment_directions,segment_lengths,&a_pos.m1_2,&a_pos.m2_2,a_pos.l_2,a_pos.offset_2,bs_2->end_offset,a_pos.grad,bs_2->end_grad);
            a_pos.relative_variance=(a_pos.variance_1+a_pos.variance_2)*inv_old_variance;

            for(g_delta=starting_gradient_correction_delta;g_delta>min_gradient_correction_delta;g_delta*=0.33333333) ///finds best grad at initial offset
                a_pos=test_bezier_alteration_adjacent_gradients(points,segment_directions,segment_lengths,a_pos,g_delta,inv_old_variance);

            if(a_pos.relative_variance < a.relative_variance)a=a_pos;//,printf("PO %u\n",o_delta)
        }
        if(bs_1->start_offset+minimum_bezier_segment_count+o_delta <= a_neg.offset_1)
        {
            a_neg.offset_1-=o_delta;
            a_neg.offset_2-=o_delta;

            for(a_neg.l_1=0.0,i=bs_1->start_offset;i<a_neg.offset_1;i++)a_neg.l_1+=segment_lengths[i];
            for(a_neg.l_2=0.0,i=a_neg.offset_2;i<bs_2->end_offset;i++)a_neg.l_2+=segment_lengths[i];
            a_neg.grad=v2f_norm(v2f_sub(points[a_neg.offset_2+1],points[a_neg.offset_1-1]));
            a_neg.variance_1=find_bezier_section_lowest_variance(points,segment_directions,segment_lengths,&a_neg.m1_1,&a_neg.m2_1,a_neg.l_1,bs_1->start_offset,a_neg.offset_1,bs_1->start_grad,a_neg.grad);
            a_neg.variance_2=find_bezier_section_lowest_variance(points,segment_directions,segment_lengths,&a_neg.m1_2,&a_neg.m2_2,a_neg.l_2,a_neg.offset_2,bs_2->end_offset,a_neg.grad,bs_2->end_grad);
            a_neg.relative_variance=(a_neg.variance_1+a_neg.variance_2)*inv_old_variance;

            for(g_delta=starting_gradient_correction_delta;g_delta>min_gradient_correction_delta;g_delta*=0.33333333) ///finds best grad at initial offset
                a_neg=test_bezier_alteration_adjacent_gradients(points,segment_directions,segment_lengths,a_neg,g_delta,inv_old_variance);

            if(a_neg.relative_variance < a.relative_variance)a=a_neg;//,printf("NO %u\n",o_delta)
        }
    }

    if(a.relative_variance<0.999) add_section_alteration(alterations,alteration_count++,a);///0.999 instead of 1.0 to ensure non-trivial improvement

    return alteration_count;
}

static uint32_t perform_section_alteration_adjustment(vec2f * points,vec2f * segment_directions,float * segment_lengths,bezier_alteration a,bezier_section ** first_available_section,bezier_section ** first_section,bezier_alteration * alterations,uint32_t alteration_count)
{
    if(((a.bs_1)&&(a.bs_1->times_altered!=a.ta_1))||((a.bs_2)&&(a.bs_2->times_altered!=a.ta_2)))return alteration_count;

    if(a.bs_1)
    {
        a.bs_1->end_offset=a.offset_1;
        a.bs_1->end_grad=a.grad;
        a.bs_1->m1=a.m1_1;
        a.bs_1->m2=a.m2_1;
        a.bs_1->total_length=a.l_1;
        a.bs_1->variance=a.variance_1;
        a.bs_1->times_altered++;

        if(a.bs_1->prev) alteration_count=find_best_section_alteration_adjustment(points,segment_directions,segment_lengths,alterations,alteration_count,a.bs_1->prev,a.bs_1);
    }

    if(a.bs_2)
    {
        a.bs_2->start_offset=a.offset_2;
        a.bs_2->start_grad=a.grad;
        a.bs_2->m1=a.m1_2;
        a.bs_2->m2=a.m2_2;
        a.bs_2->total_length=a.l_2;
        a.bs_2->variance=a.variance_2;
        a.bs_2->times_altered++;

        if(a.bs_2->next) alteration_count=find_best_section_alteration_adjustment(points,segment_directions,segment_lengths,alterations,alteration_count,a.bs_2,a.bs_2->next);
    }

    if((a.bs_1)&&(a.bs_2)&&(a.bs_1->end_connection==NULL)&&(a.bs_2->start_connection==NULL))alteration_count=find_best_section_alteration_adjustment(points,segment_directions,segment_lengths,alterations,alteration_count,a.bs_1,a.bs_2);
    ///try correcting again to try to find better movement, this shouldnt be needed for unmoving entries as it's exactly the same test

    return alteration_count;
}





void convert_complete_chain_to_bezier_curves(vectorizer_data * vd,node_connection * start_connection,bezier_section ** first_available_section)
{
    uint32_t point_count,point_space;
    point *current_point,*prev_point;
    node_connection *current_connection,*sc1,*sc2;
    vec2f *points,*segment_directions;
    float *segment_lengths;
    bezier_section *first_section,*current_section,*next_section;
    bezier *b,*bp;
    bezier_alteration a;
    uint32_t traversal_direction;

    bezier_alteration * alterations;
    uint32_t alteration_count,alteration_space;

    point_space=16;
    point_count=0;
    points=malloc(sizeof(vec2f)*point_space);
    segment_directions=malloc(sizeof(vec2f)*point_space);
    segment_lengths=malloc(sizeof(float)*point_space);


    alteration_space=16;
    alteration_count=0;
    alterations=malloc(sizeof(bezier_alteration)*alteration_space);


    prev_point=start_connection->parent_node;
    points[point_count++]=prev_point->b.position;
    current_connection=start_connection;

    #warning rather than having end_gradient for section have end gradient for chain which is used when section->next is null, otherwise section->next->start_gradient is used




    first_section=current_section=get_bezier_section(first_available_section);
    first_section->start_connection=start_connection;
    first_section->start_offset=0;
    first_section->prev=NULL;

    do
    {
        for(current_point=current_connection->point_index ; current_point->b.is_link_point ; current_point = current_connection->traversal_direction ? current_point->l.next : current_point->l.prev)
        {
            if((point_count+2)>point_space)/// +2 ensures space for node data if loop is exited when point_count==point_space (space for 2 more
            {
                point_space*=2;
                points=realloc(points,sizeof(vec2f)*point_space);
                segment_directions=realloc(segment_directions,sizeof(vec2f)*point_space);
                segment_lengths=realloc(segment_lengths,sizeof(float)*point_space);
            }

            points[point_count]=current_point->b.position;
            segment_directions[point_count-1]=v2f_sub(points[point_count],points[point_count-1]);
            segment_lengths[point_count-1]=v2f_mag(segment_directions[point_count-1]);
            segment_directions[point_count-1]=v2f_mult(segment_directions[point_count-1],1.0/segment_lengths[point_count-1]);
            point_count++;

            prev_point = current_point;
        }

        points[point_count]=current_point->b.position;
        segment_directions[point_count-1]=v2f_sub(points[point_count],points[point_count-1]);
        segment_lengths[point_count-1]=v2f_mag(segment_directions[point_count-1]);
        segment_directions[point_count-1]=v2f_mult(segment_directions[point_count-1],1.0/segment_lengths[point_count-1]);
        point_count++;



        find_smoothest_node_connections(current_point,&sc1,&sc2);

        if((sc1)&&(sc1->point_index==prev_point))current_connection=sc2;
        else if((sc2)&&(sc2->point_index==prev_point))current_connection=sc1;
        else current_connection=NULL;///terminate this chain


        for(current_section->end_connection=current_point->n.first_connection;current_section->end_connection;current_section->end_connection=current_section->end_connection->next)if(current_section->end_connection->point_index==prev_point)break;
        if(current_section->end_connection==NULL)puts("ERROR BEZIER SECTION END CONNECTION NULL");
        current_section->end_offset=point_count-1;



        if(current_connection==start_connection)
        {
            ///connect last to first to form loop
            current_section->next=first_section;
            first_section->prev=current_section;
        }
        else if(current_connection)
        {
            ///just another node join that passes through
            next_section=get_bezier_section(first_available_section);

            next_section->start_connection=current_connection;
            next_section->start_offset=point_count-1;
            next_section->prev=current_section;

            current_section->next=next_section;

            subdivide_bezier_section(current_section,points,segment_directions,segment_lengths,first_available_section);

            current_section=next_section;
        }
        else
        {
            ///regular termination
            current_section->next=NULL;
        }

        prev_point=current_point;
    }
    while((current_connection)&&(current_connection!=start_connection));


    subdivide_bezier_section(current_section,points,segment_directions,segment_lengths,first_available_section);///break up last section


    current_section=first_section;
    do
    {
        current_section->start_grad = v2f_norm(v2f_sub(points[current_section->start_offset+1],points[(current_section->prev)?current_section->prev->end_offset-1:current_section->start_offset]));
        current_section->end_grad = v2f_norm(v2f_sub(points[(current_section->next)?current_section->next->start_offset+1:current_section->end_offset],points[current_section->end_offset-1]));

        current_section->variance=find_bezier_section_lowest_variance(points,segment_directions,segment_lengths,
            &current_section->m1,&current_section->m2,current_section->total_length,current_section->start_offset,current_section->end_offset,current_section->start_grad,current_section->end_grad);

        current_section=current_section->next;
    }
    while((current_section)&&(current_section!=first_section));



//printf("===================================================== %u\n",point_count);



    for(;;)///combination & centre_movement
    {
        ///combine any that fit withing variance limits when combined
        current_section=first_section;
        do
        {
            if(alteration_count==alteration_space)alterations=realloc(alterations,sizeof(bezier_alteration)*(alteration_space*=2));
            if(current_section->next)alteration_count=find_best_section_alteration_combination_simple(points,segment_directions,segment_lengths,alterations,alteration_count,current_section,current_section->next);
            current_section=current_section->next;
        }
        while((current_section)&&(current_section!=first_section));

        if(alteration_count==0)break;

        while(alteration_count)
        {
            a=get_top_section_alteration(alterations,--alteration_count);
            if((alteration_count+2)>alteration_space)alterations=realloc(alterations,sizeof(bezier_alteration)*(alteration_space*=2));
            alteration_count=perform_section_alteration_combination_simple(points,segment_directions,segment_lengths,a,first_available_section,&first_section,alterations,alteration_count);
        }


        if( ! vd->adjust_beziers)break;

        /// test various shifted centres for section pairs and various gradients for each new centre tested to decrease variance

        if(first_section->prev==NULL) alteration_count=find_best_section_alteration_adjustment(points,segment_directions,segment_lengths,alterations,alteration_count,NULL,first_section);
        ///always at least 1 space so no need to check for space for this

        current_section=first_section;
        do
        {
            if(alteration_count==alteration_space)alterations=realloc(alterations,sizeof(bezier_alteration)*(alteration_space*=2));
            alteration_count=find_best_section_alteration_adjustment(points,segment_directions,segment_lengths,alterations,alteration_count,current_section,current_section->next);
            current_section=current_section->next;
        }
        while((current_section)&&(current_section!=first_section));

        if(alteration_count==0)break;///if nothing can be done to improve extant bexiers then above combo is best and need not be tested again

        while(alteration_count)
        {
            a=get_top_section_alteration(alterations,--alteration_count);
            if((alteration_count+3)>alteration_space)alterations=realloc(alterations,sizeof(bezier_alteration)*(alteration_space*=2));
            alteration_count=perform_section_alteration_adjustment(points,segment_directions,segment_lengths,a,first_available_section,&first_section,alterations,alteration_count);
        }
    }





    current_section=first_section;
    bp=NULL;


    do
    {
        b=create_bezier(vd);

        b->straight_line=false;
        b->num_sections=current_section->end_offset - current_section->start_offset;


        if(current_section->start_connection)
        {
            traversal_direction=current_section->start_connection->traversal_direction;

            if(traversal_direction)b->prev=NULL;
            else b->next=NULL;

            current_section->start_connection->bezier_index=b;
            //puts("start bez set");
        }
        else
        {
            if(bp==NULL) puts("ERROR NO PREV WHEN STARTING CONNECTION NULL");

            if(traversal_direction)
            {
                bp->next=b;
                b->prev=bp;
            }
            else
            {
                bp->prev=b;
                b->next=bp;
            }
        }

        if((current_section->m1<0.0)||(current_section->m2<0.0))puts("negative gradient");


        if(traversal_direction)
        {
            b->p1=points[current_section->start_offset];
            b->p2=points[current_section->end_offset];

            b->c1=v2f_add(b->p1,v2f_mult(current_section->start_grad,current_section->m1));
            b->c2=v2f_sub(b->p2,v2f_mult(current_section->end_grad,current_section->m2));
        }
        else
        {
            b->p1=points[current_section->end_offset];
            b->p2=points[current_section->start_offset];

            b->c1=v2f_sub(b->p1,v2f_mult(current_section->end_grad,current_section->m2));
            b->c2=v2f_add(b->p2,v2f_mult(current_section->start_grad,current_section->m1));
        }



        if(current_section->end_connection)
        {
            if(traversal_direction)b->next=NULL;
            else b->prev=NULL;

            current_section->end_connection->bezier_index=b;
            //puts("end bez set");

            bp=NULL;///not strictly necesssary but good for error checking
        }
        else bp=b;



        next_section=current_section->next;

        relinquish_bezier_section(first_available_section,current_section);

        current_section=next_section;
    }
    while((current_section)&&(current_section!=first_section));

//puts("DDD");

    ///set image beziers


    free(points);
    free(segment_directions);
    free(segment_lengths);
    free(alterations);
}

static void convert_border_chain_to_beziers(vectorizer_data * vd,node_connection * start_connection)
{
    bezier *prev_bezier,*current_bezier;
    point *prev_point,*current_point;
    node_connection * current_connection;


    prev_point=start_connection->parent_node;

    start_connection->bezier_index=current_bezier=create_bezier(vd);
    current_bezier->straight_line=true;
    current_bezier->num_sections=1;

    if(start_connection->traversal_direction)
    {
        current_bezier->c1=current_bezier->p1=prev_point->b.position;
        current_bezier->prev=NULL;
    }
    else
    {
        current_bezier->c2=current_bezier->p2=prev_point->b.position;
        current_bezier->next=NULL;
    }

    for(current_point=start_connection->point_index;current_point->b.is_link_point;current_point=(start_connection->traversal_direction)?current_point->l.next:current_point->l.prev)
    {
        if((!current_point->b.fixed_horizontally)&&(!current_point->b.fixed_vertically))puts("BERROR EZIER BORDER POINT NOT BORDER");


        if((current_point->b.fixed_horizontally)&&(current_point->b.fixed_vertically))
        {
            prev_bezier=current_bezier;

            current_bezier=create_bezier(vd);
            current_bezier->straight_line=true;
            current_bezier->num_sections=1;

            if(start_connection->traversal_direction)
            {
                prev_bezier->next=current_bezier;
                current_bezier->prev=prev_bezier;

                prev_bezier->c2=prev_bezier->p2=current_bezier->c1=current_bezier->p1=current_point->b.position;
            }
            else
            {
                prev_bezier->prev=current_bezier;
                current_bezier->next=prev_bezier;

                prev_bezier->c1=prev_bezier->p1=current_bezier->c2=current_bezier->p2=current_point->b.position;
            }
        }

        prev_point=current_point;
    }



    if(start_connection->traversal_direction)
    {
        current_bezier->c2=current_bezier->p2=current_point->b.position;
        current_bezier->next=NULL;
    }
    else
    {
        current_bezier->c1=current_bezier->p1=current_point->b.position;
        current_bezier->prev=NULL;
    }


    for(current_connection=current_point->n.first_connection;current_connection;current_connection=current_connection->next) if(current_connection->point_index==prev_point)
    {
        current_connection->bezier_index=current_bezier;
        return;
    }

    puts("prev point not found in convert_border_chain_to_beziers");
}


void convert_image_to_bezier_curves(vectorizer_data * vd)
{
    point * current_node;
    node_connection * current_connection,*sc1,*sc2;

    remove_all_bezier_curves_from_image(vd);

    bezier_section * first_available_section=NULL;
    bezier_section * bs;


    uint32_t t=SDL_GetTicks();

    ///do perimeter straight lines
    for(current_node=vd->first_node;current_node;current_node=current_node->n.next)
    {
        if((current_node->b.fixed_horizontally)||(current_node->b.fixed_vertically)) for(current_connection=current_node->n.first_connection;current_connection;current_connection=current_connection->next)
        {
            if((current_connection->bezier_index==NULL)&&((current_connection->point_index->b.fixed_horizontally)||(current_connection->point_index->b.fixed_vertically)))convert_border_chain_to_beziers(vd,current_connection);
        }
    }

    ///do lines that terminate at either end
    for(current_node=vd->first_node;current_node;current_node=current_node->n.next)
    {
        find_smoothest_node_connections(current_node,&sc1,&sc2);

        for(current_connection=current_node->n.first_connection;current_connection;current_connection=current_connection->next)
        {
            if((current_connection->bezier_index==NULL)&&(current_connection!=sc1)&&(current_connection!=sc2))convert_complete_chain_to_bezier_curves(vd,current_connection,&first_available_section);
        }
    }

    ///do lines that effectively form a loop (node pass through on 2 deemed smoothable)
    for(current_node=vd->first_node;current_node;current_node=current_node->n.next)
    {
        for(current_connection=current_node->n.first_connection;current_connection;current_connection=current_connection->next)
        {
            if((current_connection->bezier_index==NULL)&&(current_connection->traversal_direction))///remove traversal_direction check
            {
                convert_complete_chain_to_bezier_curves(vd,current_connection,&first_available_section);
            }
        }
    }

    ///free() all available_sections
    while(first_available_section)
    {
        bs=first_available_section;
        first_available_section=bs->next;
        free(bs);
    }

    printf("bezier time: %u\n",SDL_GetTicks()-t);
}
