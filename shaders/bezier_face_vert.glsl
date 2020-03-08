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


