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

uniform float inverse_vertex_count;

uniform vec2 data[5];

uniform vec4 coords;

void main()
{
    vec2 pos;
    float f,g;

    if(gl_VertexID!=0)
    {
        f=float(gl_VertexID-1)*inverse_vertex_count;
        g=1.0-f;

        pos= f*f*(f*data[3]+3.0*g*data[2])+g*g*(g*data[0]+3.0*f*data[1]);
    }
    else
    {
        pos= data[4];
    }

    gl_Position=vec4( (pos*2.0-1)*coords.xy+coords.zw ,0.0,1.0);
}


