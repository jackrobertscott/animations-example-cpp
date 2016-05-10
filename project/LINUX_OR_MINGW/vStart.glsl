attribute vec3 vPosition;
attribute vec3 vNormal;
attribute vec2 vTexCoord;

varying vec2 texCoord;
varying vec4 vpos;
varying vec3 pos;
varying vec3 N;
varying vec3 Lvec;
varying vec3 L2vec;


uniform vec3 AmbientProduct, DiffuseProduct, SpecularProduct;
uniform mat4 ModelView;
uniform mat4 Projection;
uniform vec4 LightPosition;
uniform vec4 Light2Position;
uniform float Shininess;

void main()
{

    vpos = vec4(vPosition, 1.0);

    // Transform vertex position into eye coordinates
    pos = (ModelView * vpos).xyz;

    // The vector to the light from the vertex
    Lvec = LightPosition.xyz - pos;
    L2vec = (Light2Position.xyz);

    // Transform vertex normal into eye coordinates (assumes scaling
    // is uniform across dimensions)
    N = normalize( (ModelView*vec4(vNormal, 0.0)).xyz );

    gl_Position = Projection * ModelView * vpos;
    texCoord = vTexCoord;
}
