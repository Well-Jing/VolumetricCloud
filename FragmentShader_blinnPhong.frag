#version 330 core

struct Material{
	vec3 ambient;
	sampler2D diffuse;
	sampler2D specular;
	sampler2D emission;
	float shininess;
};

struct LightDirectional{
	vec3 pos;
	vec3 color;
	vec3 dirToLight;
};

struct LightPoint{
	vec3 pos;
	vec3 color;
	vec3 dirToLight;
	float constant;
	float linear;
	float quadratic;
};

struct LightSpot{
vec3 pos;
	vec3 color;
	vec3 dirToLight;
	//float constant;
	//float linear;
	//float quadratic;
	float cosInner;
	float cosOuter;
};

in vec3 fragPos;
in vec4 fragPosLightSpace;
in vec3 normal;
in vec2 texCoord;

uniform Material material;
uniform LightDirectional lightD;
uniform LightDirectional lightD1;
uniform LightPoint lightP0;
uniform LightPoint lightP1;
uniform LightPoint lightP2;
uniform LightPoint lightP3;
uniform LightSpot lightS;
uniform vec3 objectColor;
uniform vec3 ambientColor;
uniform vec3 lightColor;
uniform vec3 lightPos;
uniform vec3 lightDirUniform;
uniform vec3 viewPos;
uniform sampler2D shadowMap;

out vec4 FragColor;

vec3 CalcLightDirectional(LightDirectional light, vec3 uNormal, vec3 dirToCamera)
{
	// diffuse
	float diffIntensity = max(dot(uNormal, light.dirToLight), 0);
	vec3 diffColor = 0.6 * diffIntensity * light.color * texture(material.diffuse, texCoord).rgb;
	
	// specular
	vec3 H = normalize(light.dirToLight + dirToCamera); 
	float specIntensity = pow(max(dot(H, uNormal), 0), 16);
	vec3 specColor = 0.1 * specIntensity * light.color * texture(material.specular, texCoord).rgb;

	// ambinent
	vec3 ambientColor = 0.2 * texture(material.specular, texCoord).rgb;

	vec3 result = diffColor + specColor + ambientColor;

	// float shadow = ShadowCalculation(fragPosLightSpace);  
	// vec3 result = (1 - shadow) * (diffColor + specColor) + ambientColor;
	
	return result;
};

vec3 CalcLightPoint(LightPoint light, vec3 uNormal, vec3 dirToCamera, vec3 dirToLight)
{
	// attenuation
	float dist = length(light.pos - fragPos);
	float attenuation = 1 / (light.constant + light.linear * dist + light.quadratic * pow(dist, 2));

	// diffuse
	float diffIntensity = max(dot(uNormal, dirToLight), 0);
	vec3 diffColor = diffIntensity * light.color * texture(material.diffuse, texCoord).rgb;

	// specular
	vec3 H = normalize(dirToLight + dirToCamera); 
	float specIntensity = pow(max(dot(H, uNormal), 0), 16);
	vec3 specColor = specIntensity * light.color * texture(material.specular, texCoord).rgb;

	vec3 ambientColor = 0.6 * texture(material.diffuse, texCoord).rgb;

	vec3 result = (diffColor + specColor) * attenuation + ambientColor;
	return result;
};


void main() 
{
	vec3 finalResult = vec3(0, 0, 0);
	vec3 uNormal = normalize(normal);
	vec3 dirToCamera = normalize(viewPos - fragPos);
	vec3 dirFragToLightP0 = normalize(lightD.pos - fragPos); 

	finalResult += CalcLightDirectional(lightD, uNormal, dirToCamera);
	//finalResult += CalcLightPoint(lightD, uNormal, dirToCamera, dirFragToLightP0);

	FragColor = vec4(finalResult, 1.0);

}

