#version 460
#extension GL_EXT_shader_explicit_arithmetic_types : require

layout( push_constant ) uniform Constants {
	layout(offset = 64) f32vec4 Color;
} PushConstant;

layout( location = 0 ) out f32vec4 Attachment0;

void main()
{
	Attachment0 = PushConstant.Color;
}	