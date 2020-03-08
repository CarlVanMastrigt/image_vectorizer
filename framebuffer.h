#ifndef _HEADERS_H_
#include "headers.h"
#endif

#ifndef _FRAMEBUFFER_H
#define _FRAMEBUFFER_H

void initialise_framebuffer_set(config_data * cd);

GLuint get_colour_texture(void);

void bind_colour_framebuffer(void);

#endif
