out vec3 colour;

in float f;

uniform vec3 c;

void main()
{
    colour=vec3(f,0.0,1.0-f);
    //colour=vec3(1,0,0);
}

