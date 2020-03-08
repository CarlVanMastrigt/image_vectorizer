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
