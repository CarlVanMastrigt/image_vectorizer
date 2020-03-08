uniform float inverse_vertex_count;

uniform vec2 data[4];

uniform vec4 coords;

out float f;

void main()
{
    f=float(gl_VertexID)*inverse_vertex_count;
    float g=1.0-f;

    vec2 pos= f*f*(f*data[0]+3.0*g*data[1])+g*g*(g*data[3]+3.0*f*data[2]);
    //vec2 pos= f*f*data[0]+2.0*f*g*data[1]+g*g*data[3];

    gl_Position=vec4( (pos*2.0-1)*coords.xy+coords.zw ,0.0,1.0);
}

