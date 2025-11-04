#[vertex]
#version 450

layout(location = 0) in vec3 vertex;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 uv;

layout(location = 0) out vec3 frag_pos;
layout(location = 1) out vec3 frag_normal;
layout(location = 2) out vec2 frag_uv;
layout(location = 3) out float frag_depth;

layout(set = 0, binding = 0) uniform Matrices {
    mat4 projection_matrix;
    mat4 view_matrix;
    mat4 model_matrix;
};

void main() {
    vec4 world_pos = model_matrix * vec4(vertex, 1.0);
    vec4 view_pos = view_matrix * world_pos;
    gl_Position = projection_matrix * view_pos;
    
    frag_pos = world_pos.xyz;
    frag_normal = mat3(model_matrix) * normal;
    frag_uv = uv;
    frag_depth = gl_Position.z / gl_Position.w;
}

// fragment shader
#[fragment]
#version 450

layout(location = 0) in vec3 frag_pos;
layout(location = 1) in vec3 frag_normal;
layout(location = 2) in vec2 frag_uv;
layout(location = 3) in float frag_depth;

layout(location = 0) out vec4 accum_output;
layout(location = 1) out float reveal_output;

layout(set = 1, binding = 0) uniform sampler2D albedo_texture;

layout(set = 1, binding = 1) uniform MaterialParams {
    vec4 base_color;
    float alpha;
    float weight_bias;
    float weight_scale;
    float weight_power;
};

void main() {
    // Sample albedo texture
    vec4 color = texture(albedo_texture, frag_uv) * base_color;
    color.a *= alpha;
    
    // Early discard for fully transparent fragments
    if (color.a < 0.001) {
        discard;
    }
    
    // Normalize depth to [0, 1]
    float depth = frag_depth * 0.5 + 0.5;
    
    // WBOIT weight function
    // Higher weight = more influence on final color
    float weight = color.a * max(weight_bias, 
                                 weight_scale * pow(1.0 - depth, weight_power));
    
    // Clamp to prevent numerical instability
    weight = clamp(weight, 0.01, 3000.0);
    
    // Accumulation: store weighted premultiplied color
    accum_output = vec4(color.rgb * color.a * weight, color.a * weight);
    
    // Revealage: multiplicative blending of transparency
    reveal_output = color.a;
}