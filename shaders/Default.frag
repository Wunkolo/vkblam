#version 460
#extension GL_EXT_shader_explicit_arithmetic_types : require

layout( location = 0 ) in f32vec3 InPosition;
layout( location = 1 ) in f32vec3 InNormal;
layout( location = 2 ) in f32vec3 InBinormal;
layout( location = 3 ) in f32vec3 InTangent;
layout( location = 4 ) in f32vec2 InUV;

layout( location = 5 ) in f32vec3 InLightmapNormal;
layout( location = 6 ) in f32vec2 InLightmapUV;

layout( location = 0 ) out f32vec4 Attachment0;

void main()
{
	Attachment0 = f32vec4(
		dot(f32vec3(InNormal), vec3(0.0, 0.0, 1.0)).xxx,
		1.0
	);
}	