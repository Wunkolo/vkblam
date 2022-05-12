#version 460

#extension GL_GOOGLE_include_directive : require

#include "vkBlam.glsl"

layout( push_constant ) uniform PushConstants {
	CameraGlobals Camera;
};

// Input vertex data: Standard vertex
layout( location = 0 ) in f32vec3 InPosition;
layout( location = 1 ) in f32vec3 InNormal;
layout( location = 2 ) in f32vec3 InBinormal;
layout( location = 3 ) in f32vec3 InTangent;
layout( location = 4 ) in f32vec2 InUV;

// Input vertex data: Lightmap-vertex
layout( location = 5 ) in f32vec3 InLightmapNormal;
layout( location = 6 ) in f32vec2 InLightmapUV;

// Output vertex data
layout( location = 0 ) out f32vec3 OutPosition;
layout( location = 1 ) out f32vec3 OutNormal;
layout( location = 2 ) out f32vec3 OutBinormal;
layout( location = 3 ) out f32vec3 OutTangent;
layout( location = 4 ) out f32vec2 OutUV;

layout( location = 5 ) out f32vec3 OutLightmapNormal;
layout( location = 6 ) out f32vec2 OutLightmapUV;

void main()
{
	OutPosition			= InPosition;
	OutNormal			= InNormal;
	OutBinormal			= InBinormal;
	OutTangent			= InTangent;
	OutUV				= InUV;

	OutLightmapNormal 	= InLightmapNormal;
	OutLightmapUV 		= InLightmapUV;
	
	gl_Position			= Camera.ViewProjection * vec4( InPosition.xyz, 1.0 );
}