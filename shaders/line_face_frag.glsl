out vec3 colour;

uniform vec3 c;

void main()
{
    if(gl_FrontFacing)colour=-c;
    else colour=c;
}
