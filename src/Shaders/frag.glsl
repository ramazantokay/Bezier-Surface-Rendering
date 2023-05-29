#version 460 core

// All of the following variables could be defined in the OpenGL
// program and passed to this shader as uniform variables. This
// would be necessary if their values could change during runtim.
// However, we will not change them and therefore we define them 
// here for simplicity.

vec3 I = vec3(1, 1, 1);          // point light intensity
vec3 Iamb = vec3(0.8, 0.8, 0.8); // ambient light intensity
vec3 ka = vec3(0.3, 0.3, 0.3);   // ambient reflectance coefficient
vec3 kd = vec3(0.8, 0.8, 0.8);     // diffuse reflectance coefficient
vec3 ks = vec3(0.8, 0.8, 0.8);   // specular reflectance coefficient
//vec3 lightPos = vec3(-1, 2, 1);   // light position in world coordinates
//vec3 lightInt = vec3(5.0, 0.1, 0.1);   // light position in world coordinates

uniform vec3 eyePos;
uniform int numberOfPointLights;
//uniform vec3 lightIntesity; // added light intensity

struct Light
{
	vec3 lightPosition;
	vec3 lightColor;
};

uniform Light lights[5];

in vec4 fragWorldPos;
in vec3 fragWorldNor;

out vec4 fragColor;

//vec3 calcLight(Light l, vec3 normal, vec3 fragPos, vec3 view)
//{
//	vec3 L = normalize(l.lightPosition-fragPos);
//
//	float NdotL = max(dot(normal, L),0.0); // diffuse
//	vec3 reflectDir = reflect(-L,normal); // specular
//
//	return ;
//}
//

void main(void)
{
	// Compute lighting. We assume lightPos and eyePos are in world
	// coordinates. fragWorldPos and fragWorldNor are the interpolated
	// coordinates by the rasterizer.
	vec3 result;
	vec3 ambientColor = Iamb * ka;
	result+=ambientColor;
	
	for(int i=0; i<numberOfPointLights; i++)
	{
		float dist = length(lights[i].lightPosition - vec3(fragWorldPos));

		vec3 L = normalize(lights[i].lightPosition - vec3(fragWorldPos)); // light direction wi
		vec3 V = normalize(eyePos - vec3(fragWorldPos));
		vec3 H = normalize(L + V);
		vec3 N = normalize(fragWorldNor);

//		vec3 reflecDir = reflect(-L, N);

		float NdotL = dot(N, L); // for diffuse component
		float NdotH = dot(N, H); // for specular component

		vec3 diffuseColor = lights[i].lightColor * kd * max(0, NdotL);
		vec3 specularColor = lights[i].lightColor * ks * pow(max(0, NdotH), 400);

		result += vec3(diffuseColor + specularColor)/pow(dist,2);
		
		}

//	vec3 L = normalize(lights[0].lightPosition - vec3(fragWorldPos)); // light direction wi
////	vec3 L2 = normalize(lights[1].lightPosition - vec3(fragWorldPos)); // light direction wi
//	vec3 V = normalize(eyePos - vec3(fragWorldPos));
//	vec3 H = normalize(L + V);
//	vec3 N = normalize(fragWorldNor);
//
//	float NdotL = dot(N, L); // for diffuse component
//	float NdotH = dot(N, H); // for specular component
//
//	vec3 diffuseColor = lights[0].lightColor * kd * max(0, NdotL);
//	vec3 specularColor = lights[0].lightColor* ks * pow(max(0, NdotH), 400);
//	vec3 ambientColor = Iamb * ka;
//
//	fragColor = vec4(diffuseColor + specularColor + ambientColor, 1.0);
	fragColor = vec4(clamp(result,0.0,1.0),1.0);
//	fragColor = vec4(N, 1.0); // normal color for debug purpose

//		fragColor = vec4(0.2f, 0.3f, 0.7f, 1.0f);

}
