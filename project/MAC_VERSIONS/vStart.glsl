/**
 * CITS3003 Project
 * Tahmer Hijjawi 21130321
 * Jack Scott	21504053
 * 2016 Sem 1
 */

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

void main()
{
    // Part B - D: calculate bone transformation
    ivec4 bone = ivec4(vBoneIDs); // convert vBoneIDs to ivec4
    boneTransform = vBoneWeights[0] * uBoneTransforms[bone[0]] +
			 vBoneWeights[1] * uBoneTransforms[bone[1]] +
			 vBoneWeights[2] * uBoneTransforms[bone[2]] +
			 vBoneWeights[3] * uBoneTransforms[bone[3]];

    vec4 vTransPos = vec4(vPosition, 1.0) * boneTransform;
    vec4 vTransNorm = vec4(vNormal, 0.0) * boneTransform;

    // Transform vertex position and vertex normal into eye coordinates (assumes scaling is uniform across dimensions)
    fPosition = vTransPos;
    fNormal = vTransNorm;

    gl_Position = Projection * ModelView * vTransPos;
    texCoord = vTexCoord;
}
