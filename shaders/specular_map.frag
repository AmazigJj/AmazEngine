//glsl version 4.5
#version 450

struct LightData{
	vec3 lightPos;
	vec3 lightColor;
};

//shader input
layout (location = 0) in vec3 inColor;
layout (location = 1) in vec2 texCoord;
layout (location = 2) in vec3 Normal;
layout (location = 3) in vec3 FragPos;
layout (location = 4) in vec4 FragPosLightSpace;
//output write
layout (location = 0) out vec4 outFragColor;

layout(set = 0, binding = 1) uniform  SceneData{
	vec4 fogColor; // w is for exponent
	vec4 fogDistances; //x for min, y for max, zw unused.
	vec4 ambientColor;
	vec3 sunlightDirection; //w for sun power
	vec3 sunlightColor;
	vec3 camPos;
	mat4 lightSpaceMatrix;
} sceneData;

//all light matrices
layout(std140,set = 0, binding = 2) readonly buffer LightBuffer{
	LightData lights[];
} lightBuffer;

layout(set = 2, binding = 0) uniform sampler2D tex1;

layout(set = 3, binding = 0) uniform sampler2D specMap;


void main()
{

	
	LightData light = lightBuffer.lights[0];

	float lightConstant = 1.0f;
	float lightLinear = 0.022f;
	float lightQuadratic = 0.0019f;
	
	float distance = length(light.lightPos - FragPos);
	float attenuation = 1.0 / (lightConstant + lightLinear * distance + 
    		    lightQuadratic * (distance * distance));    

	float ambientStrength = 0.1;
	vec3 ambient = ambientStrength * light.lightColor * texture(tex1,texCoord).xyz;

	vec3 norm = normalize(Normal);
	vec3 lightDir = normalize(light.lightPos - FragPos);
	float diff = max(dot(norm, lightDir), 0.0);
	vec3 diffuse = diff * light.lightColor * texture(tex1,texCoord).xyz;

	vec3 viewDir = normalize(sceneData.camPos - FragPos);
	vec3 reflectDir = reflect(-lightDir, norm);
	float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
	vec3 specular = spec * light.lightColor * texture(specMap,texCoord).xyz;

	ambient  *= attenuation; 
	diffuse  *= attenuation;
	specular *= attenuation;   

	vec3 color = (ambient + diffuse + specular);
	outFragColor = vec4(color,1.0f);
}
