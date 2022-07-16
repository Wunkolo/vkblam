#version 460
#extension GL_EXT_shader_explicit_arithmetic_types : require
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

//// Descriptor sets

// Set 0: Scene Globals
layout( set = 0, binding = 0 ) uniform sampler Default2DSamplerFiltered;
layout( set = 0, binding = 1 ) uniform sampler Default2DSamplerUnfiltered;
layout( set = 0, binding = 2 ) uniform sampler DefaultCubeSampler;

// Set 1: Shader
layout( set = 1, binding = 0 ) uniform texture2D BaseMapImage;
layout( set = 1, binding = 1 ) uniform texture2D PrimaryDetailMapImage;
layout( set = 1, binding = 2 ) uniform texture2D SecondaryDetailMapImage;
layout( set = 1, binding = 3 ) uniform texture2D MicroDetailMapImage;
layout( set = 1, binding = 4 ) uniform texture2D BumpMapImage;
layout( set = 1, binding = 5 ) uniform texture2D GlowMapImage;
layout( set = 1, binding = 6 ) uniform textureCube ReflectionCubeMapImage;

// Set 2: Object
layout( set = 2, binding = 0 ) uniform texture2D LightmapImage;

// Attachments
layout( location = 0 ) out f32vec4 Attachment0;

f32vec3 Glow(f32vec2 UV)
{
	const f32vec3 GlowSample = texture(sampler2D(GlowMapImage, Default2DSamplerFiltered), InUV).rgb;

	f32vec3 GlowResult = f32vec3(0, 0, 0);

	// Primary Animation Color
	const f32vec3 PrimaryOnColor = f32vec3(1, 1, 1);
	const f32vec3 PrimaryOffColor = f32vec3(1, 1, 1);
	const float32_t PrimaryAnimationValue = 1.0;
	GlowResult += GlowSample.r * mix(PrimaryOffColor, PrimaryOnColor, PrimaryAnimationValue);

	// Secondary Animation Color
	const f32vec3 SecondaryOnColor = f32vec3(1, 1, 1);
	const f32vec3 SecondaryOffColor = f32vec3(1, 1, 1);
	const float32_t SecondaryAnimationValue = 1.0;
	GlowResult += GlowSample.g * mix(SecondaryOffColor, SecondaryOnColor, SecondaryAnimationValue);

	// Plasma Animation Color
	const f32vec3 PlasmaOnColor = f32vec3(1, 1, 1);
	const f32vec3 PlasmaOffColor = f32vec3(1, 1, 1);
	const float32_t PlasmaAnimationValue = 1.0;
	GlowResult += GlowSample.b * mix(PlasmaOffColor, PlasmaOnColor, PlasmaAnimationValue);

	return GlowResult;
}

f32vec3 Reflection(f32vec3 Normal)
{
	const f32vec3 CameraPos = (Camera.View * vec4(0.0, 0.0, 0.0, 1.0)).xyz;
	const f32vec3 ReflectionDirection = reflect(normalize(CameraPos - InPosition), Normal);

	const f32vec3 CubeSample = texture(
		samplerCube(ReflectionCubeMapImage, DefaultCubeSampler),
		ReflectionDirection).rgb;

	// Todo, fresnel

	return CubeSample;
}

f32vec3 BumpedNormal()
{
	const f32vec4 BumpSample = texture(sampler2D(BumpMapImage, Default2DSamplerFiltered), InUV);
	const f32vec3 BumpVector = normalize(BumpSample.xyz * 2.0 - 1.0);
	
	const f32mat3 Basis = f32mat3(InTangent, InBinormal, InNormal);

	return normalize(Basis * BumpVector);
}

void main()
{
	const f32vec4 DiffuseSample = texture(sampler2D(BaseMapImage, Default2DSamplerFiltered), InUV);
	const f32vec4 LightmapSample = texture(sampler2D(LightmapImage, Default2DSamplerFiltered), InLightmapUV);
	
	const f32vec3 Normal = BumpedNormal();

	Attachment0 = f32vec4(
		(DiffuseSample.rgb + Reflection(Normal) * DiffuseSample.a) * LightmapSample.rgb
		+ Glow(InUV),
		1.0
	);
}	