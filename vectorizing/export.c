#include "headers.h"

static void write_face_beziers(FILE * output_file,face * f)
{
    point *current_point,*prev_point;
    bezier *current_curve;
    node_connection *start_connection,*current_connection;
    char buffer[12];


    sprintf(buffer,"%x",rgb_hash_from_lab(f->colour)|255<<24);
    //fprintf(output_file,"<path fill=\"#%s\" d=\"M %.3f %.3f C ",buffer+2,current_point->b.position.x,current_point->b.position.y);

    fprintf(output_file,"<path fill-rule=\"evenodd\" fill=\"#%s\" d=\"",buffer+2);

    while(f)
    {
        current_connection=start_connection=get_face_start_connection(f);
        current_point=start_connection->parent_node;

        fprintf(output_file,"M %.3f %.3f C ",current_point->b.position.x,current_point->b.position.y);

        do
        {
            prev_point=current_point;
            current_curve=current_connection->bezier_index;
            current_point=current_connection->point_index;

            if(current_connection->traversal_direction)
            {
                while(current_curve)
                {
                    fprintf(output_file,"%.3f,%.3f %.3f,%.3f %.3f,%.3f "
                            ,current_curve->c1.x,current_curve->c1.y
                            ,current_curve->c2.x,current_curve->c2.y
                            ,current_curve->p2.x,current_curve->p2.y);

                    current_curve=current_curve->next;
                }
                while(current_point->b.is_link_point)
                {
                    prev_point=current_point;
                    current_point=current_point->l.next;
                }
            }
            else
            {
                while(current_curve)
                {
                    fprintf(output_file,"%.3f,%.3f %.3f,%.3f %.3f,%.3f "
                            ,current_curve->c2.x,current_curve->c2.y
                            ,current_curve->c1.x,current_curve->c1.y
                            ,current_curve->p1.x,current_curve->p1.y);

                    current_curve=current_curve->prev;
                }
                while(current_point->b.is_link_point)
                {
                    prev_point=current_point;
                    current_point=current_point->l.prev;
                }
            }

            current_connection=get_next_ccw_connection(current_point,prev_point);
        }
        while(current_connection!=start_connection);

        fprintf(output_file,"Z ");

        f=f->next_excapsulated;
    }


    fprintf(output_file,"\"/>\n");
}

static void write_face_boundary_beziers(FILE * output_file,node_connection * start_connection)
{
    bezier * current_curve;
    char buffer[12];

    ///assumes traversal_direction is 1

    //if((start_connection->in_face->perimeter)||(start_connection->out_face->perimeter))return;///not needed

    if((start_connection->in_face->perimeter)&&(start_connection->in_face->encapsulating_face==NULL))return;
    if((start_connection->out_face->perimeter)&&(start_connection->out_face->encapsulating_face==NULL))return;

    if(start_connection->in_face->perimeter)sprintf(buffer,"%x",rgb_hash_from_lab(start_connection->out_face->colour)|255<<24);
    else sprintf(buffer,"%x",rgb_hash_from_lab(start_connection->in_face->colour)|255<<24);

    fprintf(output_file,"<path stroke-width=\"2.0\" stroke=\"#%s\" fill=\"none\" d=\"M %.3f %.3f C ",buffer+2,start_connection->parent_node->b.position.x,start_connection->parent_node->b.position.y);

    current_curve=start_connection->bezier_index;

    while(current_curve)
    {
        fprintf(output_file,"%.3f,%.3f %.3f,%.3f %.3f,%.3f "
                ,current_curve->c1.x,current_curve->c1.y
                ,current_curve->c2.x,current_curve->c2.y
                ,current_curve->p2.x,current_curve->p2.y);

        current_curve=current_curve->next;
    }

    fprintf(output_file,"\"/>\n");
}


bool export_vector_image_as_svg(vectorizer_data * vd,char * filename)
{
    FILE * output_file;

    output_file=fopen(filename,"w");

    if(output_file==NULL)
    {
        puts("error opening file");
        return false;
    }

    fprintf(output_file,"<?xml version=\"1.0\" encoding=\"utf-8\"?>\n");
    fprintf(output_file,"<svg xmlns=\"http://www.w3.org/2000/svg\" xmlns:xlink=\"http://www.w3.org/1999/xlink\" xmlns:ev=\"http://www.w3.org/2001/xml-events\" width=\"%upx\" height=\"%upx\" >\n",vd->w,vd->h);

    shape *current_shape,*prev_shape;
    face *current_face;
    point * current_node;
    node_connection * current_connection;

    current_shape=vd->first_shape;
    prev_shape=NULL;

    if(vd->fix_exported_boundaries)
    {
        for(current_node=vd->first_node;current_node;current_node=current_node->n.next)
        {
            for(current_connection=current_node->n.first_connection;current_connection;current_connection=current_connection->next)
                if(current_connection->traversal_direction)write_face_boundary_beziers(output_file,current_connection);
        }
    }

    while(current_shape)
    {
        if(  (current_shape->parent==prev_shape)  ||  ((prev_shape)&&(prev_shape->next==current_shape))  )
        {
            for(current_face=current_shape->first_face;current_face;current_face=current_face->next_in_shape)
            {
                if(current_face->perimeter==0)write_face_beziers(output_file,current_face);
            }

            if(current_shape->first_child)
            {
                prev_shape=current_shape;
                current_shape=current_shape->first_child;
                continue;
            }
        }

        prev_shape=current_shape;
        if(current_shape->next) current_shape=current_shape->next;
        else current_shape=current_shape->parent;
    }



    fprintf(output_file,"</svg>");

    fclose(output_file);

    return true;
}

