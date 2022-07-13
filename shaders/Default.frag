#version 460
#extension GL_EXT_shader_explicit_arithmetic_types : require

// Input vertex data: Standard vertex
layout( location = 0 ) in f32vec3 InPosition;
layout( location = 1 ) in f32vec3 InNormal;
layout( location = 2 ) in f32vec3 InBinormal;
layout( location = 3 ) in f32vec3 InTangent;
layout( location = 4 ) in f32vec2 InUV;

// Input vertex data: Lightmap-vertex
layout( location = 5 ) in f32vec3 InLightmapNormal;
layout( location = 6 ) in f32vec2 InLightmapUV;

//// Descriptor sets

// Set 0: Scene Globals
layout( set = 0, binding = 0 ) uniform sampler Default2DSamplerFiltered;
layout( set = 0, binding = 1 ) uniform sampler Default2DSamplerUnfiltered;

// Set 1: Shader
layout( set = 1, binding = 0 ) uniform texture2D BaseMapImage;

// Set 2: Object
layout( set = 2, binding = 0 ) uniform texture2D LightmapImage;

// Attachments
layout( location = 0 ) out f32vec4 Attachment0;

void main()
{
	Attachment0 = f32vec4(
		//dot(f32vec3(InNormal), vec3(0.0, 0.0, 1.0)).xxx,
		texture(sampler2D(LightmapImage, Default2DSamplerFiltered), InLightmapUV).rgb
		* texture(sampler2D(BaseMapImage, Default2DSamplerFiltered), InUV).rgb,
		1.0
	);
}	