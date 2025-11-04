#include "froxel_visualizer.h"

#include <godot_cpp/classes/viewport.hpp>

FroxelVisualizer::FroxelVisualizer() 
{

}

void FroxelVisualizer::_bind_methods() 
{
    UtilityFunctions::print("=== Binding FroxelVisualizer ===");

    ClassDB::bind_method(D_METHOD("set_froxel_count", "x", "y", "z"), &FroxelVisualizer::set_froxel_count);
    ClassDB::bind_method(D_METHOD("set_camera", "camera"), &FroxelVisualizer::set_camera);
    ClassDB::bind_method(D_METHOD("get_camera"), &FroxelVisualizer::get_camera);
    ClassDB::bind_method(D_METHOD("update_visualization"), &FroxelVisualizer::update_visualization);
    ClassDB::bind_method(D_METHOD("set_show_grid", "enabled"), &FroxelVisualizer::set_show_grid);
    
    // Export the property - this makes it visible in inspector!
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "camera", PROPERTY_HINT_NODE_TYPE, "Camera3D"), "set_camera", "get_camera");
    //ADD_PROPERTY(PropertyInfo(Variant::BOOL, "show_grid"), "set_show_grid", "");
    
    UtilityFunctions::print("=== Binding complete ===");
}

void FroxelVisualizer::_notification(int p_what)
{
    switch (p_what) 
    {
        case NOTIFICATION_PREDELETE:
            // Cleanup if needed
            mesh = Ref<ImmediateMesh>();
            material = Ref<StandardMaterial3D>();
            break;
    }
}

void FroxelVisualizer::_ready()
{
    mesh.instantiate();
    set_mesh(mesh);
    
    material.instantiate();
    material->set_shading_mode(StandardMaterial3D::SHADING_MODE_UNSHADED);
    material->set_albedo(Color(0.2f, 0.8f, 1.0f, 0.3f));
    material->set_transparency(StandardMaterial3D::TRANSPARENCY_ALPHA);
    material->set_cull_mode(StandardMaterial3D::CULL_DISABLED);
    set_material_override(material);
}

void FroxelVisualizer::_process(double delta)
{
    update_visualization();
}

void FroxelVisualizer::set_camera(Camera3D* cam) 
{
    camera = cam;
    update_visualization();
}

void FroxelVisualizer::set_froxel_count(int x, int y, int z) 
{
    froxel_count_x = x;
    froxel_count_y = y;
    froxel_count_z = z;
    update_visualization();
}

void FroxelVisualizer::set_show_grid(bool enabled)
{
    show_grid = enabled;
    update_visualization();
}

// Convert depth slice to exponential depth distribution
float FroxelVisualizer::get_depth_at_slice(int slice) 
{
    float t = (float)slice / (float)froxel_count_z;
    // Exponential distribution for better near-field resolution
    return near_plane * pow(far_plane / near_plane, t);
}

Vector3 FroxelVisualizer::frustum_corner(float depth, float norm_x, float norm_y)
{
    if (!camera) return Vector3();

    auto vp = get_viewport();
    if(!vp) return Vector3();
    
    float fov = camera->get_fov();
    float aspect = vp->get_visible_rect().size.x / 
                   vp->get_visible_rect().size.y;

    float h = 2.0f * depth * tan(Math::deg_to_rad(fov) * 0.5f);
    float w = h * aspect;
    
    return Vector3(
        (norm_x - 0.5f) * w,
        (norm_y - 0.5f) * h,
        -depth
    );
}

void FroxelVisualizer::update_visualization()
{
    if (!camera)
        return;

	if (mesh.is_null())
		return;

    if(!show_grid)
    {
        mesh->clear_surfaces();
        return;
    }
    
    mesh->clear_surfaces();
    mesh->surface_begin(Mesh::PRIMITIVE_LINES);
    
    Transform3D cam_transform = camera->get_global_transform();
    
    // Draw froxel grid
    for (int z = 0; z <= froxel_count_z; z += froxel_count_z / 8) {
        float depth = get_depth_at_slice(z);
        
        // Draw horizontal and vertical grid lines
        for (int x = 0; x <= froxel_count_x; x++) {
            float norm_x = (float)x / (float)froxel_count_x;
            Vector3 top = frustum_corner(depth, norm_x, 0.0f);
            Vector3 bottom = frustum_corner(depth, norm_x, 1.0f);
            
            mesh->surface_add_vertex(cam_transform.xform(top));
            mesh->surface_add_vertex(cam_transform.xform(bottom));
        }
        
        for (int y = 0; y <= froxel_count_y; y++) {
            float norm_y = (float)y / (float)froxel_count_y;
            Vector3 left = frustum_corner(depth, 0.0f, norm_y);
            Vector3 right = frustum_corner(depth, 1.0f, norm_y);
            
            mesh->surface_add_vertex(cam_transform.xform(left));
            mesh->surface_add_vertex(cam_transform.xform(right));
        }
    }
    
    // Draw depth slices edges
    for (int z = 0; z <= froxel_count_z; z += froxel_count_z / 8) {
        float depth = get_depth_at_slice(z);
        Vector3 corners[4] = {
            frustum_corner(depth, 0.0f, 0.0f),
            frustum_corner(depth, 1.0f, 0.0f),
            frustum_corner(depth, 1.0f, 1.0f),
            frustum_corner(depth, 0.0f, 1.0f)
        };
        
        for (int i = 0; i < 4; i++) {
            mesh->surface_add_vertex(cam_transform.xform(corners[i]));
            mesh->surface_add_vertex(cam_transform.xform(corners[(i + 1) % 4]));
        }
    }
    
    mesh->surface_end();
}
