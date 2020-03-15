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

static GLuint screen_quad_array;
static GLuint screen_quad_vao;

static GLuint unit_quad_array;
static GLuint unit_quad_vao;

static GLuint line_render_array;
static uint32_t line_render_count;
static GLuint line_render_vao;

static GLuint point_render_array;
static uint32_t point_render_count;
static GLuint point_render_vao;

static GLuint face_line_render_array;
static GLuint face_line_render_vao;

static GLuint empty_vao;

static GLuint shader_supersampling;
static GLuint shader_image_display;
static GLuint shader_line;
static GLuint shader_point;
static GLuint shader_bezier;
static GLuint shader_line_face;
static GLuint shader_bezier_face;

void initialise_misc_render_gl_variables(gl_functions * glf)
{
    float screen_quad_points[8]={-1,-1,1,-1,-1,1,1,1};

    glf->glGenVertexArrays(1,&screen_quad_vao);
    glf->glBindVertexArray(screen_quad_vao);

    glf->glGenBuffers(1,&screen_quad_array);
    glf->glBindBuffer(GL_ARRAY_BUFFER,screen_quad_array);
    glf->glBufferData(GL_ARRAY_BUFFER,sizeof(float)*8,screen_quad_points, GL_STATIC_DRAW);

    glf->glEnableVertexAttribArray(0);
    glf->glVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,0,0);

    glf->glBindVertexArray(0);


    float unit_quad_points[8]={0,0,1,0,0,1,1,1};

    glf->glGenVertexArrays(1,&unit_quad_vao);
    glf->glBindVertexArray(unit_quad_vao);

    glf->glGenBuffers(1,&unit_quad_array);
    glf->glBindBuffer(GL_ARRAY_BUFFER,unit_quad_array);
    glf->glBufferData(GL_ARRAY_BUFFER,sizeof(float)*8,unit_quad_points, GL_STATIC_DRAW);

    glf->glEnableVertexAttribArray(0);
    glf->glVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,0,0);

    glf->glBindVertexArray(0);






    glf->glGenVertexArrays(1,&line_render_vao);
    glf->glBindVertexArray(line_render_vao);

    glf->glGenBuffers(1,&line_render_array);

    glf->glBindBuffer(GL_ARRAY_BUFFER,line_render_array);
    glf->glEnableVertexAttribArray(0);
    glf->glVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,0,0);

    glf->glBindVertexArray(0);






    glf->glGenVertexArrays(1,&empty_vao);




    glf->glGenVertexArrays(1,&point_render_vao);
    glf->glBindVertexArray(point_render_vao);

    glf->glGenBuffers(1,&point_render_array);

    glf->glBindBuffer(GL_ARRAY_BUFFER,point_render_array);
    glf->glEnableVertexAttribArray(0);
    glf->glVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,0,0);

    glf->glBindVertexArray(0);




    glf->glGenVertexArrays(1,&face_line_render_vao);
    glf->glBindVertexArray(face_line_render_vao);

    glf->glGenBuffers(1,&face_line_render_array);

    glf->glBindBuffer(GL_ARRAY_BUFFER,face_line_render_array);
    glf->glEnableVertexAttribArray(0);
    glf->glVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,0,0);

    glf->glBindVertexArray(0);






    shader_supersampling=initialise_shader_program(glf,NULL,"./shaders/generic_vert.glsl",NULL,"./shaders/supersampling_frag.glsl");
    glf->glUseProgram(shader_supersampling);
    glf->glUniform1i(glf->glGetUniformLocation(shader_supersampling,"colour"),0);
    glf->glUseProgram(0);

    shader_image_display=initialise_shader_program(glf,NULL,"./shaders/image_display_vert.glsl",NULL,"./shaders/image_display_frag.glsl");
    glf->glUseProgram(shader_image_display);
    glf->glUniform1i(glf->glGetUniformLocation(shader_image_display,"image"),0);
    glf->glUseProgram(0);

    shader_line=initialise_shader_program(glf,NULL,"./shaders/line_vert.glsl",NULL,"./shaders/line_frag.glsl");

    shader_point=initialise_shader_program(glf,NULL,"./shaders/point_vert.glsl",NULL,"./shaders/point_frag.glsl");

    shader_bezier=initialise_shader_program(glf,NULL,"./shaders/bezier_vert.glsl",NULL,"./shaders/bezier_frag.glsl");

    shader_line_face=initialise_shader_program(glf,NULL,"./shaders/line_face_vert.glsl",NULL,"./shaders/line_face_frag.glsl");

    shader_bezier_face=initialise_shader_program(glf,NULL,"./shaders/bezier_face_vert.glsl",NULL,"./shaders/bezier_face_frag.glsl");
}

static void screen_quad(gl_functions * glf)
{
    glf->glBindVertexArray(screen_quad_vao);
    glf->glDrawArrays(GL_TRIANGLE_STRIP,0,4);
    glf->glBindVertexArray(0);
}

static void unit_quad(gl_functions * glf)
{
    glf->glBindVertexArray(unit_quad_vao);
    glf->glDrawArrays(GL_TRIANGLE_STRIP,0,4);
    glf->glBindVertexArray(0);
}

static void upload_line_render_data(gl_functions * glf,vectorizer_data * vd)
{
    face *current_face;
    point *current_node,*current,*prev;
    node_connection *current_connection,*start_connection;

    float wf=1.0/((float)vd->w);
    float hf=1.0/((float)vd->h);

    uint32_t face_vertex_count;

    uint32_t point_space=256;
    GLfloat * display_point_buffer=malloc(sizeof(GLfloat)*2*point_space);

    uint32_t line_space=256;
    GLfloat * display_line_buffer=malloc(sizeof(GLfloat)*4*line_space);

    uint32_t face_space=256;
    GLfloat * display_face_buffer=malloc(sizeof(GLfloat)*2*face_space);///  2 floats per point, points used up to MAX_POINT_NODE_CONNECTIONS times (???) and lines used twice (face each side of line)

    line_render_count=0;
    point_render_count=0;

    for(current_node=vd->first_node;current_node;current_node=current_node->n.next)
    {
        if(point_render_count==point_space)display_point_buffer=realloc(display_point_buffer,sizeof(GLfloat)*2*(point_space*=2));

        display_point_buffer[point_render_count*2+0]=current_node->b.position.x*wf;
        display_point_buffer[point_render_count*2+1]=current_node->b.position.y*hf;
        point_render_count++;

        for(current_connection=current_node->n.first_connection;current_connection;current_connection=current_connection->next) if(current_connection->traversal_direction)
        {
            prev=current_node;
            current=current_connection->point_index;

            do
            {
                if(line_render_count==line_space)display_line_buffer=realloc(display_line_buffer,sizeof(GLfloat)*4*(line_space*=2));

                display_line_buffer[line_render_count*4+0]=prev->b.position.x*wf;
                display_line_buffer[line_render_count*4+1]=prev->b.position.y*hf;

                display_line_buffer[line_render_count*4+2]=current->b.position.x*wf;
                display_line_buffer[line_render_count*4+3]=current->b.position.y*hf;

                line_render_count++;

                prev=current;
                current=current->l.next;
            }
            while(prev->b.is_link_point);
        }
    }

    face_vertex_count=0;

    for(current_face=vd->first_face;current_face;current_face=current_face->next)
    {
        current_face->line_render_offset=face_vertex_count;

        current_connection=start_connection=get_face_start_connection(current_face);
        current=start_connection->parent_node;

        do
        {
            if(face_space==face_vertex_count)display_face_buffer=realloc(display_face_buffer,sizeof(GLfloat)*2*(face_space*=2));

            display_face_buffer[face_vertex_count*2+0]=current->b.position.x*wf;
            display_face_buffer[face_vertex_count*2+1]=current->b.position.y*hf;
            face_vertex_count++;

            prev=current;
            current=current_connection->point_index;

            while(current->b.is_link_point)
            {
                if(face_space==face_vertex_count)display_face_buffer=realloc(display_face_buffer,sizeof(GLfloat)*2*(face_space*=2));

                display_face_buffer[face_vertex_count*2+0]=current->b.position.x*wf;
                display_face_buffer[face_vertex_count*2+1]=current->b.position.y*hf;
                face_vertex_count++;

                prev=current;

                if(current_connection->traversal_direction) current=current->l.next;
                else current=current->l.prev;
            }

            current_connection=get_next_ccw_connection(current,prev);
        }
        while(current_connection!=start_connection);

        current_face->line_render_count = face_vertex_count - current_face->line_render_offset;
    }



    glf->glBindBuffer(GL_ARRAY_BUFFER,line_render_array);
    glf->glBufferData(GL_ARRAY_BUFFER,sizeof(GLfloat)*4*line_render_count,display_line_buffer,GL_STATIC_DRAW);
    glf->glBindBuffer(GL_ARRAY_BUFFER,0);

    glf->glBindBuffer(GL_ARRAY_BUFFER,point_render_array);
    glf->glBufferData(GL_ARRAY_BUFFER,sizeof(GLfloat)*2*point_render_count,display_point_buffer,GL_STATIC_DRAW);
    glf->glBindBuffer(GL_ARRAY_BUFFER,0);

    glf->glBindBuffer(GL_ARRAY_BUFFER,face_line_render_array);
    glf->glBufferData(GL_ARRAY_BUFFER,sizeof(GLfloat)*2*face_vertex_count,display_face_buffer,GL_STATIC_DRAW);
    glf->glBindBuffer(GL_ARRAY_BUFFER,0);

    free(display_line_buffer);
    free(display_point_buffer);
    free(display_face_buffer);
}


static void supersample_screen(gl_functions * glf,config_data * cd)
{
    glf->glUseProgram(shader_supersampling);

    bind_colour_texture(glf,GL_TEXTURE0);

    glf->glUniform1i(glf->glGetUniformLocation(shader_supersampling,"supersample_magnitude"),cd->supersample_magnitude);

    screen_quad(glf);

    glf->glUseProgram(0);
}

static void render_overlap(gl_functions * glf,config_data * cd,vectorizer_data * vd)
{
    float inv_w,inv_h;
    point * current_node;
    bezier * current_bezier;
    node_connection * current_connection;

    bind_colour_framebuffer(glf);

    float x=(((float)vd->w))/((float)(cd->screen_width*cd->supersample_magnitude))*cd->zoom;
    float y=(((float)vd->h))/((float)(cd->screen_height*cd->supersample_magnitude))*cd->zoom;

    float x_off=(2.0*cd->x*cd->zoom)/((float)(cd->screen_width*cd->supersample_magnitude));
    float y_off=(-2.0*cd->y*cd->zoom)/((float)(cd->screen_height*cd->supersample_magnitude));


    glf->glUseProgram(shader_image_display);

    glf->glActiveTexture(GL_TEXTURE0);

    float rw,rh;
    rw=((float)(vd->w))/((float)(vd->texture_w));
    rh=((float)(vd->h))/((float)(vd->texture_h));


    if((vd->render_mode>=VECTORIZER_RENDER_MODE_IMAGE)&&(vd->render_mode<=VECTORIZER_RENDER_MODE_CLUSTERISED))
    {
        glf->glBindTexture(GL_TEXTURE_2D,vd->images[vd->render_mode-1]);
        glf->glUniform4f(glf->glGetUniformLocation(shader_image_display,"coords"),x,-y,x_off,y_off);
        glf->glUniform2f(glf->glGetUniformLocation(shader_image_display,"relative_size"),rw,rh);
        screen_quad(glf);
    }


    x_off=(2.0*cd->x*cd->zoom)/((float)(cd->screen_width*cd->supersample_magnitude));
    y_off=(-2.0*cd->y*cd->zoom)/((float)(cd->screen_height*cd->supersample_magnitude));


    if((vd->can_render_lines)&&(vd->render_lines))
    {
        glf->glUseProgram(shader_line);
        glf->glUniform3f(glf->glGetUniformLocation(shader_line,"c"),0,1,0);

        glf->glLineWidth((float)cd->supersample_magnitude);

        glf->glUniform4f(glf->glGetUniformLocation(shader_line,"coords"),x,-y,x_off,y_off);

        glf->glBindVertexArray(line_render_vao);
        glf->glDrawArrays(GL_LINES,0,line_render_count*2);
        glf->glBindVertexArray(0);



        glf->glUseProgram(shader_point);

        glf->glPointSize(4.0*((float)cd->supersample_magnitude));

        glf->glUniform4f(glf->glGetUniformLocation(shader_point,"coords"),x,-y,x_off,y_off);

        glf->glBindVertexArray(point_render_vao);
        glf->glDrawArrays(GL_POINTS,0,line_render_count*2);
        glf->glBindVertexArray(0);
    }

    if((vd->can_render_curves)&&(vd->render_curves))
    {
        glf->glUseProgram(shader_bezier);
        glf->glBindVertexArray(empty_vao);

        glf->glUniform4f(glf->glGetUniformLocation(shader_bezier,"coords"),x,-y,x_off,y_off);

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

                        glf->glUniform2fv(glf->glGetUniformLocation(shader_bezier,"data"),4,bezier_data);

                        glf->glUniform1f(glf->glGetUniformLocation(shader_bezier,"inverse_vertex_count"),1.0/((float)(current_bezier->num_sections*4)));

                        glf->glDrawArrays(GL_LINE_STRIP,0,current_bezier->num_sections*4+1);
                    }
                }
            }
        }

        glf->glBindVertexArray(0);
    }

    glf->glUseProgram(0);
}

static void render_line_faces(gl_functions * glf,config_data * cd,vectorizer_data * vd)
{
    bind_colour_framebuffer(glf);

    shape *current_shape,*prev_shape;
    face * current_face;

    float x=(((float)vd->w))/((float)(cd->screen_width*cd->supersample_magnitude))*cd->zoom;
    float y=(((float)vd->h))/((float)(cd->screen_height*cd->supersample_magnitude))*cd->zoom;

    float x_off=(2.0*cd->x*cd->zoom)/((float)(cd->screen_width*cd->supersample_magnitude));
    float y_off=(-2.0*cd->y*cd->zoom)/((float)(cd->screen_height*cd->supersample_magnitude));


    glf->glUseProgram(shader_line_face);///render black box as starting background
    glf->glUniform4f(glf->glGetUniformLocation(shader_line_face,"coords"),x,-y,x_off,y_off);
    glf->glUniform3f(glf->glGetUniformLocation(shader_line_face,"c"),0.0,0.0,0.0);
    unit_quad(glf);

    float r,g,b;

    GLint colour_uniform_loc=glf->glGetUniformLocation(shader_line_face,"c");

    glf->glEnable(GL_BLEND);
    glf->glBlendFunc(GL_ONE, GL_ONE);

    current_shape=vd->first_shape;
    prev_shape=NULL;

    glf->glBindVertexArray(face_line_render_vao);

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

                glf->glUniform3f(colour_uniform_loc,r,g,b);

                glf->glDrawArrays(GL_TRIANGLE_FAN,current_face->line_render_offset,current_face->line_render_count);
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

    glf->glBindVertexArray(0);

    glf->glDisable(GL_BLEND);

    glf->glUseProgram(0);
}

static void render_bezier_faces(gl_functions * glf,config_data * cd,vectorizer_data * vd)
{
    bind_colour_framebuffer(glf);

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


    glf->glUseProgram(shader_line_face);///render black box as starting background
    glf->glUniform4f(glf->glGetUniformLocation(shader_line_face,"coords"),x,-y,x_off,y_off);
    glf->glUniform3f(glf->glGetUniformLocation(shader_line_face,"c"),0.0,0.0,0.0);
    unit_quad(glf);


    glf->glUseProgram(shader_bezier_face);

    glf->glUniform4f(glf->glGetUniformLocation(shader_bezier_face,"coords"),x,-y,x_off,y_off);

    GLint colour_uniform_loc=glf->glGetUniformLocation(shader_bezier_face,"c");
    GLint points_uniform_loc=glf->glGetUniformLocation(shader_bezier_face,"data");
    GLint inverse_count_uniform_loc=glf->glGetUniformLocation(shader_bezier_face,"inverse_vertex_count");

    GLfloat control_points[10];

    glf->glEnable(GL_BLEND);
    glf->glBlendFunc(GL_ONE, GL_ONE);

    float r,g,b;


    current_shape=vd->first_shape;
    prev_shape=NULL;

    glf->glBindVertexArray(empty_vao);

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

                glf->glUniform3f(colour_uniform_loc,r,g,b);

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


                        glf->glUniform2fv(points_uniform_loc,5,control_points);

                        glf->glUniform1f(inverse_count_uniform_loc,1.0/((float)current_bezier->num_sections*4));

                        glf->glDrawArrays(GL_TRIANGLE_FAN,0,current_bezier->num_sections*4+2);///+2 istead of +1 b/c 0th element reserved for fan start pos, (extra +1 is for point to segment difference, that is: the end point)

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

    glf->glBindVertexArray(0);


    glf->glDisable(GL_BLEND);


    glf->glUseProgram(0);
}

void render_screen(gl_functions * glf,config_data * cd,vectorizer_data * vd)
{
    glf->glViewport(0,0,cd->screen_width*cd->supersample_magnitude,cd->screen_height*cd->supersample_magnitude);

    glf->glClearColor(0.05,0.05,0.05,1.0);

    bind_colour_framebuffer(glf);
    glf->glClear(GL_COLOR_BUFFER_BIT);

    if(vd->lines_require_upload)upload_line_render_data(glf,vd),vd->lines_require_upload=false;

    if(vd->render_mode<=VECTORIZER_RENDER_MODE_CLUSTERISED)render_overlap(glf,cd,vd);
    if(vd->render_mode==VECTORIZER_RENDER_MODE_LINES)render_line_faces(glf,cd,vd);
    if(vd->render_mode==VECTORIZER_RENDER_MODE_CURVES)render_bezier_faces(glf,cd,vd);

    glf->glViewport(0,0,cd->screen_width,cd->screen_height);

    glf->glBindFramebuffer(GL_FRAMEBUFFER,0);
    glf->glDrawBuffer(GL_BACK);

    supersample_screen(glf,cd);

    check_for_gl_error(glf);
}
