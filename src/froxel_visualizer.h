
#include <godot_cpp/classes/mesh_instance3d.hpp>
#include <godot_cpp/classes/immediate_mesh.hpp>
#include <godot_cpp/classes/standard_material3d.hpp>
#include <godot_cpp/classes/camera3d.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/core/class_db.hpp>

using namespace godot;

class FroxelVisualizer : public MeshInstance3D {
    GDCLASS(FroxelVisualizer, MeshInstance3D)

public:
    FroxelVisualizer();

    void _notification(int p_what);
    void _ready() override;
    void _process(double delta) override;

    void set_camera(Camera3D* cam);
    Camera3D* get_camera() const {
        return camera;
    }

    void set_froxel_count(int x, int y, int z);
    void set_show_grid(bool enabled);
    void FroxelVisualizer::update_visualization();
    float get_depth_at_slice(int slice);
    Vector3 frustum_corner(float depth, float norm_x, float norm_y);

protected:
    static void _bind_methods();
private:
    Ref<ImmediateMesh> mesh;
    Ref<StandardMaterial3D> material;
    Camera3D* camera = nullptr;
    
    int froxel_count_x = 16;
    int froxel_count_y = 9;
    int froxel_count_z = 48;
    float near_plane = 0.1f;
    float far_plane = 100.0f;
    bool show_grid = true;
};