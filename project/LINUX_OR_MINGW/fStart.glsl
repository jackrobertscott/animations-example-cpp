varying vec2 texCoord;  // The third coordinate is always 0.0 and is discarded
varying vec4 vpos;
varying vec3 pos;
varying vec3 N;
varying vec3 Lvec;
varying vec3 L2vec;

uniform sampler2D texture;
uniform vec3 AmbientProduct, DiffuseProduct, SpecularProduct;
uniform mat4 ModelView;
uniform mat4 Projection;
uniform vec4 LightPosition;
uniform vec4 Light2Position;
uniform float Shininess;

void main()
{

    float dist = length(Lvec);
    dist = 1.0 / (1.0 + dist + dist*dist); //1/(a+bd+cd^2)

    // Unit direction vectors for Blinn-Phong shading calculation
    vec3 L2 = normalize(L2vec);   // Direction to second light source from the origin
    vec3 L = normalize( Lvec );   // Direction to the light source
    vec3 E = normalize( -pos );   // Direction to the eye/camera
    vec3 H = normalize( L + E );  // Halfway vector
    vec3 H2 = normalize( L2 + E); //Halfway vector light 2

    // Compute terms in the illumination equation
    vec3 ambient = AmbientProduct;

    float Kd = max( dot(L, N), 0.0 );
    float Kd2 = max( dot(L2, N), 0.0);
    vec3  diffuse = Kd*DiffuseProduct + dist;
    vec3  diffuse2 = Kd2*DiffuseProduct;

    float Ks = pow( max(dot(N, H), 0.0), Shininess );
    float Ks2 = 2.0 * pow( max(dot(N, H2), 0.0), Shininess );

    vec3 materialSpecularColor = vec3(1.0, 1.0, 1.0);
    vec3  specular = materialSpecularColor * Ks * SpecularProduct + dist;
    vec3  specular2 =2.0 * Ks2 * SpecularProduct;

    if (dot(L, N) < 0.0 ) {
    specular = vec3(0.0, 0.0, 0.0);
    }

    // globalAmbient is independent of distance from the light source
    vec3 globalAmbient = vec3(0.1, 0.1, 0.1);
    vec3 colormod = globalAmbient + ambient + diffuse + specular + diffuse2 + specular2;


    gl_FragColor = texture2D( texture, texCoord * 2.0 ) * vec4(colormod, 1.0);
}
