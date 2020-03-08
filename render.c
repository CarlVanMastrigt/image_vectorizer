#include "headers.h"

static GLuint screen_quad_array;

static GLuint unit_box_array;

static GLuint shader_supersampling;

static GLuint shader_image_display;

static GLuint shader_line;

static GLuint shader_point;

static GLuint shader_bezier;

static GLuint shader_line_face;

static GLuint shader_bezier_face;

void initialise_misc_render_gl_variables(void)
{
    /// screen quad
    float screen_points[8]={-1,-1,1,-1,-1,1,1,1};
    float unit_box_points[8]={0,0,1,0,0,1,1,1};

    screen_quad_array=0;
    glGenBuffers_ptr(1, &screen_quad_array);
    glBindBuffer_ptr(GL_ARRAY_BUFFER, screen_quad_array);
    glBufferData_ptr(GL_ARRAY_BUFFER, sizeof(float)*8,screen_points, GL_STATIC_DRAW);
    glBindBuffer_ptr(GL_ARRAY_BUFFER, 0);

    unit_box_array=0;
    glGenBuffers_ptr(1, &unit_box_array);
    glBindBuffer_ptr(GL_ARRAY_BUFFER, unit_box_array);
    glBufferData_ptr(GL_ARRAY_BUFFER, sizeof(float)*8,unit_box_points, GL_STATIC_DRAW);
    glBindBuffer_ptr(GL_ARRAY_BUFFER, 0);

    shader_supersampling=initialise_shader_program(NULL,NULL,NULL,"./shaders/supersampling_frag.glsl");
    glUseProgram_ptr(shader_supersampling);
    glUniform1i_ptr(glGetUniformLocation_ptr(shader_supersampling,"colour"),0);
    glUseProgram_ptr(0);

    shader_image_display=initialise_shader_program(NULL,"./shaders/image_display_vert.glsl",NULL,"./shaders/image_display_frag.glsl");
    glUseProgram_ptr(shader_image_display);
    glUniform1i_ptr(glGetUniformLocation_ptr(shader_image_display,"image"),0);
    glUseProgram_ptr(0);

    shader_line=initialise_shader_program(NULL,"./shaders/line_vert.glsl",NULL,"./shaders/line_frag.glsl");

    shader_point=initialise_shader_program(NULL,"./shaders/point_vert.glsl",NULL,"./shaders/point_frag.glsl");

    shader_bezier=initialise_shader_program(NULL,"./shaders/bezier_vert.glsl",NULL,"./shaders/bezier_frag.glsl");

    shader_line_face=initialise_shader_program(NULL,"./shaders/line_face_vert.glsl",NULL,"./shaders/line_face_frag.glsl");

    shader_bezier_face=initialise_shader_program(NULL,"./shaders/bezier_face_vert.glsl",NULL,"./shaders/bezier_face_frag.glsl");
}

void screen_quad(void)
{
    glBindBuffer_ptr(GL_ARRAY_BUFFER, screen_quad_array);
    glEnableVertexAttribArray_ptr(0);
    glVertexAttribPointer_ptr(0, 2, GL_FLOAT, GL_FALSE, 0, 0);

    glDrawArrays(GL_TRIANGLE_STRIP,0,4);


    glBindBuffer_ptr(GL_ARRAY_BUFFER,0);
    glDisableVertexAttribArray_ptr(0);
}

void unit_box(void)
{
    glBindBuffer_ptr(GL_ARRAY_BUFFER, unit_box_array);
    glEnableVertexAttribArray_ptr(0);
    glVertexAttribPointer_ptr(0, 2, GL_FLOAT, GL_FALSE, 0, 0);

    glDrawArrays(GL_TRIANGLE_STRIP,0,4);


    glBindBuffer_ptr(GL_ARRAY_BUFFER,0);
    glDisableVertexAttribArray_ptr(0);
}

static void supersample_screen(config_data * cd)
{
    glUseProgram_ptr(shader_supersampling);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D,get_colour_texture());

    glUniform1i_ptr(glGetUniformLocation_ptr(shader_supersampling,"supersample_magnitude"),cd->supersample_magnitude);

    screen_quad();

    glUseProgram_ptr(0);
}

static void render_overlap(config_data * cd,vectorizer_data * vd)
{
    float inv_w,inv_h;
    point * current_node;
    bezier * current_bezier;
    node_connection * current_connection;

    bind_colour_framebuffer();

    float x=(((float)vd->w))/((float)(cd->screen_width*cd->supersample_magnitude))*cd->zoom;
    float y=(((float)vd->h))/((float)(cd->screen_height*cd->supersample_magnitude))*cd->zoom;

    float x_off=(2.0*cd->x*cd->zoom)/((float)(cd->screen_width*cd->supersample_magnitude));
    float y_off=(-2.0*cd->y*cd->zoom)/((float)(cd->screen_height*cd->supersample_magnitude));


    glUseProgram_ptr(shader_image_display);

    glActiveTexture(GL_TEXTURE0);

    float rw,rh;
    rw=((float)(vd->w))/((float)(vd->texture_w));
    rh=((float)(vd->h))/((float)(vd->texture_h));


    if((vd->render_mode>=VECTORIZER_RENDER_MODE_IMAGE)&&(vd->render_mode<=VECTORIZER_RENDER_MODE_CLUSTERISED))
    {
        glBindTexture(GL_TEXTURE_2D,vd->images[vd->render_mode-1]);
        glUniform4f_ptr(glGetUniformLocation_ptr(shader_image_display,"coords"),x,-y,x_off,y_off);
        glUniform2f_ptr(glGetUniformLocation_ptr(shader_image_display,"relative_size"),rw,rh);
        screen_quad();
    }


    x_off=(2.0*cd->x*cd->zoom)/((float)(cd->screen_width*cd->supersample_magnitude));
    y_off=(-2.0*cd->y*cd->zoom)/((float)(cd->screen_height*cd->supersample_magnitude));


    if((vd->can_render_lines)&&(vd->render_lines))
    {
        glUseProgram_ptr(shader_line);
        glUniform3f_ptr(glGetUniformLocation_ptr(shader_line,"c"),0,1,0);

        glLineWidth((float)cd->supersample_magnitude);

        glUniform4f_ptr(glGetUniformLocation_ptr(shader_line,"coords"),x,-y,x_off,y_off);

        glBindBuffer_ptr(GL_ARRAY_BUFFER, vd->line_display_array);
        glEnableVertexAttribArray_ptr(0);
        glVertexAttribPointer_ptr(0, 2, GL_FLOAT, GL_FALSE, 0, 0);

        glDrawArrays(GL_LINES,0,vd->display_line_count*2);

        glUseProgram_ptr(shader_point);

        glEnable(GL_POINT_SMOOTH);

        glPointSize(4.0*((float)cd->supersample_magnitude));

        glUniform4f_ptr(glGetUniformLocation_ptr(shader_point,"coords"),x,-y,x_off,y_off);


        glBindBuffer_ptr(GL_ARRAY_BUFFER, vd->point_display_array);
        glEnableVertexAttribArray_ptr(0);
        glVertexAttribPointer_ptr(0, 2, GL_FLOAT, GL_FALSE, 0, 0);

        glDrawArrays(GL_POINTS,0,vd->display_point_count);

        glDisable(GL_POINT_SMOOTH);
    }

    if((vd->can_render_curves)&&(vd->render_curves))
    {
        glUseProgram_ptr(shader_bezier);

        glUniform4f_ptr(glGetUniformLocation_ptr(shader_bezier,"coords"),x,-y,x_off,y_off);

        inv_w=1.0/((float)(vd->w));
        inv_h=1.0/((float)(vd->h));

        float bezier_data[8];

        for(current_node=vd->first_node;current_node;current_node=current_node->n.next)
        {
            for(current_connection=current_node->n.first_connection;current_connection;current_connection=current_connection->next)
            {
                if(current_connection->traversal_direction)
                {
                    for(current_bezier=current_connection->bezier_index;current_bezier;current_bezier=current_bezier->next)
                    {
                        bezier_data[0]=current_bezier->p1.x*inv_w;
                        bezier_data[1]=current_bezier->p1.y*inv_h;

                        bezier_data[2]=current_bezier->c1.x*inv_w;
                        bezier_data[3]=current_bezier->c1.y*inv_h;

                        bezier_data[4]=current_bezier->c2.x*inv_w;
                        bezier_data[5]=current_bezier->c2.y*inv_h;

                        bezier_data[6]=current_bezier->p2.x*inv_w;
                        bezier_data[7]=current_bezier->p2.y*inv_h;

                        glUniform2fv_ptr(glGetUniformLocation_ptr(shader_bezier,"data"),4,bezier_data);

                        glUniform1f_ptr(glGetUniformLocation_ptr(shader_bezier,"inverse_vertex_count"),1.0/((float)(current_bezier->num_sections*4)));

                        glDrawArrays(GL_LINE_STRIP,0,current_bezier->num_sections*4+1);
                    }
                }
            }
        }
    }


    glBindBuffer_ptr(GL_ARRAY_BUFFER,0);
    glDisableVertexAttribArray_ptr(0);

    glUseProgram_ptr(0);
}

static void render_line_faces(config_data * cd,vectorizer_data * vd)
{
    bind_colour_framebuffer();

    shape *current_shape,*prev_shape;
    face * current_face;

    float x=(((float)vd->w))/((float)(cd->screen_width*cd->supersample_magnitude))*cd->zoom;
    float y=(((float)vd->h))/((float)(cd->screen_height*cd->supersample_magnitude))*cd->zoom;

    float x_off=(2.0*cd->x*cd->zoom)/((float)(cd->screen_width*cd->supersample_magnitude));
    float y_off=(-2.0*cd->y*cd->zoom)/((float)(cd->screen_height*cd->supersample_magnitude));


    glUseProgram_ptr(shader_line_face);///render black box as starting background
    glUniform4f_ptr(glGetUniformLocation_ptr(shader_line_face,"coords"),x,-y,x_off,y_off);
    glUniform3f_ptr(glGetUniformLocation_ptr(shader_line_face,"c"),0.0,0.0,0.0);
    unit_box();

    glBindBuffer_ptr(GL_ARRAY_BUFFER, vd->face_line_display_array);
    glEnableVertexAttribArray_ptr(0);
    glVertexAttribPointer_ptr(0, 2, GL_FLOAT, GL_FALSE, 0, 0);

    float r,g,b;

    GLint colour_uniform_loc=glGetUniformLocation_ptr(shader_line_face,"c");

    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);

    current_shape=vd->first_shape;
    prev_shape=NULL;

    while(current_shape)
    {
        if(  (current_shape->parent==prev_shape)  ||  ((prev_shape)&&(prev_shape->next==current_shape))  )
        {
            current_face=current_shape->first_face;

            for(current_face=current_shape->first_face;current_face;current_face=current_face->next_in_shape)
            {
                if(current_face->perimeter)
                {
                    if(current_face->encapsulating_face)convert_cielab_to_rgb_float(current_face->encapsulating_face->colour,&r,&g,&b);
                    else continue;
                }
                else convert_cielab_to_rgb_float(current_face->colour,&r,&g,&b);

                glUniform3f_ptr(colour_uniform_loc,r,g,b);

                glDrawArrays(GL_TRIANGLE_FAN,current_face->line_render_offset,current_face->line_render_count);
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


    glDisable(GL_BLEND);


    glBindBuffer_ptr(GL_ARRAY_BUFFER,0);
    glDisableVertexAttribArray_ptr(0);

    glUseProgram_ptr(0);
}

static void render_bezier_faces(config_data * cd,vectorizer_data * vd)
{
    bind_colour_framebuffer();

    point *current,*prev;
    shape *current_shape,*prev_shape;
    node_connection *start_connection,*current_connection;
    uint32_t traversal_direction;

    bezier * current_bezier;
    face * current_face;

    float x=(((float)vd->w))/((float)(cd->screen_width*cd->supersample_magnitude))*cd->zoom;
    float y=(((float)vd->h))/((float)(cd->screen_height*cd->supersample_magnitude))*cd->zoom;

    float x_off=(2.0*cd->x*cd->zoom)/((float)(cd->screen_width*cd->supersample_magnitude));
    float y_off=(-2.0*cd->y*cd->zoom)/((float)(cd->screen_height*cd->supersample_magnitude));

    float inv_w=1.0/((float)(vd->w));
    float inv_h=1.0/((float)(vd->h));


    glUseProgram_ptr(shader_line_face);///render black box as starting background
    glUniform4f_ptr(glGetUniformLocation_ptr(shader_line_face,"coords"),x,-y,x_off,y_off);
    glUniform3f_ptr(glGetUniformLocation_ptr(shader_line_face,"c"),0.0,0.0,0.0);
    unit_box();


    glUseProgram_ptr(shader_bezier_face);

    glUniform4f_ptr(glGetUniformLocation_ptr(shader_bezier_face,"coords"),x,-y,x_off,y_off);

    GLint colour_uniform_loc=glGetUniformLocation_ptr(shader_bezier_face,"c");
    GLint points_uniform_loc=glGetUniformLocation_ptr(shader_bezier_face,"data");
    GLint inverse_count_uniform_loc=glGetUniformLocation_ptr(shader_bezier_face,"inverse_vertex_count");

    GLfloat control_points[10];

    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);

    float r,g,b;


    current_shape=vd->first_shape;
    prev_shape=NULL;

    while(current_shape)
    {
        if(  (current_shape->parent==prev_shape)  ||  ((prev_shape)&&(prev_shape->next==current_shape))  )
        {
            for(current_face=current_shape->first_face;current_face;current_face=current_face->next_in_shape)
            {
                if(current_face->perimeter)
                {
                    if(current_face->encapsulating_face)convert_cielab_to_rgb_float(current_face->encapsulating_face->colour,&r,&g,&b);
                    else continue;
                }
                else convert_cielab_to_rgb_float(current_face->colour,&r,&g,&b);

                glUniform3f_ptr(colour_uniform_loc,r,g,b);

                current_connection=start_connection=get_face_start_connection(current_face);

                current=start_connection->parent_node;

                control_points[8]=current->b.position.x*inv_w;///set start node uniform
                control_points[9]=current->b.position.y*inv_h;

                do
                {
                    prev=current;
                    current=current_connection->point_index;

                    current_bezier=current_connection->bezier_index;
                    traversal_direction=current_connection->traversal_direction;

                    ///render beziers for section
                    while(current_bezier)
                    {
                        control_points[(1-traversal_direction)*6+0]=current_bezier->p1.x*inv_w;
                        control_points[(1-traversal_direction)*6+1]=current_bezier->p1.y*inv_h;

                        control_points[(1-traversal_direction)*2+2]=current_bezier->c1.x*inv_w;
                        control_points[(1-traversal_direction)*2+3]=current_bezier->c1.y*inv_h;

                        control_points[traversal_direction*2+2]=current_bezier->c2.x*inv_w;
                        control_points[traversal_direction*2+3]=current_bezier->c2.y*inv_h;

                        control_points[traversal_direction*6+0]=current_bezier->p2.x*inv_w;
                        control_points[traversal_direction*6+1]=current_bezier->p2.y*inv_h;


                        glUniform2fv_ptr(points_uniform_loc,5,control_points);

                        glUniform1f_ptr(inverse_count_uniform_loc,1.0/((float)current_bezier->num_sections*4));

                        glDrawArrays(GL_TRIANGLE_FAN,0,current_bezier->num_sections*4+2);///+2 istead of +1 b/c 0th element reserved for fan start pos, (extra +1 is for point to segment difference, that is: the end point)

                        if(traversal_direction) current_bezier=current_bezier->next;
                        else current_bezier=current_bezier->prev;
                    }

                    while(current->b.is_link_point)
                    {
                        prev=current;
                        if(traversal_direction) current=current->l.next;
                        else current=current->l.prev;
                    }

                    current_connection=get_next_ccw_connection(current,prev);
                }
                while(current_connection!=start_connection);///loop over segments
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


    glDisable(GL_BLEND);


    glUseProgram_ptr(0);
}

void render_screen(config_data * cd,vectorizer_data * vd)
{
    glViewport(0,0,cd->screen_width*cd->supersample_magnitude,cd->screen_height*cd->supersample_magnitude);

    glClearColor(0.05,0.05,0.05,1.0);

    bind_colour_framebuffer();
    glClear(GL_COLOR_BUFFER_BIT);

    if(vd->render_mode<=VECTORIZER_RENDER_MODE_CLUSTERISED)render_overlap(cd,vd);
    if(vd->render_mode==VECTORIZER_RENDER_MODE_LINES)render_line_faces(cd,vd);
    if(vd->render_mode==VECTORIZER_RENDER_MODE_CURVES)render_bezier_faces(cd,vd);

    glViewport(0,0,cd->screen_width,cd->screen_height);

    glBindFramebuffer_ptr(GL_FRAMEBUFFER,0);
    glDrawBuffer(GL_BACK);

    supersample_screen(cd);

    get_gl_error();

}
