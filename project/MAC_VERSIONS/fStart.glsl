/**
 * CITS3003 Project
 * Tahmer Hijjawi 21130321
 * Jack Scott	21504053
 * 2016 Sem 1
 */

varying vec2 texCoord;  // The third coordinate is always 0.0 and is discarded
varying vec3 pos;
varying vec3 N;

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

    // The vector to the light from the vertex
    vec3 Lvec1 = LightPosition1.xyz - pos;

    // Part 1 - G: lighting calculations
    float lightToObject = length(Lvec1);
    float distance = 1.0 / (1.0 + lightToObject + lightToObject * lightToObject); // Formula: 1/(a+bd+cd^2)

    // Unit direction vectors for Blinn-Phong shading calculation
    vec3 L1 = normalize( Lvec1 );   // Direction to the light source
    vec3 E1 = normalize( -pos );   // Direction to the eye/camera
    vec3 H1 = normalize( L1 + E1 );  // Halfway vector

    float Kd1 = max( dot(L1, N), 0.0 );
    vec3 diffuse1 = Kd1 * DiffuseProduct + distance;

    float Ks1 = pow( max(dot(N, H1), 0.0), Shininess );
    vec3 specular1 = vec3(0.1, 0.1, 0.1) * Ks1 * SpecularProduct + distance;

    if (dot(L1, N) < 0.0 ) {
        specular1 = vec3(0.0, 0.0, 0.0);
    }

    /////////////
    // Light 2 //
    /////////////

    // The vector to the light from the vertex
    vec3 Lvec2 = LightPosition2.xyz - pos;

    // Unit direction vectors for Blinn-Phong shading calculation
    vec3 L2 = normalize( Lvec2 );   // Direction to the light source
    vec3 E2 = normalize( -pos );   // Direction to the eye/camera
    vec3 H2 = normalize( L2 + E2 );  // Halfway vector

    float Kd2 = max( dot(L2, N), 0.0 );
    vec3 diffuse2 = Kd2 * DiffuseProduct;

    float Ks2 = pow( max(dot(N, H2), 0.0), Shininess );
    vec3 specular2 = vec3(0.1, 0.1, 0.1) * Ks2 * SpecularProduct;

    if (dot(L2, N) < 0.0 ) {
        specular2 = vec3(0.0, 0.0, 0.0);
    }

    //////////////////
    // Illumination //
    //////////////////

    // globalAmbient is independent of distance from the light source
    vec3 globalAmbient = vec3(0.1, 0.1, 0.1);

    // Compute ambience in the illumination equation
    vec3 ambient = AmbientProduct;

    vec3 colormod = globalAmbient + ambient + diffuse1 + diffuse2 + specular1 + specular2;

    gl_FragColor = texture2D( texture, TexScale * texCoord * 2.0 ) * vec4(colormod, 1.0); // 1.0 is the opacity
}
