//glsl version 4.5
#version 450

struct ShadowMapData {
	float shadowMapX;		// 1 = 1 tile, 1024 pixels
	float shadowMapY;		// 1 = 1 tile, 1024 pixels
	float shadowMapSize;	// 1 = 1024 x 1024 pixels
};

struct PointLight {
	vec3 lightPos;
	mat4 lightSpaceMatrix[6];

	vec3 lightColor;
	vec3 ambientColor;

	float constant;
    float linear;
    float quadratic;
	float farPlane;

	ShadowMapData shadowMapData[6];
};

struct DirLight {
	vec3 lightPos;
	vec3 lightDir;
	mat4 lightSpaceMatrix;
	
	vec3 lightColor;
	vec3 ambientColor;

	ShadowMapData shadowMapData;
};

struct SpotLight {
	vec3 lightPos;
	vec3 lightDir;
	mat4 lightSpaceMatrix;

	vec3 lightColor;
	vec3 ambientColor;

	float radius;
	float constant;
    float linear;
    float quadratic;

	ShadowMapData shadowMapData;
};

//shader input
layout (location = 0) in vec3 inColor;
layout (location = 1) in vec2 texCoord;
layout (location = 2) in vec3 Normal;
layout (location = 3) in vec3 FragPos;
layout (location = 4) in vec4 FragPosLightSpace;
//output write
layout (location = 0) out vec4 outFragColor;

layout(set = 0, binding = 1) uniform SceneData{
    vec4 fogColor; // w is for exponent
	vec4 fogDistances; //x for min, y for max, zw unused.
	vec3 ambientColor;
	vec3 sunlightDirection; //w for sun power
	vec3 sunlightColor;
	vec3 camPos;
	mat4 lightSpaceMatrix;
	int shadowSamples;
	float shadowStride;
} sceneData;

layout(set = 0, binding = 2) uniform sampler2D shadowMap;

//all directional lights
layout(std140,set = 1, binding = 1) readonly buffer DirLightBuffer {
	int count;
	DirLight lights[];
} dirLightBuffer;

//all point lights
layout(std140,set = 1, binding = 2) readonly buffer PointLightBuffer {
	int count;
	PointLight lights[];
} pointLightBuffer;

//all spot lights
layout(std140,set = 1, binding = 3) readonly buffer SpotLightBuffer {
	int count;
	SpotLight lights[];
} spotLightBuffer;

layout(set = 2, binding = 0) uniform sampler2D tex1;

float sampleShadow(const ShadowMapData shadowMapData, const vec2 pos);
float sampleCubeShadow(const PointLight light, const vec3 v);
vec2 sampleCube(const vec3 v, out int faceIndex);
vec3 calcDirLight(const DirLight dirLight, const vec3 color, const vec3 fragPos, const vec3 normal, const vec3 viewDir);
float dirShadowCalculation(const DirLight dirLight, const vec3 fragPos, const vec3 normal);
vec3 calcPointLight(const PointLight light, const vec3 color, const vec3 fragPos, const vec3 normal, const vec3 viewDir);
float pointShadowCalculation(const PointLight light, const vec3 fragPos, const vec3 normal);

void main() {

	vec3 norm = normalize(Normal);
	vec3 viewDir = normalize(sceneData.camPos - FragPos);

	vec3 color = vec3(0);

	// srgb texture -> linear space
	float gamma = 2.2;
	vec3 diffuseColor = pow(texture(tex1, texCoord).rgb, vec3(gamma));

	// vec3 diffuseColor = vec3(1.0, 1.0, 1.0);

	for(int i = 0; i < dirLightBuffer.count; i++)
		color += calcDirLight(dirLightBuffer.lights[i], diffuseColor, FragPos, norm, viewDir);

	for(int i = 0; i < pointLightBuffer.count; i++)
		color += calcPointLight(pointLightBuffer.lights[i], diffuseColor, FragPos, norm, viewDir);
	
	//TODO: Spotlights

    outFragColor = vec4(color,1.0f);

}

vec3 calcDirLight(const DirLight dirLight, const vec3 color, const vec3 fragPos, const vec3 normal, const vec3 viewDir) {
    vec3 newLightDir = normalize(dirLight.lightDir);
    vec3 halfwayDir = normalize(newLightDir + viewDir);
    // diffuse shading
    float diff = max(dot(normal, newLightDir), 0.0);
    // specular shading
    vec3 reflectDir = reflect(-newLightDir, normal);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), 32); //TODO: change 32 to material shininess

	// combine results
    vec3 ambient  = dirLight.ambientColor * color;
    vec3 diffuse  = dirLight.lightColor  * diff  * color;
    vec3 specular = dirLight.lightColor * spec * 0;
    //return (ambient + diffuse + specular);

	float shadow = dirShadowCalculation(dirLight, fragPos, normal);

    return (ambient + (1.0 - shadow) * (diffuse + specular));
}

float dirShadowCalculation(const DirLight dirLight, const vec3 fragPos, const vec3 normal) {

	ShadowMapData shadowMapData = dirLight.shadowMapData;

	vec4 fragPosLightSpace = dirLight.lightSpaceMatrix * vec4(fragPos, 0.0);

	// vec4 fragPosLightSpace = FragPosLightSpace;

    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;  // (w is always 1 for ortho projection)
    projCoords = projCoords * 0.5 + 0.5;                            // map [-1, 1] to [0, 1]
    
    if(projCoords.z > 1.0 /*|| (projCoords.x < 0.0 || projCoords.x >= 1.0) || (projCoords.y < 0.0 || projCoords.y >= 1.0)*/)
        return 0.0;

    float currentDepth = fragPosLightSpace.z;

    //float bias = max(0.05 * (1.0 - dot(normal, lightDir)), 0.005);  

    float shadow = 0.0;
    float texelSize = 1.0 / (1024 * shadowMapData.shadowMapSize);

	float samples = sceneData.shadowSamples;
	float stride = sceneData.shadowStride;

	float maxCoord = stride * (samples - 1.0) / 2.0;

	float x, y;
	float pcfDepth;
	for (y = -maxCoord; y <= maxCoord; y += stride) {
		for (x = -maxCoord; x <= maxCoord; x += stride) {
			//pcfDepth = sampleShadow(shadowMapData, projCoords.xy + (vec2(x, y) * texelSize));
			pcfDepth = texture(shadowMap, (projCoords.xy + (vec2(x, y) * texelSize))/8).r;
			shadow += currentDepth > pcfDepth ? 1.0 : 0.0; 
		}
	}
	shadow /= (samples * samples);

    return shadow;
}

vec3 calcPointLight(const PointLight light, const vec3 color, const vec3 fragPos, const vec3 normal, const vec3 viewDir) {
	vec3 lightDir = normalize(light.lightPos - fragPos);
    vec3 halfwayDir = normalize(lightDir + viewDir);

    // diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);
    // specular shading
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), 32); //TODO: change 32 to material shininess

    // combine results
    vec3 ambient  = light.ambientColor * color;
    vec3 diffuse  = light.lightColor  * diff * color;
    vec3 specular = light.lightColor * spec * 0;

	// attenuation
    float distance    = length(light.lightPos - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));

    ambient  *= attenuation;
    diffuse  *= attenuation;
    specular *= attenuation;

	float shadow = pointShadowCalculation(light, fragPos, normal);

	//float shadow = 0.0;

    return (ambient + (1.0 - shadow) * (diffuse + specular));
}

float pointShadowCalculation(const PointLight light, const vec3 fragPos, const vec3 normal) {
	vec3 fragToLight = fragPos - light.lightPos;
	
	float closestDepth = sampleCubeShadow(light, normalize(fragToLight));
	closestDepth *= 25;

	float currentDepth = length(fragToLight);

	float shadow = currentDepth - 0.1 > closestDepth ? 1.0 : 0.0;

	// float shadow  = 0.0;
	// float bias    = 0.1; 
	// float samples = 4.0;
	// float offset  = 0.1;
	// for(float x = -offset; x < offset; x += offset / (samples * 0.5))
	// {
	// 	for(float y = -offset; y < offset; y += offset / (samples * 0.5))
	// 	{
	// 		for(float z = -offset; z < offset; z += offset / (samples * 0.5))
	// 		{
	// 			float closestDepth = sampleCubeShadow(light, fragToLight + vec3(x, y, z));
	// 			closestDepth *= 25;   // undo mapping [0;1]
	// 			if(currentDepth - bias > closestDepth)
	// 				shadow += 1.0;
	// 		}
	// 	}
	// }
	// shadow /= (samples * samples * samples);

	return shadow;

}

float sampleShadow(const ShadowMapData shadowMapData, const vec2 pos) {

	if ((pos.x >= 1.0 || pos.x < 0.0) || (pos.y >= 1.0 || pos.y < 0.0)) {
		return 0.0;
	}
	vec2 mapCoords = ((pos * shadowMapData.shadowMapSize)																// Convert [0, 1] to [0, mapSize]
					 + vec2(shadowMapData.shadowMapX, shadowMapData.shadowMapY))								// Move to tile within the shadow atlas
					 / 8; 																// Convert [0, 8] to [0, 1]

	return texture(shadowMap, mapCoords).r;
}

float sampleShadow(const int face, const vec2 pos) {

	if ((pos.x >= 1.0 || pos.x < 0.0) || (pos.y >= 1.0 || pos.y < 0.0)) {
		return 1.0;
	}
	vec2 mapCoords = ((pos)																// Convert [0, 1] to [0, mapSize]
					 + vec2(face + 1, 0))												// Move to tile within the shadow atlas
					 / 8; 																// Convert [0, 8] to [0, 1]
	
	//mapCoords = (pos + vec2(2, 0)) / 8;

	//mapCoords = pos / 8;

	return texture(shadowMap, mapCoords).r;
}
float sampleCubeShadow(const PointLight light, const vec3 v) {
	int face;
	vec2 coords = sampleCube(v, face);

	ShadowMapData shadowMapData = light.shadowMapData[face];

	return sampleShadow(shadowMapData, coords);

	// vec2 mapCoords = ((coords * shadowMapData.shadowMapSize)							// Convert [0, 1] to [0, mapSize]
	// 				 + vec2(shadowMapData.shadowMapX, shadowMapData.shadowMapY))		// Move to tile within the shadow atlas
	// 				 / 8; 																// Convert [0, 8] to [0, 1]
	// return 1.0;
}

// yoinked from https://www.gamedev.net/forums/topic/687535-implementing-a-cube-map-lookup-function/5337482/
vec2 sampleCube(const vec3 v, out int faceIndex) {
	vec3 vAbs = abs(v);
	float ma;
	vec2 uv;
	if(vAbs.z >= vAbs.x && vAbs.z >= vAbs.y)
	{
		faceIndex = v.z < 0.0 ? 5 : 4;			// pos z = 4, neg z = 5
		ma = 0.5 / vAbs.z;
		uv = vec2(v.z < 0.0 ? -v.x : v.x, -v.y);
	}
	else if(vAbs.y >= vAbs.x)
	{
		faceIndex = v.y < 0.0 ? 3 : 2;			// pos y = 2, neg y = 3
		ma = 0.5 / vAbs.y;
		uv = vec2(v.x, v.y < 0.0 ? -v.z : v.z);
	}
	else
	{
		faceIndex = v.x < 0.0 ? 1 : 0;			// pos x = 0, neg x = 1
		ma = 0.5 / vAbs.x;
		uv = vec2(v.x < 0.0 ? v.z : -v.z, -v.y);
	}
	return uv * ma + 0.5;
}

// old stuff

// vec3 calcPointLight(LightData light, vec3 normal, vec3 fragPos, vec3 viewDir, float shadow) {
// 	vec3 lightDir = normalize(light.lightPos - fragPos);

//     // diffuse shading
//     float diff = max(dot(normal, lightDir), 0.0);
//     // specular shading
//     vec3 reflectDir = reflect(-lightDir, normal);
//     float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32); //TODO: change 32 to material shininess
//     // attenuation
//     float distance    = length(light.lightPos - fragPos);
//     float attenuation = 1.0 / (light.constant + light.linear * distance + 
//   			     light.quadratic * (distance * distance));    
//     // combine results
//     float ambientStrength = 0.1f;
//     vec3 ambient  = light.lightColor  * ambientStrength * vec3(texture(tex1, texCoord));
//     vec3 diffuse  = light.lightColor  * diff * vec3(texture(tex1, texCoord));
//     vec3 specular = light.lightColor * spec;
//     ambient  *= attenuation;
//     diffuse  *= attenuation;
//     specular *= attenuation;
//     return (ambient + (1.0 - shadow) * (diffuse + specular));
// }

// float ShadowCalculation(vec4 fragPosLightSpace, vec3 lightDir, vec3 normal)
// {

//     vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;  // (w is always 1 for ortho projection)
//     projCoords = projCoords * 0.5 + 0.5;                            // convert [-1, 1] to [0, 1]
    
//     if(projCoords.z > 1.0)
//         return 0.0;

//     float closestDepth = texture(shadowMap, projCoords.xy).r;
//     float currentDepth = fragPosLightSpace.z;

//     float bias = max(0.05 * (1.0 - dot(normal, lightDir)), 0.005);  
//     //float shadow = currentDepth - 0.005 > closestDepth ? 1.0 : 0.0;


//     float shadow = 0.0;
//     vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
//     for(int x = -1; x <= 1; x++) {
//         for(int y = -1; y <= 1; y++) {
//             float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r; 
//             shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;        
//         }    
//     }
//     shadow /= 9.0;

// //    shadow = currentDepth - bias > closestDepth ? 1.0 : 0.0;

//     return shadow;



// }

// float ShadowTest(vec4 fragPosLightSpace)
// {
//     vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;  // (w is always 1 for ortho projection)
//     projCoords = projCoords * 0.5 + 0.5;                          // convert [-1, 1] to [0, 1] 
    
//     float currentDepth = fragPosLightSpace.z;
//     float closestDepth = texture(shadowMap, projCoords.xy).r;

//     return currentDepth;

// }