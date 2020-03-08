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

static inline float correct_srgb_gamma(float u)
{
    if(u<0.04045)return u*0.077399380805;
    else return pow((u+0.055)*0.947867298578,2.4);
}

static inline float cie_function(float t)
{
    if(t>0.00885645168)return cbrtf(t);
    else return (t*7.787037037037)+0.137931034482;
}

static inline float inverse_correct_srgb_gamma(float u)
{
    if(u<0.0031308)return u*12.92;
    else return 1.055*powf(u,0.41666666666667)-0.055;
}

static inline float inverse_cie_function(float t)
{
    if(t>0.20689655172)return t*t*t;
    else return 0.12841854934602*(t-0.13793103448276);
}

pixel_lab convert_rgb_to_cielab(uint8_t R,uint8_t G,uint8_t B)
{
    pixel_lab lab;
    float x,y,z,r,g,b;



    r=correct_srgb_gamma(((float)R)/255.0);
    g=correct_srgb_gamma(((float)G)/255.0);
    b=correct_srgb_gamma(((float)B)/255.0);

    x=0.4124564*r+0.3575761*g+0.1804375*b;
    y=0.2126729*r+0.7151522*g+0.0721750*b;
    z=0.0193339*r+0.1191920*g+0.9503041*b;

    y=cie_function(y);

///d50
//            pc->cie_l=116.0*cie_y-16.0;
//            pc->cie_a=500.0*(cie_function(x*1.0371163188)-cie_y);
//            pc->cie_b=200.0*(cie_y-cie_function(z*1.2118450583));

///d65
    lab.l=116.0*y-16.0;
    lab.a=500.0*(cie_function(x*1.052090029448)-y);
    lab.b=200.0*(y-cie_function(z*0.91840858161));

    lab.c=sqrt(lab.a*lab.a + lab.b*lab.b);

    return lab;
}

void convert_cielab_to_rgb(pixel_lab lab,uint8_t * R,uint8_t * G,uint8_t * B)
{
    float x,y,z,cie_y;
    int32_t i;

    cie_y=(lab.l+16.0)*0.00862068965517;

        ///d65
    x=0.950489*inverse_cie_function(cie_y+0.002*lab.a);
    y=inverse_cie_function(cie_y);
    z=1.088840*inverse_cie_function(cie_y-0.005*lab.b);

    i=(int32_t)(0.5+255.0*inverse_correct_srgb_gamma( 3.2404548*x-1.5371389*y-0.4985315*z));
    *R=(i<0)?0:((i>255)?255:i);
    i=(int32_t)(0.5+255.0*inverse_correct_srgb_gamma(-0.9692664*x+1.8760109*y+0.0415561*z));
    *G=(i<0)?0:((i>255)?255:i);
    i=(int32_t)(0.5+255.0*inverse_correct_srgb_gamma( 0.0556434*x-0.2040259*y+1.0572252*z));
    *B=(i<0)?0:((i>255)?255:i);
}

void convert_cielab_to_rgb_float(pixel_lab lab,float * R,float * G,float * B)
{
    float x,y,z,cie_y;

    cie_y=(lab.l+16.0)*0.00862068965517;

        ///d65
    x=0.950489*inverse_cie_function(cie_y+0.002*lab.a);
    y=inverse_cie_function(cie_y);
    z=1.088840*inverse_cie_function(cie_y-0.005*lab.b);

    *R=inverse_correct_srgb_gamma( 3.2404548*x-1.5371389*y-0.4985315*z);
    *G=inverse_correct_srgb_gamma(-0.9692664*x+1.8760109*y+0.0415561*z);
    *B=inverse_correct_srgb_gamma( 0.0556434*x-0.2040259*y+1.0572252*z);
}

uint32_t rgb_hash_from_lab(pixel_lab lab)
{
    float x,y,z,cie_y;
    uint32_t sum;
    int32_t i;

    cie_y=(lab.l+16.0)*0.00862068965517;

        ///d65
    x=0.950489*inverse_cie_function(cie_y+0.002*lab.a);
    y=inverse_cie_function(cie_y);
    z=1.088840*inverse_cie_function(cie_y-0.005*lab.b);

    i=(int32_t)(0.5+255.0*inverse_correct_srgb_gamma( 3.2404548*x-1.5371389*y-0.4985315*z));
    sum=((i<0)?0:((i>255)?255:i))<<16;
    i=(int32_t)(0.5+255.0*inverse_correct_srgb_gamma(-0.9692664*x+1.8760109*y+0.0415561*z));
    sum|=((i<0)?0:((i>255)?255:i))<<8;
    i=(int32_t)(0.5+255.0*inverse_correct_srgb_gamma( 0.0556434*x-0.2040259*y+1.0572252*z));
    sum|=(i<0)?0:((i>255)?255:i);

    return sum;
}

float cie00_difference(pixel_lab p1,pixel_lab p2)
{
    //return sqrt((p2.l-p1.l)*(p2.l-p1.l)  +  (p2.a-p1.a)*(p2.a-p1.a)  +  (p2.b-p1.b)*(p2.b-p1.b));
    float delta_L_,L,a1_,a2_,G,C_,C1_,C2_,delta_C_,h1_,h2_,delta_h_,delta_H_,H_,SL,SC,SH,RT;

    G=0.5*(p1.c+p2.c);
    G=G*G*G*G*G*G*G;
    G=1.5-0.5*sqrtf(G/(G+6103515625.0));

    a1_=p1.a*G;
    a2_=p2.a*G;

    C1_=sqrtf(a1_*a1_ + p1.b*p1.b);
    C2_=sqrtf(a2_*a2_ + p2.b*p2.b);

    C_=0.5*(C1_ + C2_);
    delta_C_=C2_-C1_;


    if(C1_<0.0000001)h1_=0.0;
    else h1_=atan2f(p1.b,a1_);
    if(h1_<0.0)h1_+=2.0*PI;///must be in range 0 to PI


    if(C2_<0.0000001)h2_=0.0;
    else h2_=atan2f(p2.b,a2_);
    if(h2_<0.0)h2_+=2.0*PI;///must be in range 0 to PI


    ///the fuck is going on here; sign of delta_h_ flips if difference between h1_ and h2_ is greater than 180
    ///because sin(delta_h_) is used directly in result this means order of pc1 and pc2 becomes important to result??? (because sin of +/- results in +/- affecting sign of contribution)

    if((C1_<0.0000001)||(C2_<0.0000001))delta_h_=0.0;///either angle is indeterminate than difference is irrelevant
    else if(fabs(h2_-h1_)<PI)delta_h_=h2_-h1_;
    else if(h2_<h1_)delta_h_=h2_-h1_+2.0*PI;
    else delta_h_=h2_-h1_-2.0*PI;

    delta_H_=2.0*sqrtf(C1_*C2_)*sinf(0.5*delta_h_);

    ///in between angle
    if((C1_<0.0000001)||(C2_<0.0000001))H_=h1_+h2_;
    else if(fabsf(h2_-h1_)>PI)H_=0.5*(h1_+h2_+2.0*PI);
    else H_=0.5*(h1_+h2_);

    ///above can be optimised/combined by setting then altering values simultaneously w/ shared if statements

    SH=1.0+0.015*C_*(1.0-0.17*cosf(H_-0.523598775)+0.24*cosf(2.0*H_)+0.32*cosf(3.0*H_+0.104719755)-0.2*cosf(4.0*H_-1.099557429));
    SC=1.0+0.045*C_;

    L=(0.5*(p1.l+p2.l))-50.0;
    SL=1.0+0.015*L*L/sqrtf(20.0+L*L);


    C_=C_*C_*C_*C_*C_*C_*C_;///C_^7
    H_=2.291831181*(H_-4.799655443);/// H_-275/25 (degrees)
    RT= -2.0*sqrtf(C_/(C_+6103515625.0))*sinf(1.047197551*expf(-H_*H_));

    delta_L_=p2.l-p1.l;

    return sqrtf( delta_L_*delta_L_/(SL*SL) + delta_C_*delta_C_/(SC*SC) + delta_H_*delta_H_/(SH*SH) + RT*delta_C_*delta_H_/(SC*SH)  );
}

vec3f convert_pixel_lab_to_vec3f(pixel_lab p)
{
    return (vec3f){.x=p.l,.y=p.a,.z=p.b};
}






typedef struct filter_data
{
    float value;
    int x_off;
    int y_off;
}
filter_data;

static uint32_t set_gaussian_filter_data(filter_data ** filter_datum,int blur_factor)
{
    int i,j,r,d;
    float filter_sum;
    filter_data fd;
    uint32_t datum_count;

    float gaussian_distance_factor=sqrt((float)blur_factor)*0.25;///div sqrt(16)*0.25 -> max of 1.0
    int gaussian_r=4;

    r=2*gaussian_r+1;

    *filter_datum=malloc(sizeof(filter_data)*r*r);

    filter_sum=0.0;
    datum_count=0;

    gaussian_distance_factor=1.0/(gaussian_distance_factor*gaussian_distance_factor);

    for(i=-gaussian_r;i<=gaussian_r;i++) for(j=-gaussian_r;j<=gaussian_r;j++)
    {
        d=i*i+j*j;

        fd.x_off=i;
        fd.y_off=j;
        fd.value=expf(-((float)(d))*gaussian_distance_factor);/// 20 mil allows base sum of 100

        if(fd.value>0.00001)
        {
            filter_sum+=fd.value;

            (*filter_datum)[datum_count++]=fd;
        }
    }

    filter_sum=1.0/filter_sum;

    for(i=0;i<datum_count;i++) (*filter_datum)[i].value*=filter_sum;

    return datum_count;
}

bool load_image(vectorizer_data * vd,char * filename)
{
    SDL_Surface * surface=IMG_Load(filename);

    int i,j;
    uint8_t red,green,blue,alpha;
    int32_t r,g,b;

    if(surface==NULL)
    {
        return false;
    }

    reset_vectorizer_display_variables(vd);
    organise_toplevel_widget(vd->render_mode_buttons[0]);

    vd->current_stage=VECTORIZER_STAGE_LOAD_IMAGE;
    vd->variant_stage=VECTORIZER_STAGE_IMAGE_PROCESSING;

    #warning may need to remove all faces/colours &c at the same time ?
    remove_all_points_from_image(vd);///must be done before resetting/reallocating blocks as remove point assumes block to still be valid


    if(surface->format->format!=SDL_PIXELFORMAT_RGBA8888)
    {
        SDL_PixelFormat *fmt;
        fmt = SDL_AllocFormat(SDL_PIXELFORMAT_RGBA8888);
        surface= SDL_ConvertSurface(surface, fmt, 0);
    }

    int w=surface->w;
    int h=surface->h;

    if((w*h)>(vd->w*vd->h))
    {
        free(vd->origional);
        vd->origional=malloc(sizeof(pixel_lab)*w*h);

        free(vd->blurred);
        vd->blurred=malloc(sizeof(pixel_lab)*w*h);

        free(vd->clusterised);
        vd->clusterised=malloc(sizeof(pixel_lab)*w*h);

        free(vd->pixel_faces);
        vd->pixel_faces=malloc(sizeof(face*)*w*h);

        free(vd->diagonals);
        vd->diagonals=malloc(sizeof(diagonal_sample_connection_type)*w*h);

        free(vd->blocks);
        vd->blocks=malloc(sizeof(point*)*w*h);

        free(vd->h_grad);
        vd->h_grad=malloc(sizeof(pixel_lab)*(w-1)*h);

        free(vd->v_grad);
        vd->v_grad=malloc(sizeof(pixel_lab)*w*(h-1));
    }

    vd->w=w;
    vd->h=h;

    vd->texture_w=(w+3)&0xFFFFFFFC;///round up to power of 4
    vd->texture_h=(h+3)&0xFFFFFFFC;///round up to power of 4

    for(i=0;i<(w*h);i++) vd->blocks[i]=NULL;

    for(i=0;i<h;i++) for(j=0;j<w;j++)
    {
        SDL_GetRGBA(*((uint32_t*)(((uint8_t*) surface->pixels)+i*surface->pitch+j*surface->format->BytesPerPixel)),surface->format,&red,&green,&blue,&alpha);
        r=(vd->back_r*(255-alpha)+red*alpha)/255;
        g=(vd->back_g*(255-alpha)+green*alpha)/255;
        b=(vd->back_b*(255-alpha)+blue*alpha)/255;

        vd->origional[i*w+j]=convert_rgb_to_cielab(r,g,b);
    }

    upload_modified_image_colour(vd,vd->origional,VECTORIZER_RENDER_MODE_IMAGE);

    SDL_FreeSurface(surface);

    vd->w=w;
    vd->h=h;

    return true;
}

static pixel_lab gaussian_filter_pixel_colour(filter_data * fd,uint32_t filter_data_count,pixel_lab * data,int x,int y,int w,int h)
{
    pixel_lab p;
    p.l=p.a=p.b=0;

    uint32_t i;
    int xc,yc;

    for(i=0;i<filter_data_count;i++)
    {
        xc=x+fd[i].x_off;
        xc-=xc*(xc<0) + (xc-w+1)*(xc>=w);

        yc=y+fd[i].y_off;
        yc-=yc*(yc<0) + (yc-h+1)*(yc>=h);

        p.l+=data[yc*w+xc].l*fd[i].value;
        p.a+=data[yc*w+xc].a*fd[i].value;
        p.b+=data[yc*w+xc].b*fd[i].value;
    }

    p.c=sqrt(p.a*p.a + p.b*p.b);

    return p;
}

void upload_modified_image_colour(vectorizer_data * vd,pixel_lab * data,int image_id)
{
    int32_t x,y;

    uint8_t * pixels=malloc(sizeof(uint8_t)*vd->texture_w*vd->texture_h*3);

    for(x=0;x<vd->w;x++)for(y=0;y<vd->h;y++) convert_cielab_to_rgb(data[vd->w*y+x],pixels+(y*vd->texture_w+x)*3+0,pixels+(y*vd->texture_w+x)*3+1,pixels+(y*vd->texture_w+x)*3+2);

    glBindTexture(GL_TEXTURE_2D,vd->images[image_id-1]);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGB8,vd->texture_w,vd->texture_h,0,GL_RGB,GL_UNSIGNED_BYTE,((GLubyte*)pixels));
    glBindTexture(GL_TEXTURE_2D,0);

    free(pixels);
}

void process_image(vectorizer_data * vd)
{
    puts("process_image");

    int32_t w=vd->w;
    int32_t h=vd->h;

    int32_t i,j;

    filter_data * gaussian_filter_datum;
    uint32_t gaussian_filter_datum_count;

    gaussian_filter_datum=NULL;

    if(vd->blur_factor>0)
    {
        gaussian_filter_datum_count=set_gaussian_filter_data(&gaussian_filter_datum,vd->blur_factor);

        for(i=0;i<w;i++) for(j=0;j<h;j++) vd->blurred[j*w+i]=gaussian_filter_pixel_colour(gaussian_filter_datum,gaussian_filter_datum_count,vd->origional,i,j,w,h);
    }
    else for(i=0;i<(w*h);i++) vd->blurred[i]=vd->origional[i];

    for(i=0;i<w;i++)for(j=0;j<h-1;j++)
    {
        vd->v_grad[j*w+i].l=vd->blurred[j*w+i+w].l-vd->blurred[j*w+i].l;
        vd->v_grad[j*w+i].a=vd->blurred[j*w+i+w].a-vd->blurred[j*w+i].a;
        vd->v_grad[j*w+i].b=vd->blurred[j*w+i+w].b-vd->blurred[j*w+i].b;
    }

    for(i=0;i<w-1;i++)for(j=0;j<h;j++)
    {
        vd->h_grad[j*(w-1)+i].l=vd->blurred[j*w+i+1].l-vd->blurred[j*w+i].l;
        vd->h_grad[j*(w-1)+i].a=vd->blurred[j*w+i+1].a-vd->blurred[j*w+i].a;
        vd->h_grad[j*(w-1)+i].b=vd->blurred[j*w+i+1].b-vd->blurred[j*w+i].b;
    }

    upload_modified_image_colour(vd,vd->blurred,VECTORIZER_RENDER_MODE_BLURRED);

    free(gaussian_filter_datum);
}
