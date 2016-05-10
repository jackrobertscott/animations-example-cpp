varying vec2 texCoord;  // The third coordinate is always 0.0 and is discarded
varying vec4 vpos;
varying vec3 pos;
varying vec3 N;
varying vec3 Lvec;

uniform sampler2D texture;
uniform vec3 AmbientProduct, DiffuseProduct, SpecularProduct;
uniform mat4 ModelView;
uniform mat4 Projection;
uniform vec4 LightPosition;
uniform float Shininess;

void main()
{

    float dist = length(Lvec);
    dist = 1.0 / (1.0 + dist + dist*dist); //1/(a+bd+cd^2)

    // Unit direction vectors for Blinn-Phong shading calculation
    vec3 L = normalize( Lvec );   // Direction to the light source
    vec3 E = normalize( -pos );   // Direction to the eye/camera
    vec3 H = normalize( L + E );  // Halfway vector

    // Compute terms in the illumination equation
    vec3 ambient = AmbientProduct;

    float Kd = max( dot(L, N), 0.0 );
    vec3  diffuse = Kd*DiffuseProduct + dist;

    float Ks = pow( max(dot(N, H), 0.0), Shininess );
    vec3  specular = Ks * SpecularProduct + dist;

    if (dot(L, N) < 0.0 ) {
    specular = vec3(0.0, 0.0, 0.0);
    }

    // globalAmbient is independent of distance from the light source
    vec3 globalAmbient = vec3(0.1, 0.1, 0.1);
    vec3 colormod = globalAmbient + ambient + diffuse + specular;


    gl_FragColor = texture2D( texture, texCoord * 2.0 ) * vec4(colormod, 1.0);
}
