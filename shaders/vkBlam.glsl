#extension GL_EXT_shader_explicit_arithmetic_types : enable

struct CameraGlobals
{
	f32mat4x4 View;
	f32mat4x4 Projection;
	f32mat4x4 ViewProjection;
};

struct SimulationGlobals
{
	float32_t Time;
};
struct PassGlobals
{
	f32vec4 ScreenSize; // {width, height, 1/width, 1/height}
};