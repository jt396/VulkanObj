#version 450
#extension GL_ARB_separate_shader_objects : enable
//#extension GL_ARB_shading_language_420pack : enable


layout(location = 0) in vec3 inColour;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inTangent;
layout(location = 3) in vec2 inTexCoord;

layout(location = 0) out vec4 outColor;

/* Bindings must match descriptor set and pipeline layout */
layout(set = 0, binding = 1) uniform sampler2D texSampler[2];
layout(set = 0, binding = 2) uniform sampler2D normalMapSampler[2];
layout(set = 0, binding = 3) uniform sampler2D specularMapSampler[2];
layout(set = 0, binding = 4) uniform DirectionalLight { 
    vec4    colour;
    vec4    direction;
    float    ambientIntensity;
    float    diffuseIntensity;
} dl;

layout(push_constant) uniform PushConstants {
    int textureIndex;
} pushConstants;


vec3 CalculateNormalFromMap() {
    vec3 normal = normalize( inNormal );
    vec3 tangent = normalize( inTangent );

    tangent = normalize( tangent - dot( tangent, normal ) * normal );
    vec3 biTangent = cross( tangent, normal );
    vec3 bumpNormal = texture( normalMapSampler[pushConstants.textureIndex], inTexCoord ).xyz;
    bumpNormal = 2.f * bumpNormal - vec3( 1.f, 1.f, 1.f );

    mat3 TBN = mat3( tangent, biTangent, normal );
    return normalize( TBN * bumpNormal );
}


void main() {
    vec4 ambientColour = dl.colour * dl.ambientIntensity;
    float diffuseFactor = dot( CalculateNormalFromMap(), vec3( -dl.direction ) );

    vec4 diffuseColour;
    if( diffuseFactor > 0.f ) {
        diffuseColour = vec4( vec3( dl.colour ) * dl.diffuseIntensity * diffuseFactor, 1.f );
    } else {
        diffuseColour = vec4( 0.f, 0.f, 0.f, 0.f );
    }

    outColor = texture( texSampler[pushConstants.textureIndex], inTexCoord ) * ( ambientColour + diffuseColour );
}
