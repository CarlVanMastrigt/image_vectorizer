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


vec4f overlay_colours[NUM_OVERLAY_COLOURS]=
{
    {0.0,0.0,0.0,0.0},///OVERLAY_NO_COLOUR

    {0.24,0.24,0.6,0.9},///OVERLAY_BACKGROUND_COLOUR
    {0.12,0.12,0.3,0.85},///OVERLAY_MAIN_COLOUR
    //{0.12,0.12,0.36,0.9},///OVERLAY_ALTERNATE_MAIN_COLOUR
    {0.0,0.0,0.0,0.3},///OVERLAY_HIGHLIGHTING_COLOUR
    {0.15,0.0,0.0,0.85},///OVERLAY_MAIN_HIGHLIGHTED_COLOUR
    {0.15,0.18,0.72,0.85},///OVERLAY_MAIN_ACTIVE_COLOUR
    {0.5,0.3,0.25,0.85},///OVERLAY_MAIN_INACTIVE_COLOUR
    {0.08,0.08,0.36,0.85},///OVERLAY_MAIN_PROMINENT_COLOUR
    {0.4,0.6,0.9,0.5},///OVERLAY_BORDER_COLOUR

    {0.4,0.6,0.9,0.25},///OVERLAY_TEXT_HIGHLIGHT_COLOUR
    {0.4,0.6,0.9,0.8},///OVERLAY_TEXT_COLOUR_0
    {0.9,0.1,0.1,1.0},///OVERLAY_TEXT_COLOUR_1
    {0.3,0.39,0.9,1.0},///OVERLAY_TEXT_COLOUR_2
    {0.3,0.8,0.48,1.0},///OVERLAY_TEXT_COLOUR_3
    {0.7,0.39,0.48,1.0},///OVERLAY_TEXT_COLOUR_4
    {0.7,0.7,0.7,1.0},///OVERLAY_TEXT_COLOUR_5
    {0.7,0.7,0.7,1.0},///OVERLAY_TEXT_COLOUR_6
    {0.7,0.7,0.7,1.0},///OVERLAY_TEXT_COLOUR_7


    {0.1,0.85,0.85,0.35},///OVERLAY_MISC_COLOUR_0
    {0.85,0.85,0.1,0.35},///OVERLAY_MISC_COLOUR_1
};

static char * image_formats[]={"jpeg","jpg","bmp","png","gif",NULL};
static char * vector_graphic_formats[]={"svg","svgz",NULL};

static file_search_file_type file_types[]=
{
    {.name="Images",.icon_name="image_file",.type_extensions=image_formats },
    {.name="Vector Graphics",.icon_name="vector_file",.type_extensions=vector_graphic_formats },
    {.name=NULL,.icon_name=NULL,.type_extensions=NULL}
};

static int import_filter[]={0,-1};
static int export_filter[]={1,-1};

static config_data initialise_config_data(void)
{
    config_data cd;

    cd.running=true;
    cd.fullscreen=false;
    cd.window_resized=true;
    cd.screen_height=768;
    cd.screen_width=1366;
    cd.supersample_magnitude=4;



    int i;
    SDL_DisplayMode dm;
    cd.max_screen_height=0;
    cd.max_screen_width=0;
    for(i=0;i<SDL_GetNumVideoDisplays();i++)
    {
        SDL_GetDesktopDisplayMode(i,&dm);
        if(dm.w>cd.max_screen_width)cd.max_screen_width=dm.w;
        if(dm.h>cd.max_screen_height)cd.max_screen_height=dm.h;
    }
    printf("largest screen dimensions: %d %d\n",cd.max_screen_width,cd.max_screen_height);

    cd.zoom=(float)cd.supersample_magnitude;
    cd.x=0.0;
    cd.y=0.0;

    return cd;
}

static void delete_config_data(config_data * cd)
{
}


typedef struct vectorizer_widget_data
{
    vectorizer_data * vd;

    vectorizer_stage relevant_stage;/// if current_vectorizer_stage <= this then set current_vectorizer_stage to 0

    void * misc_data;
}
vectorizer_widget_data;



static void vectorizer_settings_sliderbar_func(widget * w)
{
    vectorizer_widget_data * vwd=w->sliderbar.data;
    if(vwd->relevant_stage < vwd->vd->variant_stage)vwd->vd->variant_stage=vwd->relevant_stage;
    set_enterbox_text_using_int(vwd->misc_data,*w->sliderbar.value_ptr);
}

static void vectorizer_settings_checkbox_func(widget * w)
{
    vectorizer_widget_data * vwd=w->button.data;
    bool * bool_ptr=vwd->misc_data;
    if(vwd->relevant_stage < vwd->vd->variant_stage)vwd->vd->variant_stage=vwd->relevant_stage;
    *bool_ptr = !(*bool_ptr);
}

static bool vectorizer_settings_checkbox_toggle_func(widget * w)
{
    vectorizer_widget_data * vwd=w->button.data;
    bool * bool_ptr=vwd->misc_data;
    return *bool_ptr;
}

static widget * create_vectorizer_settings_adjuster_pair(char * name,int * value_ptr,int min_value,int max_value,vectorizer_stage relevant_stage,vectorizer_data * vd,int text_length)
{
    widget * box_0=create_box(WIDGET_VERTICAL,WIDGET_EVENLY_DISTRIBUTED);
    add_child_to_parent(box_0,create_static_text_bar(0,name,WIDGET_TEXT_LEFT_ALIGNED));
    widget * box_1=add_child_to_parent(box_0,create_box(WIDGET_HORIZONTAL,WIDGET_FIRST_DISTRIBUTED));

    char text[16];
    sprintf(text,"%d",*value_ptr);

    vectorizer_widget_data * vwd=malloc(sizeof(vectorizer_widget_data));

    widget * bar=add_child_to_parent(box_1,create_sliderbar(value_ptr,min_value,max_value,vectorizer_settings_sliderbar_func,vwd,true,WIDGET_HORIZONTAL,8));
    vwd->misc_data=add_child_to_parent(box_1,create_enterbox(text_length,text_length,text,adjuster_pair_enterbox_function,bar,true,false));
    vwd->relevant_stage=relevant_stage;
    vwd->vd=vd;

    return box_0;
}

static widget * create_vectorizer_settings_checkbox(char * name,bool * bool_ptr,vectorizer_stage relevant_stage,vectorizer_data * vd)
{
    vectorizer_widget_data * vwd=malloc(sizeof(vectorizer_widget_data));
    vwd->misc_data=bool_ptr;
    vwd->relevant_stage=relevant_stage;
    vwd->vd=vd;

    return create_checkbox_button_pair(name,vwd,vectorizer_settings_checkbox_func,vectorizer_settings_checkbox_toggle_func,true);
}

static void vectorizer_execution_stage_button_func(widget * w)
{
    perform_requested_vectorizer_steps(w->button.data,w->button.index);
}

static widget * create_vectorizer_execution_stage_button(char * name,vectorizer_stage stage,vectorizer_data * vd)
{
    widget * w=create_text_button(name,vd,false,vectorizer_execution_stage_button_func);
    w->button.index=stage;
    return w;
}

static file_search_error_type load_image_action_function(file_search_instance * fsi)
{
    vectorizer_data * vd=fsi->action_data;

    if(load_image(vd,fsi->directory_buffer)) return FILE_SEARCH_NO_ERROR;
    return FILE_SEARCH_ERROR_CAN_NOT_LOAD_FILE;
}

static file_search_error_type save_image_action_function(file_search_instance * fsi)
{
    vectorizer_data * vd=fsi->action_data;

    perform_requested_vectorizer_steps(vd,VECTORIZER_STAGE_CONVERT_TO_BEZIER);

    if(vd->current_stage==VECTORIZER_STAGE_START) return FILE_SEARCH_ERROR_CAN_NOT_SAVE_FILE;
    #warning need custom error no image loaded

    if(vd->current_stage!=VECTORIZER_STAGE_CONVERT_TO_BEZIER)return FILE_SEARCH_ERROR_CAN_NOT_SAVE_FILE;
    #warning need custom error, could not execute processes


    if(export_vector_image_as_svg(vd,fsi->directory_buffer)) return FILE_SEARCH_NO_ERROR;
    return FILE_SEARCH_ERROR_CAN_NOT_SAVE_FILE;
}


static void render_mode_button_function(widget * w)
{
    vectorizer_data * vd=w->button.data;

    if(w->button.index<=vd->max_render_mode)vd->render_mode=w->button.index;
    else vd->render_mode=VECTORIZER_RENDER_MODE_BLANK;
}

static bool render_mode_button_toggle_status_function(widget * w)
{
    vectorizer_data * vd=w->button.data;

    return vd->render_mode==w->button.index;
}

static widget * create_render_mode_button(char * name,int mode,vectorizer_data * vd)
{
    //widget * w=create_text_button(name,vd,false,render_mode_button_function);
    widget * w=create_text_highlight_toggle_button(name,vd,false,render_mode_button_function,render_mode_button_toggle_status_function);
    w->button.index=mode;

    vd->render_mode_buttons[mode]=w;

    w->base.status&= ~WIDGET_ACTIVE;

    return w;
}

static void create_vectorizer_menus(widget * main_menu,shared_file_search_data * sfsd,vectorizer_data * vd)
{
    #warning gray out save file if vectorization incomplete (no bezier curve yet)

    widget *box_0,*box_1,*box_2,*box_3,*box_4;///,*box_5,*box_6,*button_0,*sliderbar_0

    box_0=add_child_to_parent(main_menu,create_box(WIDGET_HORIZONTAL,WIDGET_LAST_DISTRIBUTED));///change distribution to alter end

    box_1=add_child_to_parent(box_0,create_box(WIDGET_VERTICAL,WIDGET_LAST_DISTRIBUTED));///change distribution to alter end

    box_2=add_child_to_parent(add_child_to_parent(box_1,create_panel()),create_box(WIDGET_VERTICAL,WIDGET_EVENLY_DISTRIBUTED));
    add_child_to_parent(box_1,create_empty_widget(0,0));


    widget * load_window=add_child_to_parent(main_menu,create_load_window(sfsd,"Load File",import_filter,load_image_action_function,vd,false,main_menu));
    widget * save_window=add_child_to_parent(main_menu,create_save_window(sfsd,"Save File",export_filter,save_image_action_function,vd,false,main_menu,vector_graphic_formats));


    box_3=add_child_to_parent(box_2,create_box(WIDGET_HORIZONTAL,WIDGET_FIRST_DISTRIBUTED));
    add_child_to_parent(box_3,create_text_button("Load File",load_window,false,window_toggle_button_func));
    box_4=add_child_to_parent(box_2,create_box(WIDGET_HORIZONTAL,WIDGET_LAST_DISTRIBUTED));
    add_child_to_parent(box_3,create_icon_collapse_button("collapse","expand",box_4,true));
    add_child_to_parent(box_4,create_unit_separator_widget());
    box_4=add_child_to_parent(box_4,create_box(WIDGET_VERTICAL,WIDGET_EVENLY_DISTRIBUTED));///re-use box_4
    add_child_to_parent(box_4,create_separator_widget());///================================
    add_child_to_parent(box_4,create_vectorizer_settings_adjuster_pair("Background red",&vd->back_r,0,255,VECTORIZER_STAGE_LOAD_IMAGE,vd,3));
    add_child_to_parent(box_4,create_separator_widget());
    add_child_to_parent(box_4,create_vectorizer_settings_adjuster_pair("Background green",&vd->back_g,0,255,VECTORIZER_STAGE_LOAD_IMAGE,vd,3));
    add_child_to_parent(box_4,create_separator_widget());
    add_child_to_parent(box_4,create_vectorizer_settings_adjuster_pair("Background blue",&vd->back_b,0,255,VECTORIZER_STAGE_LOAD_IMAGE,vd,3));
    add_child_to_parent(box_4,create_separator_widget());


    box_3=add_child_to_parent(box_2,create_box(WIDGET_HORIZONTAL,WIDGET_FIRST_DISTRIBUTED));
    add_child_to_parent(box_3,create_vectorizer_execution_stage_button("Image preprocessing",VECTORIZER_STAGE_IMAGE_PROCESSING,vd));///perform op to here (check progression stage and changes appropriately to avoid unnecessary ops
    box_4=add_child_to_parent(box_2,create_box(WIDGET_HORIZONTAL,WIDGET_LAST_DISTRIBUTED));
    add_child_to_parent(box_3,create_icon_collapse_button("collapse","expand",box_4,true));
    add_child_to_parent(box_4,create_unit_separator_widget());
    box_4=add_child_to_parent(box_4,create_box(WIDGET_VERTICAL,WIDGET_EVENLY_DISTRIBUTED));///re-use box_4
    add_child_to_parent(box_4,create_separator_widget());///================================
    add_child_to_parent(box_4,create_vectorizer_settings_adjuster_pair("Blur factor",&vd->blur_factor,0,16,VECTORIZER_STAGE_IMAGE_PROCESSING,vd,2));
    add_child_to_parent(box_4,create_separator_widget());
    add_child_to_parent(box_4,create_vectorizer_settings_adjuster_pair("Combination threshold",&vd->cluster_combination_threshold,2,80,VECTORIZER_STAGE_IMAGE_PROCESSING,vd,2));
    add_child_to_parent(box_4,create_separator_widget());
    add_child_to_parent(box_4,create_vectorizer_settings_adjuster_pair("Min size threshold",&vd->valid_cluster_size_factor,8,32,VECTORIZER_STAGE_IMAGE_PROCESSING,vd,2));
    add_child_to_parent(box_4,create_separator_widget());
    add_child_to_parent(box_4,create_vectorizer_settings_checkbox("Correct clusters",&vd->correct_clusters,VECTORIZER_STAGE_IMAGE_PROCESSING,vd));
    add_child_to_parent(box_4,create_separator_widget());


    box_3=add_child_to_parent(box_2,create_box(WIDGET_HORIZONTAL,WIDGET_FIRST_DISTRIBUTED));
    add_child_to_parent(box_3,create_vectorizer_execution_stage_button("Generate geometry",VECTORIZER_STAGE_GENERATE_GEOMETRY,vd));///perform op to here (check progression stage and changes appropriately to avoid unnecessary ops
    box_4=add_child_to_parent(box_2,create_box(WIDGET_HORIZONTAL,WIDGET_LAST_DISTRIBUTED));
    add_child_to_parent(box_3,create_icon_collapse_button("collapse","expand",box_4,true));
    add_child_to_parent(box_4,create_unit_separator_widget());
    box_4=add_child_to_parent(box_4,create_box(WIDGET_VERTICAL,WIDGET_EVENLY_DISTRIBUTED));///re-use box_4
    add_child_to_parent(box_4,create_separator_widget());///================================
    add_child_to_parent(box_4,create_vectorizer_settings_adjuster_pair("Correction passes",&vd->initial_geometry_correction_passes,2,24,VECTORIZER_STAGE_GENERATE_GEOMETRY,vd,2));
    add_child_to_parent(box_4,create_separator_widget());


    box_3=add_child_to_parent(box_2,create_box(WIDGET_HORIZONTAL,WIDGET_FIRST_DISTRIBUTED));
    add_child_to_parent(box_3,create_vectorizer_execution_stage_button("Convert to beziers",VECTORIZER_STAGE_CONVERT_TO_BEZIER,vd));
    box_4=add_child_to_parent(box_2,create_box(WIDGET_HORIZONTAL,WIDGET_LAST_DISTRIBUTED));
    add_child_to_parent(box_3,create_icon_collapse_button("collapse","expand",box_4,true));
    add_child_to_parent(box_4,create_unit_separator_widget());
    box_4=add_child_to_parent(box_4,create_box(WIDGET_VERTICAL,WIDGET_EVENLY_DISTRIBUTED));///re-use box_4
    add_child_to_parent(box_4,create_separator_widget());///================================
    add_child_to_parent(box_4,create_vectorizer_settings_checkbox("Minimise variation",&vd->adjust_beziers,VECTORIZER_STAGE_CONVERT_TO_BEZIER,vd));
    add_child_to_parent(box_4,create_separator_widget());



    box_3=add_child_to_parent(box_2,create_box(WIDGET_HORIZONTAL,WIDGET_FIRST_DISTRIBUTED));
    //
    add_child_to_parent(box_3,create_text_button("Save File",save_window,false,window_toggle_button_func));
    box_4=add_child_to_parent(box_2,create_box(WIDGET_HORIZONTAL,WIDGET_LAST_DISTRIBUTED));
    add_child_to_parent(box_3,create_icon_collapse_button("collapse","expand",box_4,true));
    add_child_to_parent(box_4,create_unit_separator_widget());
    box_4=add_child_to_parent(box_4,create_box(WIDGET_VERTICAL,WIDGET_EVENLY_DISTRIBUTED));///re-use box_4
    add_child_to_parent(box_4,create_separator_widget());///================================
    add_child_to_parent(box_4,create_vectorizer_settings_checkbox("Fix boundaries",&vd->fix_exported_boundaries,VECTORIZER_STAGE_END,vd));


    toggle_widget(save_window);
    toggle_widget(load_window);



    box_1=add_child_to_parent(box_0,create_box(WIDGET_HORIZONTAL,WIDGET_FIRST_DISTRIBUTED));
    add_child_to_parent(box_1,create_empty_widget(0,0));

    box_2=add_child_to_parent(box_1,create_box(WIDGET_VERTICAL,WIDGET_LAST_DISTRIBUTED));

    box_3=add_child_to_parent(add_child_to_parent(box_2,create_panel()),create_box(WIDGET_VERTICAL,WIDGET_NORMALLY_DISTRIBUTED));
    add_child_to_parent(box_2,create_empty_widget(0,0));

    add_child_to_parent(box_3,create_render_mode_button("Blank",VECTORIZER_RENDER_MODE_BLANK,vd));
    add_child_to_parent(box_3,create_render_mode_button("Image",VECTORIZER_RENDER_MODE_IMAGE,vd));
    add_child_to_parent(box_3,create_render_mode_button("Blurred",VECTORIZER_RENDER_MODE_BLURRED,vd));
    add_child_to_parent(box_3,create_render_mode_button("Clusterised",VECTORIZER_RENDER_MODE_CLUSTERISED,vd));
    add_child_to_parent(box_3,create_render_mode_button("Line shapes",VECTORIZER_RENDER_MODE_LINES,vd));
    add_child_to_parent(box_3,create_render_mode_button("Curve shapes",VECTORIZER_RENDER_MODE_CURVES,vd));

    vd->render_lines_checkbox_widget=box_4=add_child_to_parent(box_3,create_box(WIDGET_VERTICAL,WIDGET_NORMALLY_DISTRIBUTED));
    add_child_to_parent(box_4,create_separator_widget());
    add_child_to_parent(box_4,create_bool_checkbox_button_pair("Lines",&vd->render_lines));
    vd->render_curves_checkbox_widget=add_child_to_parent(box_3,create_bool_checkbox_button_pair("Curves",&vd->render_curves));

    vd->render_mode_buttons[0]->base.status|=WIDGET_ACTIVE;
    vd->render_lines_checkbox_widget->base.status&= ~WIDGET_ACTIVE;
    vd->render_curves_checkbox_widget->base.status&= ~WIDGET_ACTIVE;
}


int vectorizer_interface(void)
{
    config_data cd;
    SDL_Event event;
    overlay_data o_data;
    overlay_theme cubic_theme;
    vectorizer_data * vd;
    gl_functions * glf;

    widget * menu_widget;
    shared_file_search_data * sfsd;


    set_shader_version_and_defines("#version 130\n\n");

    SDL_Init(SDL_INIT_EVERYTHING);

    cd=initialise_config_data();


    SDL_GL_LoadLibrary(NULL);

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION,3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION,0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,SDL_GL_CONTEXT_PROFILE_CORE);

    uint32_t window_flags=SDL_WINDOW_OPENGL;
    if(cd.fullscreen)window_flags|=SDL_WINDOW_FULLSCREEN;
    else window_flags|=SDL_WINDOW_RESIZABLE;
    SDL_Window * window = SDL_CreateWindow("Image Vectorizer",SDL_WINDOWPOS_UNDEFINED,SDL_WINDOWPOS_UNDEFINED,cd.screen_width,cd.screen_height,window_flags);



    SDL_GLContext gl_context = SDL_GL_CreateContext(window);

    glf=get_gl_functions();

    load_contextual_resources();


    printf("video driver version: %s\n\n",glf->glGetString(GL_VERSION));


    TTF_Init();
    IMG_Init(0);


    vd=create_vectorizer_data(glf);

    cubic_theme=create_overlay_theme(512,512);///have 2/3 base fonts as input to this function
    #warning make above create_overlay_theme

    #warning make part of other function (initialise_cubic_theme) and improve functionality



    initialise_overlay_data(&o_data);

    initialise_cubic_theme(&cubic_theme);

    initialise_overlay(glf);

    set_current_overlay_theme(&cubic_theme);


    menu_widget=create_widget_menu();

    sfsd=create_shared_file_search_data("/home/carl/aa",file_types);

    create_vectorizer_menus(menu_widget,sfsd,vd);

    SDL_SetWindowMinimumSize(window,1280,720);
    SDL_SetWindowMaximumSize(window,cd.max_screen_width,cd.max_screen_height);



    initialise_misc_render_gl_variables(glf);

    initialise_framebuffer_set(&cd);


    int mx,my;

    while(cd.running)
    {
        if(SDL_WaitEvent(&event))
        {
            handle_input_event(&event,&cd,menu_widget);
        }

        while(SDL_PollEvent(&event))
        {
            handle_input_event(&event,&cd,menu_widget);
        }

        if(cd.window_resized)
        {
            organise_menu_widget(menu_widget,cd.screen_width,cd.screen_height);
            cd.window_resized=false;
        }

        update_shared_file_search_data_directory_use(sfsd);

        SDL_GetMouseState(&mx,&my);
        find_potential_interaction_widget(menu_widget,mx,my);

        update_theme_textures_on_videocard(&cubic_theme);
        #warning get access to current theme in better way (possibly through static or pointer to active)

        render_screen(glf,&cd,vd);

        render_widget_overlay(&o_data,menu_widget);///prerender step
        render_overlay(glf,&o_data,&cubic_theme,cd.screen_width,cd.screen_height,overlay_colours);///actual render step

        SDL_GL_SwapWindow(window);
    }

    delete_overlay_theme(&cubic_theme);
    delete_widget(menu_widget);
    delete_overlay_data(&o_data);
    delete_shared_file_search_data(sfsd);

    delete_vectorizer_data(vd);

    delete_config_data(&cd);

    free_gl_functions(glf);

    SDL_GL_DeleteContext(gl_context);

    SDL_DestroyWindow(window);

    SDL_GL_UnloadLibrary();

    IMG_Quit();
    SDL_Quit();

    return 0;
}

int main()
{
    return vectorizer_interface();
}
