/**
 * CITS3003 Project
 * Tahmer Hijjawi 21130321
 * Jack Scott	21504053
 * 2016 Sem 1
 */

varying vec2 texCoord;  // The third coordinate is always 0.0 and is discarded
varying vec4 fPosition;
varying vec4 fNormal;
varying mat4 boneTransform;

uniform sampler2D texture;
uniform vec3 AmbientProduct, DiffuseProduct, SpecularProduct;
uniform mat4 ModelView;
uniform mat4 Projection;
uniform vec4 LightPosition1;
uniform vec4 LightPosition2;
uniform float Shininess;
uniform float TexScale;

void main()
{
    /////////////
    // Light 1 //
    /////////////

    vec3 position = (ModelView * fPosition).xyz;
    vec3 normal = normalize(ModelView * fNormal).xyz;

    // The vector to the light from the vertex
    vec3 Lvec1 = LightPosition1.xyz - position;

    // Part 1 - G: lighting calculations
    float lightToObject = length(Lvec1);
    float distance = 1.0 / (1.0 + lightToObject + lightToObject * lightToObject); // Formula: 1/(a+bd+cd^2)

    // Unit direction vectors for Blinn-Phong shading calculation
    vec3 L1 = normalize( Lvec1 );   // Direction to the light source
    vec3 E1 = normalize( -position );   // Direction to the eye/camera
    vec3 H1 = normalize( L1 + E1 );  // Halfway vector

    float Kd1 = max( dot(L1, normal), 0.0 );
    vec3 diffuse1 = Kd1 * DiffuseProduct;

    float Ks1 = pow( max(dot(normal, H1), 0.0), Shininess );
    vec3 specular1 = vec3(0.1, 0.1, 0.1) * Ks1 * SpecularProduct;

    if (dot(L1, normal) < 0.0 ) {
        specular1 = vec3(0.0, 0.0, 0.0);
    }

    vec3 Light1 = (diffuse1 + distance) + (specular1 + distance);

    /////////////
    // Light 2 //
    /////////////

    // The vector to the light from the vertex
    vec3 Lvec2 = LightPosition2.xyz - position;

    // Unit direction vectors for Blinn-Phong shading calculation
    vec3 L2 = normalize( Lvec2 );   // Direction to the light source
    vec3 E2 = normalize( -position );   // Direction to the eye/camera
    vec3 H2 = normalize( L2 + E2 );  // Halfway vector

    float Kd2 = max( dot(L2, normal), 0.0 );
    vec3 diffuse2 = Kd2 * DiffuseProduct;

    float Ks2 = pow( max(dot(normal, H2), 0.0), Shininess );
    vec3 specular2 = vec3(0.1, 0.1, 0.1) * Ks2 * SpecularProduct;

    if (dot(L2, normal) < 0.0 ) {
        specular2 = vec3(0.0, 0.0, 0.0);
    }

    vec3 Light2 = diffuse2 * specular2;

    //////////////////
    // Illumination //
    //////////////////

    // globalAmbient is independent of distance from the light source
    vec3 globalAmbient = vec3(0.1, 0.1, 0.1);

    vec3 colormod = globalAmbient + AmbientProduct + Light1 + Light2;

    gl_FragColor = texture2D( texture, TexScale * texCoord * 2.0 ) * vec4(colormod, 1.0); // 1.0 is the opacity
}
