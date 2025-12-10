
#version 330 core

layout(std140) uniform camera {
    mat4 projection;
    mat4 view;
    mat4 pvm;
    mat4 ortho;
    vec4 position;
};

uniform float iTime;
uniform int iFrame;

in vec3 vtx_model_position; // direction vector (cube-vertex)

out vec4 frag_color;

#define NUM_STAR 200
#define PI 3.14159265358979323846

vec2 hash2d(float t)
{
    t += 1.;
    float x = fract(sin(t * 674.3) * 453.2);
    float y = fract(sin((t + x) * 714.3) * 263.2);

    return vec2(x, y);
}

// convert a 2D hash to dir on the unit sphere
vec3 dir_from_hash2(vec2 p)
{
    float lat = p.y * (PI * 0.5); 
    float lon = p.x * PI;
    float cl = cos(lat);
    return normalize(vec3(cl * cos(lon), sin(lat), cl * sin(lon)));
}

vec3 renderParticleOnSphere(vec3 dir, vec3 starDir, float brightness, vec3 color)
{
    float ang = acos(clamp(dot(dir, starDir), -1.0, 1.0));
    float d = max(ang, 0.0001);
    return (brightness / d) * color * 0.02;
}

vec3 renderStarsOnSphere(vec3 dir)
{
    vec3 fragColor = vec3(0.0);
    float t = iTime;
    float projScale = 1.0 / projection[1][1];
    for (int ii = 0; ii < NUM_STAR; ++ii)
    {
        float i = float(ii);
        vec2 h = hash2d(i);
        vec2 p = h * 2.0 - 1.0;
        vec3 sdir = dir_from_hash2(p);

        float brightness = .02 * projScale * 1.5;
        brightness *= sin(1.5 * t + i) * .5 + .5;
        vec3 color = vec3(0.4, 0.4, 0.4);

        fragColor += renderParticleOnSphere(dir, sdir, brightness, color) * 4.0;
    }
    return fragColor;
}

void main()
{
    vec3 dir = normalize(vtx_model_position);
    vec3 outputColor = renderStarsOnSphere(dir);
    frag_color = vec4(outputColor, 1.0);
}
