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

out vec3 colour;

in float factor;

uniform vec3 c;

void main()
{
    colour=c*(0.5+8*(factor-0.5)*(factor-0.5)*(factor-0.5)*(factor-0.5));
    //colour=vec3((factor-0.5)*(factor-0.5)*4);
    //colour=vec3(1,0,0);
}
