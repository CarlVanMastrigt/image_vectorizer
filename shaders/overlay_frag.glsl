out vec4 colour;

flat in ivec4 data1_;
flat in ivec4 data2_;
flat in ivec4 data3_;

uniform vec4 colours[NUM_OVERLAY_COLOURS];

uniform sampler2D text_images[4];

uniform sampler2D shaded_image;
uniform sampler2D coloured_image;

uniform int window_height;

void blend_colours(inout vec4 back,in vec4 front)
{
    back.xyz= (1.0-front.w)*back.xyz   +front.w*front.xyz;
    back.w=   (1.0-front.w)*back.w     +front.w;
}

void main()
{
    ivec2 coords=ivec2(gl_FragCoord.xy);
    coords.y=window_height-coords.y-1;

    if(data2_.x==4)
    {
        colour=texelFetch(coloured_image,coords-data3_.xy,0);
    }
    else
    {
        colour=colours[data2_.y-1];
    }

    if((data2_.x==1)&&(coords.x>=data3_.x)&&(coords.y>=data3_.y)&&(coords.x<data3_.z)&&(coords.y<data3_.w))
    {
        discard;
    }

    if(data2_.x==2)
    {
        coords-=data3_.xy;
        coords.x+=data2_.z;
        colour.w*=texelFetch(text_images[data2_.w],coords,0).x;
    }

    if(data2_.x==3)
    {
        coords+=data2_.zw-data3_.xy;

		if(data2_.z<0)/// REMOVE ????
		{
			coords.x=-data2_.z;
		}

		if(data2_.w<0)/// REMOVE ????
		{
			coords.y=-data2_.w;
		}

        colour.w*=texelFetch(shaded_image,coords,0).x;
    }




}
