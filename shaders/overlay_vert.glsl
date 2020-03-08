in vec2 vertex;
in ivec4 data1;
in ivec4 data2;
in ivec4 data3;

uniform vec2 inv_window_size;

flat out ivec4 data1_;
flat out ivec4 data2_;
flat out ivec4 data3_;

void main()
{
    vec4 ps=vec4(data1);
    gl_Position=vec4( ((ps.xy+ps.zw*vertex)*inv_window_size-0.5)*vec2(1.0,-1.0) ,0.0,0.5);
    //gl_Position=vec4( vertex ,0.0,0.5);

    data1_=data1;
    data2_=data2;
    data3_=data3;
}
