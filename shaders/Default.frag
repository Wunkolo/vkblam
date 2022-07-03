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

// Set 0: Engine Globals

// Set 1: Scene Globals

// Set 2: Material
layout( set = 0, binding = 0 ) uniform sampler2D LightmapImage;
layout( set = 1, binding = 0 ) uniform sampler2D BaseMapImage;

//layout( set = 0, binding = 0 ) uniform sampler2D DiffuseImage;

// Set 3: Object

// Attachments
layout( location = 0 ) out f32vec4 Attachment0;

void main()
{
	Attachment0 = f32vec4(
		//dot(f32vec3(InNormal), vec3(0.0, 0.0, 1.0)).xxx,
		texture(LightmapImage, InLightmapUV).rgb * texture(BaseMapImage, InUV).rgb,
		1.0
	);
}	