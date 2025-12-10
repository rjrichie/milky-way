#version 330 core

/*default camera matrices. do not modify.*/
layout(std140) uniform camera
{
    mat4 projection;	/*camera's projection matrix*/
    mat4 view;			/*camera's view matrix*/
    mat4 pvm;			/*camera's projection*view*model matrix*/
    mat4 ortho;			/*camera's ortho projection matrix*/
    vec4 position;		/*camera's position in world space*/
};

/* set light ubo. do not modify.*/
struct light
{
	ivec4 att; 
	vec4 pos; // position
	vec4 dir;
	vec4 amb; // ambient intensity
	vec4 dif; // diffuse intensity
	vec4 spec; // specular intensity
	vec4 atten;
	vec4 r;
};
layout(std140) uniform lights
{
	vec4 amb;
	ivec4 lt_att; // lt_att[0] = number of lights
	light lt[4];
};

// include engine-provided material uniforms (mat_amb, mat_dif, mat_spec, mat_shinness)
~include material;

/*input variables*/
in vec3 vtx_normal; // vtx normal in world space
in vec3 vtx_position; // vtx position in world space
in vec3 vtx_model_position; // vtx position in model space
in vec4 vtx_color;
in vec2 vtx_uv;
in vec3 vtx_tangent;

// `mat_amb`, `mat_dif`, `mat_spec`, `mat_shinness` are vec4s defined by the shared header

uniform sampler2D tex_color;   /* texture sampler for color */
uniform sampler2D tex_normal;   /* texture sampler for normal vector */
uniform float exposure = 1.0; /* simple exposure control for tone mapping */

/*output variables*/
out vec4 frag_color;

vec3 shading_texture_with_phong(light li, vec3 e, vec3 p, vec3 n)
{
    // determine light direction and attenuation
    vec3 L;
    float attenuation = 1.0;
    if (li.att[0] == 0) {
        // directional light
        L = normalize(-li.dir.xyz);
    } else {
        // point light
        vec3 lightPos = li.pos.xyz;
        vec3 toLight = lightPos - p;
        float dist = length(toLight);
        L = normalize(toLight);
        attenuation = 1.0 / (li.atten.x + li.atten.y * dist + li.atten.z * dist * dist + 1e-6);
    }

    // diffuse term
    float dif_coef = max(dot(n, L), 0.0);
    vec3 diffuse = dif_coef * (li.dif.rgb * mat_dif.rgb);

    // specular term
    vec3 V = normalize(e - p);
    vec3 H = normalize(L + V);
    float spec_power = mat_shinness[0];
    float spec_coef = pow(max(dot(n, H), 0.0), spec_power);
    vec3 specular = spec_coef * (li.spec.rgb * mat_spec.rgb);

    vec3 color = attenuation * (diffuse + specular);
    return color;
}

vec3 read_normal_texture()
{
    vec3 normal = texture(tex_normal, vtx_uv).rgb;
    normal = normalize(normal * 2.0 - 1.0);
    return normal;
}

void main()
{
    vec3 e = position.xyz;
    vec3 p = vtx_position;
    vec3 N = normalize(vtx_normal);
    vec3 T = normalize(vtx_tangent);

    vec3 texture_normal = read_normal_texture();
    vec3 texture_color = texture(tex_color, vtx_uv).rgb;

    // build final normal: if normal map present, transform from tangent space to world
    vec3 Nmap = texture_normal;
    if(length(Nmap) > 0.0) {
        vec3 T = normalize(vtx_tangent);
        vec3 B = normalize(cross(N, T));
        mat3 TBN = mat3(T, B, N);
        N = normalize(TBN * Nmap);
    }

    bool is_emissive = mat_amb.r > 0.8 && mat_spec.r < 0.1;
    
    vec3 final_color;
    if (is_emissive) {
        final_color = texture_color * 1.5; 
        frag_color = vec4(final_color, 1.0);
    } else {
        vec3 lighting = vec3(0.0);
        for(int i = 0; i < lt_att[0]; ++i){
            lighting += shading_texture_with_phong(lt[i], e, p, N);
        }
        lighting += amb.rgb * mat_amb.rgb;
        vec3 hdr = texture_color * lighting * exposure;
        vec3 mapped = hdr / (hdr + vec3(1.0));
        vec3 gamma_corrected = pow(clamp(mapped, 0.0, 1.0), vec3(1.0/2.2));

        frag_color = vec4(gamma_corrected, 1.0);
    }
}