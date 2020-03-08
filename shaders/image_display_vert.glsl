in vec2 pos;

out vec2 tc;

uniform vec4 coords;
uniform vec2 relative_size;///image to texture ratio, 1.0 or very slightly lower

void main()
{
    tc=(pos*0.5+0.5)*relative_size;
    gl_Position=vec4( pos*coords.xy+coords.zw ,0.0,1.0);
}
