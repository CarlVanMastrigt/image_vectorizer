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
