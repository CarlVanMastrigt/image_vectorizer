out vec3 colour;

in float factor;

uniform vec3 c;

void main()
{
    colour=c*(0.5+8*(factor-0.5)*(factor-0.5)*(factor-0.5)*(factor-0.5));
    //colour=vec3((factor-0.5)*(factor-0.5)*4);
    //colour=vec3(1,0,0);
}
