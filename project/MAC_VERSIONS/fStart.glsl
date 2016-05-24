/**
 * CITS3003 Project
 * Tahmer Hijjawi 21130321
 * Jack Scott	21504053
 * 2016 Sem 1
 */
 #version 120

varying vec2 texCoord;  // The third coordinate is always 0.0 and is discarded
varying vec3 fPosition, fNormal;
varying mat4 boneTransform;

uniform sampler2D texture;
uniform vec3 AmbientProduct, DiffuseProduct, SpecularProduct;
uniform mat4 ModelView;
uniform vec4 LightPosition1, LightPosition2;

uniform float Shininess;
uniform float TexScale;

//structure to sort the lights better
struct LightStruct {
  vec3 intensity;
  vec4 direction;
  vec3 position;
  float attenuation;

  vec3 ambient;
  vec3 diffuse;
  vec3 specular;
};

LightStruct light1;
LightStruct light2;

void main()
{

    vec3 materialSpecularColor = vec3(1.0, 1.0, 1.0);
    vec4 fpos = vec4(fPosition, 1.0);

    // Transform vertex position into eye coordinates
    vec3 pos = (ModelView * fpos).xyz;

    // Transform vertex normal into eye coordinates (assumes scaling
    // is uniform across dimensions)
    vec3 N = normalize(ModelView * vec4(fNormal, 0.0)).xyz;

    /////////////
    // Light 1 //
    /////////////

    vec3 L1 = normalize(LightPosition1.xyz - pos); // Direction to the light source
    vec3 E1 = normalize(-pos);                     // Direction to the eye/camera
    vec3 H1 = normalize(L1 + E1);                  // Halfway vector

    float Kdif1 = max(dot(N, L1), 0.0);                 //diffuse term
    float Kspc1 = pow(max(dot(N, H1), 0.0), Shininess); //specular term

    //attenuation calculations.
    float lightToObject = length(LightPosition1.xyz - pos);
    light1.attenuation = 1.0 / (1.0 + lightToObject + (lightToObject * lightToObject)); // Formula: 1/(a+bd+cd^2)

    //light properties
    light1.ambient = light1.attenuation * (AmbientProduct);
    light1.diffuse = light1.attenuation * (DiffuseProduct * Kdif1);
    light1.specular = light1.attenuation * (SpecularProduct * Kspc1);

    light1.intensity = (light1.ambient + light1.diffuse + light1.specular);

    /////////////
    // Light 2 //
    /////////////

    //clamping it improved the light.
    float dirx = clamp(-LightPosition2.x, -3.0, 3.0);
    float diry = clamp(-LightPosition2.y, -3.0, 3.0);
    float dirz = clamp(-LightPosition2.z, -3.0, 3.0);

    vec4 LightDirection = vec4(dirx, diry, dirz, 0.0);

    light2.direction = LightDirection;
    vec3 L2 = normalize(-light2.direction).xyz; //to origin
    vec3 E2 = normalize(-pos);  //to Camera
    vec3 H2 = normalize(L2 + E2); //halfway vector

    //diffuse and specular terms
    float Kdif2 = max(dot(N, L2), 0.0);
    float Kspc2 = pow(max(dot(N, H2), 0.0), Shininess);

    //light 2 properties
    light2.ambient = AmbientProduct;
    light2.diffuse = DiffuseProduct * Kdif2;
    light2.specular = SpecularProduct * Kspc2 * materialSpecularColor;

    light2.intensity = (light2.ambient + light2.diffuse + light2.specular);

    //////////////////
    // Illumination //
    //////////////////

    // globalAmbient is independent of distance from the light source
    vec3 globalAmbient = vec3(0.2, 0.2, 0.2);

    //the total light contribution.
    vec3 colormod = globalAmbient + (light1.intensity * 5.0) + (light2.intensity * 1.5);

    gl_FragColor = texture2D( texture, TexScale * texCoord * 2.0 ) * vec4(colormod, 1.0); // 1.0 is the opacity
}
