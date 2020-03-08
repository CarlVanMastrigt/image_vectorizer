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

in vec2 pos;

out vec2 tc;

uniform vec4 coords;
uniform vec2 relative_size;///image to texture ratio, 1.0 or very slightly lower

void main()
{
    tc=(pos*0.5+0.5)*relative_size;
    gl_Position=vec4( pos*coords.xy+coords.zw ,0.0,1.0);
}
