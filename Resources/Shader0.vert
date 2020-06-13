#version 450
#extension GL_ARB_separate_shader_objects : enable
//#extension GL_ARB_shading_language_420pack : enable


layout(set = 0, binding = 0) uniform UniformBufferObject { 
    mat4 model;
    mat4 view;
    mat4 projection;
} ubo;

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inColour;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec3 inTangent;
layout(location = 4) in vec2 inTexCoord;

layout(location = 0) out vec3 outColour;
layout(location = 1) out vec3 outNormal;
layout(location = 2) out vec3 outTangent;
layout(location = 3) out vec2 outTexCoord;

out gl_PerVertex {
    vec4 gl_Position;
};


void main() {
    gl_Position = ( ubo.projection * ubo.view * ubo.model ) * vec4( inPos, 1.0 );
    outColour = inColour;

    outTexCoord = inTexCoord;
    outTexCoord.y = 1.f - outTexCoord.y;

    outNormal = ( ubo.model * vec4( inNormal, 0.f ) ).xyz;
    outTangent = ( ubo.model * vec4( inTangent, 0.f ) ).xyz;
}
