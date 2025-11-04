#[compute]
#version 450

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

layout(set = 0, binding = 0) uniform sampler2D accum_texture;
layout(set = 0, binding = 1) uniform sampler2D reveal_texture;
layout(set = 0, binding = 2) uniform sampler2D color_texture;
layout(rgba16f, set = 0, binding = 3) uniform writeonly image2D output_image;

void main() {
    ivec2 pixel_coords = ivec2(gl_GlobalInvocationID.xy);
    ivec2 image_size = imageSize(output_image);
    
    // Bounds check
    if (pixel_coords.x >= image_size.x || pixel_coords.y >= image_size.y) {
        return;
    }
    
    vec2 uv = (vec2(pixel_coords) + 0.5) / vec2(image_size);
    
    // Sample accumulation buffers
    vec4 accum = textureLod(accum_texture, uv, 0.0);
    float reveal = textureLod(reveal_texture, uv, 0.0).r;
    
    // Sample opaque background
    vec4 background = textureLod(color_texture, uv, 0.0);
    
    // Composite transparent layer
    vec3 transparent_color;
    float transparent_alpha;
    
    if (accum.a >= 0.00001) {
        // Average weighted color
        transparent_color = accum.rgb / accum.a;
        
        // Calculate final alpha
        transparent_alpha = 1.0 - reveal;
        
        // Blend with background using over operator
        vec3 final_color = transparent_color * transparent_alpha + 
                          background.rgb * (1.0 - transparent_alpha);
        
        imageStore(output_image, pixel_coords, vec4(final_color, 1.0));
    } else {
        // No transparency at this pixel
        imageStore(output_image, pixel_coords, background);
    }
}