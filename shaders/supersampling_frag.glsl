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

uniform sampler2D colour;

out vec3 col;

uniform int supersample_magnitude;



void main()
{
    ivec2 tc=ivec2(gl_FragCoord.xy)*supersample_magnitude;
    col=vec3(0,0,0);

    ivec2 o;
    for(o.x=0;o.x<supersample_magnitude;o.x++)
    {
        for(o.y=0;o.y<supersample_magnitude;o.y++)
        {
            col+= clamp(texelFetch(colour,tc+o,0).xyz,0.0,1.0);
        }
    }

    col/=float(supersample_magnitude*supersample_magnitude);

    //col=vec3(1,0,0);
}
