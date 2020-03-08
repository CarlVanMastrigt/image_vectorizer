in vec2 pos;

uniform vec4 coords;

void main()
{
    //tc=pos*0.5+0.5;
    gl_Position=vec4( (pos*2.0-1)*coords.xy+coords.zw ,0.0,1.0);
}
