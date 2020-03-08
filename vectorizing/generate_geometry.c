#include "headers.h"

static bool generate_line_from_faces(vectorizer_data * vd,face ** pixel_faces,point ** points,diagonal_sample_connection_type * diagonals,uint32_t dir,vec2i p,int32_t w,int32_t h,point * first_point,face * f_face,face * b_face)
{
    //#warning may need bool return to indicate whether complete loop is achieved (first point is rejoined to) for when executing on diagonal starts
    ///traversal_direction is always 1


    uint32_t delta_dir,index;
    point *current_point,*prev_point;
    diagonal_sample_connection_type diag;
    vec2i s1,s2;

    vec2i sample_offsets[4]={{0,-1},{-1,-1},{-1,0},{0,0}};///sample offsets
    vec2i direction_offsets[4]={{1,0},{0,-1},{-1,0},{0,1}};///direction offsets

    uint32_t tl_br_directions[4]={3,2,1,0};
    uint32_t tr_bl_directions[4]={1,0,3,2};

    vec2f diagonal_point_offset[4]={{.x=0.075,.y=-0.075},{.x=-0.075,.y=0.075},{.x=-0.075,.y=-0.075},{.x=0.075,.y=0.075}};///tl_br(01,23),tr_bl(12,30)
    ///above distance must create distance between points greater than exclusion range (sqrt(er_2)) in test_if_point_position_invalid

    prev_point=first_point;

//    if(f_face->start_point==NULL)f_face->start_point=first_point;
//    if(b_face->start_point==NULL)b_face->start_point=first_point;

    if(first_point->b.is_link_point)puts("ERROR: START OF LINE SHOULD BE NODE");

    while(1)
    {
        p=v2i_add(p,direction_offsets[dir]);
        if((p.x==0)||(p.y==0)||(p.x==w)||(p.y==h))diag=NO_DIAGONAL;
        else diag=diagonals[p.y*w+p.x];
        index=(p.y*(w+1)+p.x)*2;

        //if((p.x==0)||(p.y==0))puts("AAA");

        if(diag==TL_BR_DIAGONAL)
        {
            index+=((dir==0)||(dir==1));
            if((current_point=points[index]))
            {
                connect_point_to_last(vd,prev_point,current_point,f_face,b_face);
                return current_point==first_point;///SHOULD ALWAYS BE TRUE?
            }
            else
            {
                points[index]=current_point=create_link_point(vd,v2f_add(v2i_to_v2f(p),diagonal_point_offset[0+((dir==0)||(dir==1))]));
                connect_point_to_last(vd,prev_point,current_point,f_face,b_face);

                dir=tl_br_directions[dir];
            }
        }
        else if(diag==TR_BL_DIAGONAL)
        {
            index+=((dir==1)||(dir==2));
            if((current_point=points[index]))
            {
                connect_point_to_last(vd,prev_point,current_point,f_face,b_face);
                return current_point==first_point;///SHOULD ALWAYS BE TRUE?
            }
            else
            {
                points[index]=current_point=create_link_point(vd,v2f_add(v2i_to_v2f(p),diagonal_point_offset[2+((dir==1)||(dir==2))]));
                connect_point_to_last(vd,prev_point,current_point,f_face,b_face);

                dir=tr_bl_directions[dir];
            }
        }
        else if((current_point=points[index]))
        {
            connect_point_to_last(vd,prev_point,current_point,f_face,b_face);
            return current_point==first_point;
        }
        else
        {
            for(delta_dir=3;delta_dir<6;delta_dir++)///3 is equivalent to -1 (mod 4 arithmetic) and 5 to +1
            {
                s1=v2i_add(p,sample_offsets[(dir+delta_dir+3)&3]);/// +3 is to sample "prev" location ("outside" of line)
                s2=v2i_add(p,sample_offsets[(dir+delta_dir)&3]);

                if((pixel_faces[s1.y*w+s1.x]==f_face)&&(pixel_faces[s2.y*w+s2.x]==b_face))
                {
                    ///this is an appropriate offset
                    points[index]=current_point=create_link_point(vd,v2i_to_v2f(p));
                    connect_point_to_last(vd,prev_point,current_point,f_face,b_face);

                    dir=(dir+delta_dir)&3;
                    break;
                }
            }

            if(delta_dir==6)///appropriate_next direction not found
            {
                points[index]=current_point=create_node_point(vd,v2i_to_v2f(p));
                connect_point_to_last(vd,prev_point,current_point,f_face,b_face);
                return false;
            }

            ///if reaching this point in execution then no appropriate direction was found, ergo this point is a node and is end of this chain
        }

        prev_point=current_point;
    }
}


static void set_all_link_faces(vectorizer_data * vd)
{
    point *current_point,*current_node;
    node_connection * current_connection;
    face *f_face,*b_face;
}

void generate_geometry_from_cluster_indices(vectorizer_data * vd)
{
    uint32_t i,j;
    int32_t w,h;
    face *f1,*f2,*outermost_face;
    vec2i p;
    point *first_point,*current_point,*prev_point;
    node_connection * current_connection;
    diagonal_sample_connection_type diag;

    w=vd->w;
    h=vd->h;

    diagonal_sample_connection_type * diagonals=vd->diagonals;
    face ** pixel_faces=vd->pixel_faces;
    point ** points=malloc(sizeof(point*)*(w+1)*(h+1)*2);

    remove_all_points_from_image(vd);
    remove_all_perimeter_faces_from_image_and_reset_heirarchy_data(vd);///reset faces used to denote clusters and remove perimeters so they can be created again
    remove_all_shapes_from_image(vd);



    outermost_face=create_face(vd);
    outermost_face->perimeter=true;




    for(i=0;i<((w+1)*(h+1)*2);i++)points[i]=NULL;

    prev_point=first_point=create_node_point(vd,(vec2f){.x=0.0,.y=0.0});///no need to set points array value as corner points can never be reached by geometry creation
    ///need to set face start point at lowest index (y*w+x) point to ensure it is on non-perimeter bound of that face (pixel_faces[0]->start_point   above)

    first_point->b.fixed_horizontally=true;
    first_point->b.fixed_vertically=true;

    p.y=0;

    for(p.x=1;p.x<w;p.x++)
    {
        if(pixel_faces[p.x-1]==pixel_faces[p.x])points[(p.y*(w+1)+p.x)*2]=current_point=create_link_point(vd,v2i_to_v2f(p));
        else points[(p.y*(w+1)+p.x)*2]=current_point=create_node_point(vd,v2i_to_v2f(p));

        connect_point_to_last(vd,prev_point,current_point,pixel_faces[p.x-1],outermost_face);
        current_point->b.fixed_vertically=true;
        prev_point=current_point;
    }

    current_point=create_link_point(vd,v2i_to_v2f(p));///no need to set points array value as corner points can never be reached by geometry creation
    connect_point_to_last(vd,prev_point,current_point,pixel_faces[w-1],outermost_face);
    current_point->b.fixed_horizontally=true;
    current_point->b.fixed_vertically=true;
    prev_point=current_point;

    for(p.y=1;p.y<h;p.y++)
    {
        if(pixel_faces[p.y*w-1]==pixel_faces[(p.y+1)*w-1]) points[(p.y*(w+1)+p.x)*2]=current_point=create_link_point(vd,v2i_to_v2f(p));
        else points[(p.y*(w+1)+p.x)*2]=current_point=create_node_point(vd,v2i_to_v2f(p));

        connect_point_to_last(vd,prev_point,current_point,pixel_faces[p.y*w-1],outermost_face);
        current_point->b.fixed_horizontally=true;
        prev_point=current_point;
    }

    current_point=create_link_point(vd,v2i_to_v2f(p));///no need to set points array value as corner points can never be reached by geometry creation
    connect_point_to_last(vd,prev_point,current_point,pixel_faces[w*h-1],outermost_face);
    current_point->b.fixed_horizontally=true;
    current_point->b.fixed_vertically=true;
    prev_point=current_point;

    for(p.x=w-1;p.x>0;p.x--)
    {
        if(pixel_faces[(h-1)*w+p.x]==pixel_faces[(h-1)*w+p.x-1]) points[(p.y*(w+1)+p.x)*2]=current_point=create_link_point(vd,v2i_to_v2f(p));
        else points[(p.y*(w+1)+p.x)*2]=current_point=create_node_point(vd,v2i_to_v2f(p));

        connect_point_to_last(vd,prev_point,current_point,pixel_faces[(h-1)*w+p.x],outermost_face);
        current_point->b.fixed_vertically=true;
        prev_point=current_point;
    }

    current_point=create_link_point(vd,v2i_to_v2f(p));///no need to set points array value as corner points can never be reached by geometry creation
    connect_point_to_last(vd,prev_point,current_point,pixel_faces[(h-1)*w],outermost_face);
    current_point->b.fixed_horizontally=true;
    current_point->b.fixed_vertically=true;
    prev_point=current_point;

    for(p.y=h-1;p.y>0;p.y--)
    {
        if(pixel_faces[p.y*w]==pixel_faces[(p.y-1)*w]) points[(p.y*(w+1)+p.x)*2]=current_point=create_link_point(vd,v2i_to_v2f(p));
        else points[(p.y*(w+1)+p.x)*2]=current_point=create_node_point(vd,v2i_to_v2f(p));

        connect_point_to_last(vd,prev_point,current_point,pixel_faces[p.y*w],outermost_face);
        current_point->b.fixed_horizontally=true;
        prev_point=current_point;
    }

    connect_point_to_last(vd,prev_point,first_point,pixel_faces[0],outermost_face);


    vec2i sample_offsets[4]={{0,-1},{-1,-1},{-1,0},{0,0}};///sample offsets
    vec2i direction_offsets[4]={{1,0},{0,-1},{-1,0},{0,1}};///direction offsets

    vec2f diagonal_point_offset[4]={{.x=0.075,.y=-0.075},{.x=-0.075,.y=0.075},{.x=-0.075,.y=-0.075},{.x=0.075,.y=0.075}};///tl_br(01,23),tr_bl(12,30)
    ///above distance must create distance between points greater than exclusion range (sqrt(er_2)) in test_if_point_position_invalid


    ///for all possible point locations look at every possible direction and start line like that, start as node and connect to any points encountered on NO_DIAGONAL locations, changing them to nodes
    ///(if not NO_DIAGONAL create new point as "normal")

    #warning test cluster made up entirely of single pixel wide diagonals with the "same" cluster either side (changing validity test as required) to ensure creation works properly and creates every line
    #warning check this can actually handle 4-way nodes (specifically those with 1 segment length edges

    for(p.y=1;p.y<h;p.y++)
    //for(p.y=h-1;p.y>0;p.y--)
    {
        for(p.x=1;p.x<w;p.x++)
        //for(p.x=w-1;p.x>0;p.x--)
        {
            diag=diagonals[p.y*w+p.x];
            if((diag==TL_BR_DIAGONAL)||(diag==TR_BL_DIAGONAL))
            {
                ///do all 4 directions using 2 points as base connected as appropriate
                ///don't need point array to be set for this version

                #warning are these ever actually called
                #warning provide offset to created point

                if(points[(p.y*(w+1)+p.x)*2]==NULL)
                {
                    points[(p.y*(w+1)+p.x)*2]=current_point=create_node_point(vd,v2f_add(v2i_to_v2f(p),diagonal_point_offset[2*(diag==TR_BL_DIAGONAL)+0]));

                    if(diag==TL_BR_DIAGONAL)
                    {
                        f1=pixel_faces[p.y*w+p.x];
                        f2=pixel_faces[(p.y-1)*w+p.x];

                        if(!generate_line_from_faces(vd,pixel_faces,points,diagonals,0,p,w,h,current_point,f1,f2))
                            generate_line_from_faces(vd,pixel_faces,points,diagonals,1,p,w,h,current_point,f2,f1);
                    }
                    if(diag==TR_BL_DIAGONAL)
                    {
                        f1=pixel_faces[(p.y-1)*w+p.x];
                        f2=pixel_faces[(p.y-1)*w+p.x-1];

                        if(!generate_line_from_faces(vd,pixel_faces,points,diagonals,1,p,w,h,current_point,f1,f2))
                            generate_line_from_faces(vd,pixel_faces,points,diagonals,2,p,w,h,current_point,f2,f1);
                    }
                }

                if(points[(p.y*(w+1)+p.x)*2+1]==NULL)
                {
                    points[(p.y*(w+1)+p.x)*2+1]=current_point=create_node_point(vd,v2f_add(v2i_to_v2f(p),diagonal_point_offset[2*(diag==TR_BL_DIAGONAL)+1]));

                    if(diag==TL_BR_DIAGONAL)
                    {
                        f1=pixel_faces[(p.y-1)*w+p.x-1];
                        f2=pixel_faces[p.y*w+p.x-1];

                        if(!generate_line_from_faces(vd,pixel_faces,points,diagonals,2,p,w,h,current_point,f1,f2))
                            generate_line_from_faces(vd,pixel_faces,points,diagonals,3,p,w,h,current_point,f2,f1);
                    }
                    if(diag==TR_BL_DIAGONAL)
                    {
                        f1=pixel_faces[p.y*w+p.x];
                        f2=pixel_faces[(p.y-1)*w+p.x];

                        if(!generate_line_from_faces(vd,pixel_faces,points,diagonals,0,p,w,h,current_point,f1,f2))
                            generate_line_from_faces(vd,pixel_faces,points,diagonals,3,p,w,h,current_point,f2,f1);
                    }
                }
            }
            else /// == NO_DIAGONAL or all_daigonal (same cluster shouldnt be possible)
            {
                current_point=points[(p.y*(w+1)+p.x)*2];

                /// following conditional may be broken under some circumstances (why only when already node ?)
                ///may work because line teminates to node when end of specific boundary (between 2 specific faces) reached so left as node to be assessed later

                if((current_point==NULL)||( ! current_point->b.is_link_point))
                {
                    f1=pixel_faces[(p.y+sample_offsets[3].y)*w+p.x+sample_offsets[3].x];
                    for(i=0;i<4;i++)
                    {
                        f2=pixel_faces[(p.y+sample_offsets[i].y)*w+p.x+sample_offsets[i].x];
                        if(f1!=f2)
                        {
                            if(current_point)
                            {
                                for(current_connection=current_point->n.first_connection;current_connection;current_connection=current_connection->next)
                                {
                                    if(v2f_dot(v2i_to_v2f(direction_offsets[i]),v2f_sub(current_connection->point_index->b.position,current_point->b.position))>0.5)break;///dp would be 1.0 if in same direction (which this is trying to detect) 0 or -1 otherwise
                                }
                            }
                            else
                            {
                                points[(p.y*(w+1)+p.x)*2]=current_point=create_node_point(vd,v2i_to_v2f(p));

                                current_connection=NULL;///for test, to match above when passing
                            }

                            if(current_connection==NULL)///connection in this direction not already extant
                            {
                                ///progress chain (as below, using ci1 & ci2) until a nonzero point is found
                                generate_line_from_faces(vd,pixel_faces,points,diagonals,i,p,w,h,current_point,f1,f2);
                            }
                        }
                        f1=f2;
                    }
                }
            }
        }
    }


    identify_shapes(vd,outermost_face,first_point);


    free(points);

    test_for_invalid_nodes(vd);
    remove_all_redundant_nodes(vd);
}
