#include "headers.h"


static void handle_window_event(SDL_Event * event,config_data * cd)
{
    switch(event->window.event)
    {
        case SDL_WINDOWEVENT_RESIZED:
        cd->screen_width=event->window.data1;
        cd->screen_height=event->window.data2;
        cd->window_resized=true;
        break;

        default:;
    }
}

static void handle_key_down_event(SDL_Event * event,widget * menu_widget,config_data * cd)
{
    if(handle_widget_overlay_keyboard(menu_widget,event->key.keysym.sym))return;

    switch(event->key.keysym.scancode)
    {
        case SDL_SCANCODE_ESCAPE:
        ///if(!collapse_widget_lists(head_widget));///if collapsible list is present "eat" escape press
        cd->running=false;
        break;

        case SDL_SCANCODE_C:
        cd->x=0.0;
        cd->y=0.0;
        cd->zoom=((float)cd->supersample_magnitude);
        break;


        default:;
    }
}

static void handle_mouse_button_down(SDL_Event * event,widget * menu_widget)
{
    if(event->button.button==SDL_BUTTON_LEFT)
    {
        if(handle_widget_overlay_left_click(menu_widget,event->button.x,event->button.y))return;

        printf("click not on menu\n");
    }
}

static void handle_mouse_button_up(SDL_Event * event,widget * menu_widget)
{
    if(event->button.button==SDL_BUTTON_LEFT)
    {
        if(handle_widget_overlay_left_release(menu_widget,event->button.x,event->button.y))return;
    }
}

static void handle_mouse_motion(SDL_Event * event,widget * menu_widget,config_data * cd)
{
    if(handle_widget_overlay_movement(menu_widget,event->button.x,event->button.y))return;

    if(event->motion.state&SDL_BUTTON(SDL_BUTTON_MIDDLE))
    {
        cd->x+=((float)event->motion.xrel*cd->supersample_magnitude)/cd->zoom;
        cd->y+=((float)event->motion.yrel*cd->supersample_magnitude)/cd->zoom;
    }
}

static void handle_mouse_wheel(SDL_Event * event,widget * menu_widget,config_data * cd)
{
    int x,y;
    SDL_GetMouseState(&x,&y);
    int delta_wheel=event->wheel.y;

    if(handle_widget_overlay_wheel(menu_widget,x,y,delta_wheel))return;

    if(delta_wheel<0) cd->zoom*=1.25;
    if(delta_wheel>0) cd->zoom*=0.8;
}

void handle_input_event(SDL_Event * event,config_data * cd,widget * menu_widget)
{
    switch(event->type)
    {
        case SDL_QUIT:
        cd->running=false;
        break;

        case SDL_WINDOWEVENT:
        handle_window_event(event,cd);
        break;

        case SDL_KEYDOWN:
        handle_key_down_event(event,menu_widget,cd);
        break;

        case SDL_MOUSEMOTION:
        handle_mouse_motion(event,menu_widget,cd);
        break;

        case SDL_MOUSEBUTTONDOWN:
        handle_mouse_button_down(event,menu_widget);
        break;

        case SDL_MOUSEBUTTONUP:
        handle_mouse_button_up(event,menu_widget);
        break;

        case SDL_MOUSEWHEEL:
        handle_mouse_wheel(event,menu_widget,cd);
        break;

        default:;
    }
}




