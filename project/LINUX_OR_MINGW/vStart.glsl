/**
 * CITS3003 Project
 * Tahmer Hijjawi 21130321
 * Jack Scott	21504053
 * 2016 Sem 1
 */

 #version 130

attribute vec3 vPosition;
attribute vec3 vNormal;
attribute vec2 vTexCoord;

varying vec2 texCoord;
varying vec3 fragVert;
varying vec3 fragNorm;

uniform mat4 ModelView;
uniform mat4 Projection;

void main()
{

    fragVert = vPosition;
    fragNorm = vNormal;
    texCoord = vTexCoord;

    gl_Position = Projection * ModelView * vec4(vPosition, 1.0);

}
