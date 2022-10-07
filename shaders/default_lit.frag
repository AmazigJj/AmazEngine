//glsl version 4.5
#version 450

layout(set = 0, binding = 0) uniform CameraBuffer{
	mat4 view;
	mat4 proj;
	mat4 viewproj;
} camData;

struct ShadowMapData {
	float shadowMapX;		// 1 = 1 tile, 1024 pixels
	float shadowMapY;		// 1 = 1 tile, 1024 pixels
	float shadowMapSize;	// 1 = 1024 x 1024 pixels
};

struct PointLight {
	vec3 lightPos;
	mat4 lightSpaceMatrix[6]; // do i need this?

	vec3 lightColor;
	vec3 ambientColor;

	float radius;
	float farPlane;

	ShadowMapData shadowMapData[6]; // move to using int index
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

layout (location = 0) in vec3 inColor;
layout (location = 1) in vec2 texCoord;
layout (location = 2) in vec3 Normal;
layout (location = 3) in vec3 FragPos;
layout (location = 4) in vec4 viewPos;
layout (location = 5) in vec4 viewprojPos;

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
	float zFar;
	float zNear;
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

struct ActiveLight {
	uint type;
	uint index;
};

layout(std140,set = 1, binding = 4) buffer activeLightBuffer {
	uint count;
	ActiveLight lights[];
} activeLights;

struct Cluster {   // A cluster volume is represented using an AABB
   vec3 minPoint;
   vec3 maxPoint;
   uint index;
   uint count;
};

layout(std140,set = 1, binding = 5) buffer clusterLightBuffer {
	Cluster clusters[];
};

layout(std140,set = 1, binding = 6) buffer lightIndexBuffer {
	uint count;
	uint indices[];
} lightIndices;

float sampleShadow(const ShadowMapData shadowMapData, const vec2 pos);
float sampleCubeShadow(const PointLight light, const vec3 v);
vec2 sampleCube(const vec3 v, out int faceIndex);
vec3 calcDirLight(const DirLight dirLight, const vec3 fragPos, const vec3 normal, const vec3 viewDir);
float dirShadowCalculation(const DirLight dirLight, const vec3 fragPos, const vec3 normal);
vec3 calcPointLight(const PointLight light, const vec3 fragPos, const vec3 normal, const vec3 viewDir);
float pointShadowCalculation(const PointLight light, const vec3 fragPos, const vec3 normal);

float pointShadowTest(const PointLight light, const vec3 fragPos, const vec3 normal) {
	vec3 fragToLight = fragPos - light.lightPos;
	
	float closestDepth = sampleCubeShadow(light, normalize(fragToLight));
	//closestDepth *= 25;

	closestDepth = closestDepth;

	float currentDepth = (length(fragToLight) / 25);

	float shadow = currentDepth > closestDepth ? 1.0 : 0.0; 

	return currentDepth;

}

float dirShadowTest(const DirLight dirLight, const vec3 fragPos, const vec3 normal) {
	ShadowMapData shadowMapData = dirLight.shadowMapData;

	vec4 fragPosLightSpace = dirLight.lightSpaceMatrix * vec4(fragPos, 0.0);

	//vec4 fragPosLightSpace = FragPosLightSpace;

    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;  // (w is always 1 for ortho projection)
    projCoords = projCoords * 0.5 + 0.5;                            // map [-1, 1] to [0, 1]
    
    if(fragPosLightSpace.z > 1.0 /*|| (projCoords.x < 0.0 || projCoords.x >= 1.0) || (projCoords.y < 0.0 || projCoords.y >= 1.0)*/)
        return 0.0;

    float currentDepth = fragPosLightSpace.z;

	return currentDepth;

}

float linearizeDepth(float d,float zNear,float zFar) {
    return zNear * zFar / (zFar + d * (zNear - zFar));
}

uint getDepthSlice(float depth, float depthSlices) {

	// take these as input?
	float zNear = 0.1;
	float zFar = 200.0;
	//float depthSlices = 24;

	float linearDepth = linearizeDepth(1.0 - depth, zNear, zFar);

	float a = depthSlices / log(zFar/zNear);

	float b = (depthSlices * log(zNear))/(log(zFar/zNear));

	return int(log(linearDepth) * a - b);

	//return int(linearDepth * 24);
}

vec3 displayDepthSlices(float depth) {
	uint depthSlice = getDepthSlice(depth, 24);

	switch (depthSlice % 8) {
		case 0:
			return vec3(0, 0, 1);
		break;case 1:
			return vec3(0, 1, 0);
		break;case 2:
			return vec3(0, 1, 1);
		break;case 3:
			return vec3(1, 0, 0);
		break;case 4:
			return vec3(1, 0, 1);
		break;case 5:
			return vec3(1, 1, 0);
		break;case 6:
			return vec3(1, 1, 1);
		break;case 7:
			return vec3(0, 0, 0);
	}
}


uvec3 getClusterPos(vec3 fragCoord) {
	uint clusterZVal = getDepthSlice(fragCoord.z, 24);
    uvec3 clusters = uvec3(floor((fragCoord.xy * vec2(16, 8)) / vec2(1600, 900) ), clusterZVal);
    return clusters;
}

uint getClusterIndex(vec3 fragCoord){
	uvec3 clusterPos = getClusterPos(fragCoord);
    uint clusterIndex =	(clusterPos.x) +
                    	(clusterPos.y * 16) +
                    	(clusterPos.z * 16 * 8);
    return clusterIndex;
}

// uvec3 getClusterPosFromIndex(uint index) {

// }

void main() {

	vec3 norm = normalize(Normal);
	vec3 viewDir = normalize(sceneData.camPos - FragPos);
	vec3 clusterPos = getClusterPos(gl_FragCoord.xyz);
	uint clusterIndex = getClusterIndex(gl_FragCoord.xyz);
	Cluster cluster = clusters[clusterIndex];
//	float shadow = ShadowCalculation(FragPosLightSpace, sceneData.sunlightDirection, norm);

	vec3 color = vec3(0);

	for(int i = 0; i < dirLightBuffer.count; i++)
		color += calcDirLight(dirLightBuffer.lights[i], FragPos, norm, viewDir);

	// for(int i = 0; i < pointLightBuffer.count; i++)
	// 	color += calcPointLight(pointLightBuffer.lights[i], FragPos, norm, viewDir);

	int lightCount = 0;

	// for (int i = 0; i < activeLights.count; i++) {
	// 	ActiveLight activeLight = activeLights.lights[i];
		
	// 	if (activeLight.type == 1){
	// 		PointLight light = pointLightBuffer.lights[activeLight.index];

	// 		color += calcPointLight(light, FragPos, norm, viewDir);
	// 	}
	// }




	for (int i = 0; i < cluster.count; i++) {
		uint lightIndex = lightIndices.indices[cluster.index + i];
		ActiveLight activeLight = activeLights.lights[lightIndex];
		if (activeLight.type == 1) {	
			PointLight light = pointLightBuffer.lights[activeLight.index];

			color += calcPointLight(light, FragPos, norm, viewDir);
		}
	}



	//TODO: Spotlights

	// color = displayDepthSlices(gl_FragCoord.z);

	// color = vec3(gl_FragCoord.z);

	// color = vec3(linearizeDepth(1.0 - gl_FragCoord.z, 0.1, 200));

	// color = vec3((cluster.count / 10.0), (1.0 - (cluster.count / 10.0)), 0);

	// color = (cluster.minPoint);

	// color = gl_FragCoord.xyz / vec3(1600, 900, 1);

	// color = viewprojPos.xyz;
	
	// color = (camData.viewproj * vec4(FragPos, 1.0)).xyz;

	//color = displayDepthSlices(gl_FragCoord.z);

	//color = vec3((clusterIndex % 16) / 16.0, ((clusterIndex / 16) % 8) / 8.0, 0);
	outFragColor = vec4(color, 1.0);

}

// new stuff

vec3 calcDirLight(const DirLight dirLight, const vec3 fragPos, const vec3 normal, const vec3 viewDir) {
    vec3 newLightDir = normalize(dirLight.lightDir);
    vec3 halfwayDir = normalize(newLightDir + viewDir);
    // diffuse shading
    float diff = max(dot(normal, newLightDir), 0.0);
    // specular shading
    vec3 reflectDir = reflect(-newLightDir, normal);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), 32); //TODO: change 32 to material shininess
    // combine results
    vec3 ambient  = dirLight.ambientColor;
    vec3 diffuse  = dirLight.lightColor  * diff;
    vec3 specular = dirLight.lightColor * spec * 0;
    //return (ambient + diffuse + specular);

	float shadow = dirShadowCalculation(dirLight, fragPos, normal);

    return (ambient + (1.0 - shadow) * (diffuse + specular));
}

float dirShadowCalculation(const DirLight dirLight, const vec3 fragPos, const vec3 normal) {

	ShadowMapData shadowMapData = dirLight.shadowMapData;

	vec4 fragPosLightSpace;

	fragPosLightSpace = dirLight.lightSpaceMatrix * vec4(fragPos, 1.0);

    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;  // (w is always 1 for ortho projection)
    projCoords = projCoords * 0.5 + 0.5;                            // map [-1, 1] to [0, 1]
    
    if(fragPosLightSpace.z > 1.0 /*|| (projCoords.x < 0.0 || projCoords.x >= 1.0) || (projCoords.y < 0.0 || projCoords.y >= 1.0)*/)
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
			//pcfDepth = texture(shadowMap, (projCoords.xy + (vec2(x, y) * texelSize))/8).r;

			pcfDepth = sampleShadow(shadowMapData, projCoords.xy + (vec2(x, y) * texelSize));
			shadow += currentDepth > pcfDepth ? 1.0 : 0.0; 
		}
	}
	shadow /= (samples * samples);

    return shadow;
}

vec3 calcPointLight(const PointLight light, const vec3 fragPos, const vec3 normal, const vec3 viewDir) {
	vec3 lightToFrag = light.lightPos - fragPos;

	float sqDist = dot(lightToFrag, lightToFrag);
	float sqRadius = light.radius * light.radius;
	if (sqDist > sqRadius) {
		return vec3(0.0);
	}

	vec3 lightDir = normalize(lightToFrag);
    vec3 halfwayDir = normalize(lightDir + viewDir);

    // diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);
    // specular shading
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), 32); //TODO: change 32 to material shininess
    
    // combine results
    vec3 ambient  = light.ambientColor;
    vec3 diffuse  = light.lightColor  * diff;
    vec3 specular = light.lightColor * spec * 0;

	// attenuation
    // float distance    = length(light.lightPos - fragPos);
    // float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));

	
	
	float attenuation = clamp(1.0 - (sqDist/sqRadius), 0.0, 1.0);
	attenuation *= attenuation;

    ambient  *= attenuation;
    diffuse  *= attenuation;
    specular *= attenuation;

	float shadow = pointShadowCalculation(light, fragPos, normal);

    return (ambient + (1.0 - shadow) * (diffuse + specular));
}

float pointShadowCalculation(const PointLight light, const vec3 fragPos, const vec3 normal) {
	vec3 fragToLight = fragPos - light.lightPos;
	
	float closestDepth = sampleCubeShadow(light, normalize(fragToLight));
	closestDepth *= light.radius;

	float currentDepth = length(fragToLight);

	float shadow = currentDepth - 0.2 > closestDepth ? 1.0 : 0.0;

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
		return 1.0;
	}
	vec2 mapCoords = ((pos * shadowMapData.shadowMapSize)																// Convert [0, 1] to [0, mapSize]
					 + vec2(shadowMapData.shadowMapX, shadowMapData.shadowMapY))								// Move to tile within the shadow atlas
					 / 8; 																// Convert [0, 8] to [0, 1]

	return texture(shadowMap, mapCoords).r;
}

// float sampleShadow(const int face, const vec2 pos) {

// 	if ((pos.x >= 1.0 || pos.x < 0.0) || (pos.y >= 1.0 || pos.y < 0.0)) {
// 		return 1.0;
// 	}
// 	vec2 mapCoords = ((pos)																// Convert [0, 1] to [0, mapSize]
// 					 + vec2(face + 1, 0))												// Move to tile within the shadow atlas
// 					 / 8; 																// Convert [0, 8] to [0, 1]
	
// 	//mapCoords = (pos + vec2(2, 0)) / 8;

// 	//mapCoords = pos / 8;

// 	return texture(shadowMap, mapCoords).r;
// }

float sampleCubeShadow(const PointLight light, const vec3 v) {
	int face;
	vec2 coords = sampleCube(v, face);

	ShadowMapData shadowMapData = light.shadowMapData[face];

	return sampleShadow(shadowMapData, coords);
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