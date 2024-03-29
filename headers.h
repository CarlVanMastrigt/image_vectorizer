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

#ifndef HEADERS_H
#define HEADERS_H

#include "cvm_shared.h"

typedef struct config_data
{
    bool running;

    bool fullscreen;
    bool window_resized;

    int screen_width;
    int screen_height;

    int max_screen_width;
    int max_screen_height;

    int supersample_magnitude;

    float zoom;
    float x;
    float y;
}
config_data;



//#define PI 3.14159265358979
//#define E_ 2.71828
//#define SQRT_3 1.7320508075688
//#define SQRT_2 1.414213562373
//#define SQRT_HALF 0.70710678118654



//#include "oglfp.h"
//#include "game_math.h"
//#include "game_random.h"
//#include "game_list.h"
//#include "managed_buffer.h"
//
//
//#include "camera.h"
//
//
//#include "shader.h"
//#include "overlay.h"
//#include "widget.h"
//#include "view.h"
//#include "mesh.h"


#include "input.h"

#include "framebuffer.h"

#include "vectorizer.h"


#include "render.h"




#include "themes/cubic.h"







void test();





#endif
