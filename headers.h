
#ifndef _HEADERS_H_
#define _HEADERS_H_



#include <stdio.h>
#include <stdatomic.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <stdbool.h>
#include <sys/time.h>
#include <pthread.h>
#include <limits.h>
#include <float.h>


#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>

#include <dirent.h>
#include <sys/stat.h>
/// ` pkg-config --cflags --libs sdl2 `


#include <GL/gl.h>
#include <GL/glext.h>
//#include <random.h>



#include "cvm_shared.h"

//typedef enum { false=0, true=1 } bool;

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
