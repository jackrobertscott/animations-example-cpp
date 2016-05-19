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

varying vec2 texCoord;
varying vec3 fragVert;
varying vec3 fragNorm;

uniform mat4 ModelView;
uniform mat4 Projection;

uniform float Waves;
uniform float time;

void main()
{

    fragVert = vPosition;

    if(Waves == 1.0)
    {
      fragVert.z =  sin(vPosition.z + time/1000.0) * (sin(2.5 * vPosition.y + time/1000.0 ) * cos(1.5 * vPosition.x + time/1000.0) * 0.2);
    }
    else if(Waves == 2.0)
    {
    mat3 WavesX = mat3( vec3(1.0, 0.0, 0.0),
                        vec3(0.0, cos(time/1000.0 + vPosition.x/100.0) + 2.0, 0.0),
                        vec3(0.0,  0.0, 1.0));
    fragVert = fragVert * WavesX;
    }


    fragNorm = vNormal;
    texCoord = vTexCoord;


    gl_Position = Projection * ModelView * vec4(fragVert, 1.0);

}
