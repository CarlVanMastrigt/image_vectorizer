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

static GLuint colour_texture;
static GLuint colour_framebuffer;

void bind_colour_texture(gl_functions * glf,GLenum binding_point)
{
    glf->glActiveTexture(binding_point);
    glf->glBindTexture(GL_TEXTURE_2D,colour_texture);
}

void bind_colour_framebuffer(gl_functions * glf)
{
    glf->glBindFramebuffer(GL_FRAMEBUFFER,colour_framebuffer);
}

void initialise_framebuffer_set(gl_functions * glf,config_data * cd)
{
    int w,h;
    w=cd->max_screen_width*cd->supersample_magnitude;
    h=cd->max_screen_height*cd->supersample_magnitude;

    glf->glGenTextures(1,&colour_texture);
    glf->glBindTexture(GL_TEXTURE_2D,colour_texture);
    glf->glTexImage2D(GL_TEXTURE_2D,0,GL_RGB16F,w,h,0,GL_RGB,GL_FLOAT,NULL);
    glf->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glf->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    glf->glBindTexture(GL_TEXTURE_2D,0);

    GLenum framebuffer_draw_enums[8]={  GL_COLOR_ATTACHMENT0,GL_COLOR_ATTACHMENT1,GL_COLOR_ATTACHMENT2,GL_COLOR_ATTACHMENT3};

    glf->glGenFramebuffers(1,&colour_framebuffer);
    glf->glBindFramebuffer(GL_FRAMEBUFFER,colour_framebuffer);
    glf->glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,colour_texture,0);
    glf->glDrawBuffers(1,framebuffer_draw_enums);
    glf->glBindFramebuffer(GL_FRAMEBUFFER,0);
}

