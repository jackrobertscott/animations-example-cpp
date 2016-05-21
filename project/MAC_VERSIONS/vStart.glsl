/**
 * CITS3003 Project
 * Tahmer Hijjawi 21130321
 * Jack Scott	21504053
 * 2016 Sem 1
 */
#version 120

attribute vec3 vPosition;
attribute vec3 vNormal;
attribute vec2 vTexCoord;
attribute vec4 vBoneIDs;
attribute vec4 vBoneWeights;

varying vec2 texCoord;
varying vec4 fPosition;
varying vec4 fNormal;
varying mat4 boneTransform;

uniform mat4 ModelView;
uniform mat4 Projection;
uniform mat4 uBoneTransforms[64];
uniform float Waves;
uniform float time;

void main()
{
    // Part B - D: calculate bone transformation
    ivec4 bone = ivec4(vBoneIDs); // convert vBoneIDs to ivec4
    boneTransform = vBoneWeights[0] * uBoneTransforms[bone[0]] +
      vBoneWeights[1] * uBoneTransforms[bone[1]] +
      vBoneWeights[2] * uBoneTransforms[bone[2]] +
      vBoneWeights[3] * uBoneTransforms[bone[3]];

    vec4 vTransPos = boneTransform * vec4(vPosition, 1.0);
    vec4 vTransNorm = boneTransform * vec4(vNormal, 0.0);

    if(Waves == 1.0)
    {
      vTransPos.z =  sin(vTransPos.z + time/1000.0) * (sin(2.5 * vTransPos.y + time/1000.0 ) * cos(1.5 * vTransPos.x + time/1000.0) * 0.2);
    }
    else if(Waves == 2.0)
    {
      mat3 WavesX = mat3( vec3(1.0, 0.0, 0.0),
                          vec3(0.0, cos(time/1000.0 + vTransPos.x/100.0) + 2.0, 0.0),
                          vec3(0.0,  0.0, 1.0));
      vTransPos.xyz = vTransPos.xyz * WavesX;
    }

    // Transform vertex position and vertex normal into eye coordinates (assumes scaling is uniform across dimensions)
    fPosition = vTransPos;
    fNormal = vTransNorm;

    gl_Position = Projection * ModelView * vTransPos;
    texCoord = vTexCoord;
}
