out vec3 colour;


in vec2 tc;

uniform sampler2D image;

void main()
{
    colour=texture(image,tc).xyz;
    //colour=vec3(tc,0.5);
    //if((tc.x>1.0)||(tc.y>1.0))discard;
}

