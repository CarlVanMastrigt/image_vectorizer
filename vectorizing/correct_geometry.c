#include "headers.h"

#define ROTATION_SUBDIVISION_LEVELS 4
#define ROTATION_SUBDIVISION_COUNT 81

/// ROTATION_SUBDIVISION_COUNT = 3^ROTATION_SUBDIVISION_LEVELS
static matrix2f subdivision_rotation_matrices[ROTATION_SUBDIVISION_COUNT];

void initialise_geometry_correction_matrices(void)
{
    int i;
    float angle;

    for(i=0;i<ROTATION_SUBDIVISION_COUNT;i++)
    {
        ///check up to 1/16 rotation either direction
        angle=0.25*PI * ((float)(i-(ROTATION_SUBDIVISION_COUNT>>1)))/((float)ROTATION_SUBDIVISION_COUNT);

        subdivision_rotation_matrices[i]=m2f_rotation_matrix(angle);
    }
}



static inline float cubic_interpolate_values(float v0,float v1,float v2,float v3,float x)
{
    return (((3.0*(v1-v2)+v3-v0)*x + 2.0*v0-5.0*v1+4.0*v2-v3)*x+v2-v0)*x*0.5+v1;
}


pixel_lab cubic_interpolate_pixel_lab(pixel_lab * v,vec2f p,int32_t w,int32_t h)///subtract/add pixel centre offset before calling this function
{
    pixel_lab result;
    vec2i s0,s1,s2,s3;


    #warning possibly take gradient using only

    p=v2f_modf(p,&s1);

    s0=v2i_sub(s1,(vec2i){1,1});
    s2=v2i_add(s1,(vec2i){1,1});
    s3=v2i_add(s1,(vec2i){2,2});

    s0=v2i_clamp(s0,(vec2i){0,0},(vec2i){.x=w-1,.y=h-1});
    s1=v2i_clamp(s1,(vec2i){0,0},(vec2i){.x=w-1,.y=h-1});
    s2=v2i_clamp(s2,(vec2i){0,0},(vec2i){.x=w-1,.y=h-1});
    s3=v2i_clamp(s3,(vec2i){0,0},(vec2i){.x=w-1,.y=h-1});

    result.l=cubic_interpolate_values
    (
        cubic_interpolate_values(v[s0.y*w+s0.x].l,v[s0.y*w+s1.x].l,v[s0.y*w+s2.x].l,v[s0.y*w+s3.x].l,p.x),
        cubic_interpolate_values(v[s1.y*w+s0.x].l,v[s1.y*w+s1.x].l,v[s1.y*w+s2.x].l,v[s1.y*w+s3.x].l,p.x),
        cubic_interpolate_values(v[s2.y*w+s0.x].l,v[s2.y*w+s1.x].l,v[s2.y*w+s2.x].l,v[s2.y*w+s3.x].l,p.x),
        cubic_interpolate_values(v[s3.y*w+s0.x].l,v[s3.y*w+s1.x].l,v[s3.y*w+s2.x].l,v[s3.y*w+s3.x].l,p.x),
        p.y
    );

    result.a=cubic_interpolate_values
    (
        cubic_interpolate_values(v[s0.y*w+s0.x].a,v[s0.y*w+s1.x].a,v[s0.y*w+s2.x].a,v[s0.y*w+s3.x].a,p.x),
        cubic_interpolate_values(v[s1.y*w+s0.x].a,v[s1.y*w+s1.x].a,v[s1.y*w+s2.x].a,v[s1.y*w+s3.x].a,p.x),
        cubic_interpolate_values(v[s2.y*w+s0.x].a,v[s2.y*w+s1.x].a,v[s2.y*w+s2.x].a,v[s2.y*w+s3.x].a,p.x),
        cubic_interpolate_values(v[s3.y*w+s0.x].a,v[s3.y*w+s1.x].a,v[s3.y*w+s2.x].a,v[s3.y*w+s3.x].a,p.x),
        p.y
    );

    result.b=cubic_interpolate_values
    (
        cubic_interpolate_values(v[s0.y*w+s0.x].b,v[s0.y*w+s1.x].b,v[s0.y*w+s2.x].b,v[s0.y*w+s3.x].b,p.x),
        cubic_interpolate_values(v[s1.y*w+s0.x].b,v[s1.y*w+s1.x].b,v[s1.y*w+s2.x].b,v[s1.y*w+s3.x].b,p.x),
        cubic_interpolate_values(v[s2.y*w+s0.x].b,v[s2.y*w+s1.x].b,v[s2.y*w+s2.x].b,v[s2.y*w+s3.x].b,p.x),
        cubic_interpolate_values(v[s3.y*w+s0.x].b,v[s3.y*w+s1.x].b,v[s3.y*w+s2.x].b,v[s3.y*w+s3.x].b,p.x),
        p.y
    );



    return result;
}

static inline float linear_interpolate_values(float v0,float v1,float x)
{
    return v0+x*(v1-v0);
}

pixel_lab linear_interpolate_pixel_lab(pixel_lab * v,vec2f p,int32_t w,int32_t h)
{
    pixel_lab result;//,s_tl,s_tr,s_bl,s_br;
    vec2i s0,s1;

    p=v2f_modf(p,&s0);

    s1=v2i_add(s0,(vec2i){1,1});

//    s0=v2i_min((vec2i){.x=w-1,.y=h-1},v2i_max((vec2i){0,0},s0));
//    s1=v2i_min((vec2i){.x=w-1,.y=h-1},v2i_max((vec2i){0,0},s1));

    s0=v2i_clamp(s0,(vec2i){0,0},(vec2i){.x=w-1,.y=h-1});
    s1=v2i_clamp(s0,(vec2i){0,0},(vec2i){.x=w-1,.y=h-1});


    result.l=linear_interpolate_values
    (
        linear_interpolate_values(v[s0.y*w+s0.x].l,v[s0.y*w+s1.x].l,p.x),
        linear_interpolate_values(v[s1.y*w+s0.x].l,v[s1.y*w+s1.x].l,p.x),
        p.y
    );

    result.a=linear_interpolate_values
    (
        linear_interpolate_values(v[s0.y*w+s0.x].a,v[s0.y*w+s1.x].a,p.x),
        linear_interpolate_values(v[s1.y*w+s0.x].a,v[s1.y*w+s1.x].a,p.x),
        p.y
    );

    result.b=linear_interpolate_values
    (
        linear_interpolate_values(v[s0.y*w+s0.x].b,v[s0.y*w+s1.x].b,p.x),
        linear_interpolate_values(v[s1.y*w+s0.x].b,v[s1.y*w+s1.x].b,p.x),
        p.y
    );

    return result;
}













static inline float compare_direction_to_sampled_gradient(vec2f d,pixel_lab h_grad,pixel_lab v_grad)
{
    float v,v_total=0.0;

    v=h_grad.l*d.x+v_grad.l*d.y;
    v_total+=v*v;

    v=h_grad.a*d.x+v_grad.a*d.y;
    v_total+=v*v;

    v=h_grad.b*d.x+v_grad.b*d.y;
    v_total+=v*v;

    return v_total;
}


static void correct_segment_by_sampling_image_gradient(vectorizer_data * vd,point * p1,point * p2)
{
    vec2f d,offset;
    float v,v_best;
    uint32_t m,o,o_best;
    pixel_lab h_grad,v_grad;
    node_connection * nc;

    if((p1->b.fixed_horizontally || p1->b.fixed_vertically)&&(p2->b.fixed_horizontally || p2->b.fixed_vertically))return;///dont alter edge segments

    ///if segment is unsmoothable then dont correct
    if(! p1->b.is_link_point) for(nc=p1->n.first_connection;nc;nc=nc->next) if((nc->point_index==p2)&&(! nc->smoothable)) return;
    if(! p2->b.is_link_point) for(nc=p2->n.first_connection;nc;nc=nc->next) if((nc->point_index==p1)&&(! nc->smoothable)) return;

    offset=v2f_sub(p2->b.position,p1->b.position);
    d=v2f_orth(offset);

    h_grad=cubic_interpolate_pixel_lab(vd->h_grad,v2f_sub(v2f_mid(p1->b.position,p2->b.position),(vec2f){.x=1.0,.y=0.5}),vd->w-1,vd->h);
    v_grad=cubic_interpolate_pixel_lab(vd->v_grad,v2f_sub(v2f_mid(p1->b.position,p2->b.position),(vec2f){.x=0.5,.y=1.0}),vd->w,vd->h-1);

    o_best=ROTATION_SUBDIVISION_COUNT>>1;
    v_best=compare_direction_to_sampled_gradient(d,h_grad,v_grad);

    for(m=ROTATION_SUBDIVISION_COUNT/3; m ;m/=3)///div by constant 3 should be optimised into simple/bit-ops
    {
        o=o_best;

        v=compare_direction_to_sampled_gradient(m2f_vec2f_multiply(subdivision_rotation_matrices[o+m],d),h_grad,v_grad);
        if(v>v_best)v_best=v,o_best=o+m;

        v=compare_direction_to_sampled_gradient(m2f_vec2f_multiply(subdivision_rotation_matrices[o-m],d),h_grad,v_grad);
        if(v>v_best)v_best=v,o_best=o-m;
    }

    offset=v2f_mult(v2f_sub(m2f_vec2f_multiply(subdivision_rotation_matrices[o_best],offset),offset),0.25);

    p1->b.modified_position=v2f_sub(p1->b.modified_position,offset);///sigma to avoid div by 0
    p2->b.modified_position=v2f_add(p2->b.modified_position,offset);///sigma to avoid div by 0
}

void correct_geometry_by_sampling_image_gradient(vectorizer_data * vd)
{
    point *current_point,*current_node;
    node_connection *current_connection;

    prepare_all_point_positions_for_modification(vd);

    for(current_node=vd->first_node;current_node;current_node=current_node->n.next)
    {
        for(current_connection=current_node->n.first_connection;current_connection;current_connection=current_connection->next)
        {
            if(current_connection->traversal_direction)
            {
                correct_segment_by_sampling_image_gradient(vd,current_node,current_connection->point_index);

                for(current_point=current_connection->point_index;current_point->b.is_link_point;current_point=current_point->l.next)
                {
                    correct_segment_by_sampling_image_gradient(vd,current_point,current_point->l.next);
                }
            }
        }
    }

    update_all_point_positions_from_modification(vd);
}


void geometry_correction(vectorizer_data * vd)
{
    int i,j;

    for(i=0;i<vd->initial_geometry_correction_passes;i++)
    {
        store_point_old_positions(vd);

        for(j=0;j<8;j++)
        {
            /// need to ensure division by 0 (or even just small values) is avoided in all these functions

            set_connections_smoothable_status(vd);

            correct_geometry_by_sampling_image_gradient(vd);
            low_pass_filter_geometry(vd);
            adjust_segment_lengths(vd);
            low_pass_filter_geometry(vd);
        }

        /// do "stitching" ? (combine node connections w/ very small angle between),possibly as part of correction (update_point_positions) step

        update_point_positions(vd);
    }

    relocate_loop_nodes_to_flattest_spot(vd);


    /// detect corners (on per chain basis starting with best corner (recursive, check over sections bounded by nodes and better (up-stack) corners, will allow corners to be within 2 segments of each other rather than 4)
    /// relocate loop nodes to corners as necessary/applicable

    ///perform "subsequent" smoothing passes
}
