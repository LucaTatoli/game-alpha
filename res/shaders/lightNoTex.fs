#version 330
in vec3 fragPosition;
in vec3 fragNormal;
in vec2 fragTexCoord;
in vec4 fragColor;

uniform vec4 colDiffuse;
uniform vec3 lightColor;
uniform vec3 localLightColor;
uniform vec3 localLight;
uniform vec3 ambient;
uniform float time;

out vec4 FragColor;

float map(float value, float min1, float max1, float min2, float max2) {
  return min2 + (value - min1) * (max2 - min2) / (max1 - min1);
}

void main() { 

	vec3 lightPos = normalize(vec3(0.0, 2.0, 1.0));

	// calculate the amount of light absorbed by
	// local light based on the distance from the source
	float dist = distance(localLight, fragPosition);
	dist = dist;
	vec3 calcLocalLightColor = localLightColor / max(dist * dist, 0.5);
	vec3 localLightVector = localLight - fragPosition;
	float localLightDiff = max(dot(fragNormal, normalize(localLightVector)), 0.0);
	calcLocalLightColor = calcLocalLightColor * localLightDiff;

	float diff = max(dot(fragNormal, lightPos), 0.0);
	vec3 diffuse = diff * lightColor + (calcLocalLightColor * map(sin(time), -1, 1, 0.6, 1));

	vec3 result = (ambient + diffuse) * colDiffuse.xyz * fragColor.xyz;
	FragColor = vec4(result, 1.0);
}
