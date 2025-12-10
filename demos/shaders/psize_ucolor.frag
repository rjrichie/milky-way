#version 330 core

in vec4 vtx_color;
uniform sampler2D tex_color;
out vec4 frag_color;

void main()
{
    // gl_PointCoord contains the point-sprite UV
    vec2 uv = gl_PointCoord.xy;
    vec4 tex = texture(tex_color, uv);
    vec4 col = tex * vtx_color;
    if(col.a < 0.01) discard;
    frag_color = col;
}
