#include "main_entry.h"

#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/classes/rd_texture_format.hpp>
#include <godot_cpp/classes/rd_attachment_format.hpp>
#include <godot_cpp/classes/rd_uniform.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/mesh_instance3d.hpp>
#include <godot_cpp/classes/viewport.hpp>

#include <godot_cpp/classes/rd_texture_view.hpp>

void Sprite2D_Jiggle::_bind_methods() {

}

void Sprite2D_Jiggle::_process(double delta) {
    
    m_time_accumulator += static_cast<float>(delta);
    Vector2 new_position = Vector2(10.0 + (10.0 * sin(m_time_accumulator * 2.0)), 10.0 + (10.0 * cos(m_time_accumulator * 1.5)));

	set_position(new_position);
}


void WBOITRenderer::_bind_methods() {
    ClassDB::bind_method(D_METHOD("initialize", "size"), &WBOITRenderer::initialize);
    ClassDB::bind_method(D_METHOD("render_transparent_objects", "objects"), &WBOITRenderer::render_transparent_objects);
    ClassDB::bind_method(D_METHOD("composite_to_screen"), &WBOITRenderer::composite_to_screen);
    ClassDB::bind_method(D_METHOD("set_viewport_size", "size"), &WBOITRenderer::set_viewport_size);
    ClassDB::bind_method(D_METHOD("get_viewport_size"), &WBOITRenderer::get_viewport_size);
}

WBOITRenderer::WBOITRenderer() {
    m_viewport_size = Vector2i(1920, 1080);
}

WBOITRenderer::~WBOITRenderer() {
    cleanup_resources();
}

void WBOITRenderer::initialize(const Vector2i &size) {
    m_viewport_size = size;
    create_render_targets();
    create_shaders();
    m_initialized = true;
}

void WBOITRenderer::create_render_targets() {
    RenderingServer* rs = RenderingServer::get_singleton();
    RenderingDevice* rd = rs->get_rendering_device();
    
    if (!rd) {
        ERR_PRINT("RenderingDevice not available");
        return;
    }

    // Create accumulation texture (RGBA16F)
    Ref<RDTextureFormat> accum_format;
    accum_format.instantiate();
    accum_format->set_format(RenderingDevice::DATA_FORMAT_R16G16B16A16_SFLOAT);
    accum_format->set_width(m_viewport_size.x);
    accum_format->set_height(m_viewport_size.y);
    accum_format->set_usage_bits(
        RenderingDevice::TEXTURE_USAGE_COLOR_ATTACHMENT_BIT |
        RenderingDevice::TEXTURE_USAGE_SAMPLING_BIT
    );
    
    Ref<RDTextureView> accum_view;
    accum_view.instantiate();
    m_accumulation_texture = rd->texture_create(accum_format, accum_view);

    // Create revealage texture (R16F)
    Ref<RDTextureFormat> reveal_format;
    reveal_format.instantiate();
    reveal_format->set_format(RenderingDevice::DATA_FORMAT_R16_SFLOAT);
    reveal_format->set_width(m_viewport_size.x);
    reveal_format->set_height(m_viewport_size.y);
    reveal_format->set_usage_bits(
        RenderingDevice::TEXTURE_USAGE_COLOR_ATTACHMENT_BIT |
        RenderingDevice::TEXTURE_USAGE_SAMPLING_BIT
    );
    
    Ref<RDTextureView> reveal_view;
    reveal_view.instantiate();
    m_revealage_texture = rd->texture_create(reveal_format, reveal_view);
}

String load_shader_file(const String& path) {
    Ref<FileAccess> file = FileAccess::open(path, FileAccess::READ);
    if (file.is_null()) {
        ERR_PRINT(vformat("Failed to open shader file: %s", path));
        return String();
    }
    
    String content = file->get_as_text();
    file->close();
    return content;
}

void WBOITRenderer::create_shaders() {
    RenderingDevice *rd = RenderingServer::get_singleton()->get_rendering_device();
    if (!rd) {
        ERR_PRINT("RenderingDevice not available");
        return;
    }
    
    // Load shader code from files
    String accumulate_code = load_shader_file("res://shaders/wboit_accumulation.glsl");
    String composite_code = load_shader_file("res://shaders/wboit_composite.glsl");
    
    if (accumulate_code.is_empty() || composite_code.is_empty()) {
        ERR_PRINT("Failed to load WBOIT shader files");
        return;
    }
    
    // Create shader from GLSL code
    // Note: In production, you'd compile these shaders to SPIRV format
    // For now, we'll create shader objects
    
    Ref<Shader> accum_shader_obj;
    accum_shader_obj.instantiate();
    accum_shader_obj->set_code(accumulate_code);
    m_accumulate_shader = accum_shader_obj->get_rid();
    
    Ref<Shader> comp_shader_obj;
    comp_shader_obj.instantiate();
    comp_shader_obj->set_code(composite_code);
    m_composite_shader = comp_shader_obj->get_rid();
    
    UtilityFunctions::print("WBOIT shaders loaded successfully");
}

void WBOITRenderer::render_transparent_objects(const Array &objects) {
    if (!m_initialized) {
        ERR_PRINT("WBOITRenderer not initialized");
        return;
    }
    
    RenderingServer* rs = RenderingServer::get_singleton();
    RenderingDevice* rd = rs->get_rendering_device();
    
    if (!rd) {
        ERR_PRINT("RenderingDevice not available");
        return;
    }
    
    // Clear accumulation buffers to (0,0,0,0) and (1,1,1,1) respectively
    PackedFloat32Array clear_accum;
    clear_accum.resize(m_viewport_size.x * m_viewport_size.y * 4);
    clear_accum.fill(0.0f);
    
    PackedFloat32Array clear_reveal;
    clear_reveal.resize(m_viewport_size.x * m_viewport_size.y);
    clear_reveal.fill(1.0f);
    
    // Clear textures
    rd->texture_clear(m_accumulation_texture, Color(0, 0, 0, 0), 0, 1, 0, 1);
    rd->texture_clear(m_revealage_texture, Color(1, 1, 1, 1), 0, 1, 0, 1);
    
    // Create framebuffer with accumulation targets
    TypedArray<RID> fb_textures;
    fb_textures.push_back(m_accumulation_texture);
    fb_textures.push_back(m_revealage_texture);
    
    Vector<Ref<RDAttachmentFormat>> attachments;
    
    // Accumulation attachment (RGBA16F)
    Ref<RDAttachmentFormat> accum_attachment;
    accum_attachment.instantiate();
    accum_attachment->set_format(RenderingDevice::DATA_FORMAT_R16G16B16A16_SFLOAT);
    accum_attachment->set_samples(RenderingDevice::TEXTURE_SAMPLES_1);
    accum_attachment->set_usage_flags(
        RenderingDevice::TEXTURE_USAGE_COLOR_ATTACHMENT_BIT |
        RenderingDevice::TEXTURE_USAGE_SAMPLING_BIT
    );

    attachments.push_back(accum_attachment);
    
    // Revealage attachment (R16F)
    Ref<RDAttachmentFormat> reveal_attachment;
    reveal_attachment.instantiate();
    reveal_attachment->set_format(RenderingDevice::DATA_FORMAT_R16_SFLOAT);
    reveal_attachment->set_samples(RenderingDevice::TEXTURE_SAMPLES_1);
    reveal_attachment->set_usage_flags(
        RenderingDevice::TEXTURE_USAGE_COLOR_ATTACHMENT_BIT |
        RenderingDevice::TEXTURE_USAGE_SAMPLING_BIT
    );

    attachments.push_back(reveal_attachment);
    
    // Create framebuffer
    RID framebuffer = rd->framebuffer_create(fb_textures, RenderingDevice::INVALID_ID, 1);
    
    if (!framebuffer.is_valid()) {
        ERR_PRINT("Failed to create framebuffer for WBOIT accumulation");
        return;
    }

    // Set up clear values
    PackedColorArray clear_colors;
    clear_colors.push_back(Color(0, 0, 0, 0)); // Clear accumulation to transparent
    clear_colors.push_back(Color(1, 1, 1, 1)); // Clear revealage to opaque
    
    // Begin rendering to accumulation buffers
    int64_t draw_list = rd->draw_list_begin(
        framebuffer,
        RenderingDevice::INITIAL_ACTION_CLEAR | RenderingDevice::FINAL_ACTION_READ,
        clear_colors
    );
    
    // Set blend mode for additive blending
    // This is critical for WBOIT - we accumulate weighted colors
    
    // Render each transparent object
    for (int i = 0; i < objects.size(); i++) {
        Object *obj = objects[i];
        MeshInstance3D *mesh_inst = Object::cast_to<MeshInstance3D>(obj);
        
        if (!mesh_inst) continue;
        
        // Get mesh RID
        RID mesh_rid = mesh_inst->get_mesh()->get_rid();
        RID material_rid = m_accumulate_shader;
        
        // Draw the mesh with accumulation shader
        // You would need to set up vertex buffers, index buffers, etc.
        // This is a simplified version
        
        // rd->draw_list_bind_render_pipeline(draw_list, accumulate_shader);
        // rd->draw_list_draw(draw_list, ...);
    }
    
    rd->draw_list_end();
    
    // Submit and sync
    rd->submit();
    rd->sync();
    
    // Clean up framebuffer
    if (framebuffer.is_valid()) {
        rd->free_rid(framebuffer);
    }
    
    UtilityFunctions::print("Rendered ", objects.size(), " transparent objects with WBOIT");
}

void WBOITRenderer::composite_to_screen() {
    if (!m_initialized) {
        ERR_PRINT("WBOITRenderer not initialized");
        return;
    }
    
    RenderingServer *rs = RenderingServer::get_singleton();
    RenderingDevice *rd = rs->get_rendering_device();
    
    if (!rd) {
        ERR_PRINT("RenderingDevice not available");
        return;
    }
    
    // Create a fullscreen quad to apply composite shader
    // We'll draw a screen-space quad that samples the accumulation buffers
    
    // Get the current viewport
    RID viewport = rs->viewport_get_render_target(get_viewport()->get_viewport_rid());
    
    // Begin draw list for compositing
    Vector<RID> framebuffers;
    framebuffers.push_back(viewport);
    
    // Create uniform set for composite shader
    Array uniforms;
    
    // Uniform 0: Accumulation texture
    Ref<RDUniform> accum_uniform;
    accum_uniform.instantiate();
    accum_uniform->set_uniform_type(RenderingDevice::UNIFORM_TYPE_SAMPLER_WITH_TEXTURE);
    accum_uniform->set_binding(0);
    accum_uniform->add_id(m_accumulation_texture);
    uniforms.push_back(accum_uniform);
    
    // Uniform 1: Revealage texture
    Ref<RDUniform> reveal_uniform;
    reveal_uniform.instantiate();
    reveal_uniform->set_uniform_type(RenderingDevice::UNIFORM_TYPE_SAMPLER_WITH_TEXTURE);
    reveal_uniform->set_binding(1);
    reveal_uniform->add_id(m_revealage_texture);
    uniforms.push_back(reveal_uniform);
    
    // Create uniform set
    RID uniform_set = rd->uniform_set_create(uniforms, m_composite_shader, 0);
    
    if (!uniform_set.is_valid()) {
        ERR_PRINT("Failed to create uniform set for composite");
        return;
    }
    
    // Start compute dispatch or draw call
    int64_t compute_list = rd->compute_list_begin();
    rd->compute_list_bind_compute_pipeline(compute_list, m_composite_shader);
    rd->compute_list_bind_uniform_set(compute_list, uniform_set, 0);
    
    // Dispatch compute shader (8x8 work groups)
    int groups_x = (m_viewport_size.x + 7) / 8;
    int groups_y = (m_viewport_size.y + 7) / 8;
    rd->compute_list_dispatch(compute_list, groups_x, groups_y, 1);
    
    rd->compute_list_end();
    
    // Submit and sync
    rd->submit();
    rd->sync();
    
    // Clean up uniform set
    if (uniform_set.is_valid()) {
        rd->free_rid(uniform_set);
    }
    
    UtilityFunctions::print("Composited WBOIT to screen");
}

void WBOITRenderer::set_viewport_size(const Vector2i &size) {
    if (m_viewport_size != size) {
        m_viewport_size = size;
        if (m_initialized) {
            cleanup_resources();
            initialize(size);
        }
    }
}

Vector2i WBOITRenderer::get_viewport_size() const {
    return m_viewport_size;
}

Ref<Shader> WBOITRenderer::get_accumulation_shader() const {
    Ref<Shader> shader;
    shader.instantiate();
    //shader->set_rid(m_accumulate_shader);
    return shader;
}

Ref<Shader> WBOITRenderer::get_composite_shader() const {
    Ref<Shader> shader;
    shader.instantiate();
    //shader->set_rid(m_composite_shader);
    return shader;
}

void WBOITRenderer::cleanup_resources() {
    RenderingDevice *rd = RenderingServer::get_singleton()->get_rendering_device();
    if (rd) {
        if (m_accumulation_texture.is_valid()) {
            rd->free_rid(m_accumulation_texture);
        }
        if (m_revealage_texture.is_valid()) {
            rd->free_rid(m_revealage_texture);
        }
    }
    m_initialized = false;
}