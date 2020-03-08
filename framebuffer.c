#include "headers.h"

static GLuint colour_texture;
static GLuint colour_framebuffer;

GLuint get_colour_texture()
{
    return colour_texture;
}

void bind_colour_framebuffer()
{
    glBindFramebuffer_ptr(GL_FRAMEBUFFER,colour_framebuffer);
}

void initialise_framebuffer_set(config_data * cd)
{
    int w,h;
    w=cd->max_screen_width*cd->supersample_magnitude;
    h=cd->max_screen_height*cd->supersample_magnitude;

    glGenTextures(1,&colour_texture);
    glBindTexture(GL_TEXTURE_2D,colour_texture);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGB16F,w,h,0,GL_RGB,GL_FLOAT,NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    glBindTexture(GL_TEXTURE_2D,0);

    GLenum framebuffer_draw_enums[8]={  GL_COLOR_ATTACHMENT0,GL_COLOR_ATTACHMENT1,GL_COLOR_ATTACHMENT2,GL_COLOR_ATTACHMENT3};

    glGenFramebuffers_ptr(1,&colour_framebuffer);
    glBindFramebuffer_ptr(GL_FRAMEBUFFER,colour_framebuffer);
    glFramebufferTexture2D_ptr(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,colour_texture,0);
    glDrawBuffers_ptr(1,framebuffer_draw_enums);
    glBindFramebuffer_ptr(GL_FRAMEBUFFER,0);
}

