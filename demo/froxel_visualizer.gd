extends Node

@onready var camera = $Camera3D
@onready var froxel_viz = $FroxelVisualizer

func _ready():
	# Position camera
	#camera.position = Vector3(0, 2, 5)
	#camera.look_at(Vector3.ZERO)
	
	# Configure froxel visualizer
	froxel_viz.set_camera(camera)
	froxel_viz.set_froxel_count(16, 9, 48)
	
#func _process(delta):
	# Optional: rotate camera for demo
	#camera.rotate_y(delta * 0.2)
