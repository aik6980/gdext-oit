#pragma once

#include "godot_cpp/classes/sprite2d.hpp"

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/shader.hpp>

using namespace godot;

// Simple class to kick start my GDExtension study
class Sprite2D_Jiggle : public Sprite2D {
	GDCLASS(Sprite2D_Jiggle, Sprite2D)

public:
	Sprite2D_Jiggle() = default;
	~Sprite2D_Jiggle() override = default;

    void _process(double delta) override;
protected:
	static void _bind_methods();

    float m_time_accumulator = 0.0f;
};

// Let try implement WBOIT
class WBOITRenderer : public Node {
	GDCLASS(WBOITRenderer, Node)

public:
    WBOITRenderer();
    ~WBOITRenderer();

    void initialize(const Vector2i &size);
    void render_transparent_objects(const Array &objects);
    void composite_to_screen();
    
    void set_viewport_size(const Vector2i &size);
    Vector2i get_viewport_size() const;
    
    Ref<Shader> get_accumulation_shader() const;
    Ref<Shader> get_composite_shader() const;

protected:
	static void _bind_methods();

private:
	// https://docs.godotengine.org/en/stable/classes/class_rid.html
	// Resource ID handles for render targets and shaders
    RID m_accumulation_fb;
    RID m_accumulation_texture;
    RID m_revealage_texture;
    RID m_composite_shader;
    RID m_accumulate_shader;
    
    bool m_initialized = false;
    Vector2i m_viewport_size;

    void create_render_targets();
    void create_shaders();
    void cleanup_resources();
};