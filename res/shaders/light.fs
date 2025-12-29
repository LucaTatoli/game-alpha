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
uniform sampler2D texture0;
uniform float time;

out vec4 FragColor;

float map(float value, float min1, float max1, float min2, float max2) {
  return min2 + (value - min1) * (max2 - min2) / (max1 - min1);
}

float quantize(float value, float min, float max, int step) {
	if(value > max) return max;
	if(value < min) return min;

	float stepValue = (max - min) / step;
	float multiplier = int((value - min) / stepValue);

	return stepValue * multiplier + min;
}

void main() { 

	vec3 lightPos = normalize(vec3(0.0, 2.0, 1.0));

	// calculate the amount of light absorbed by
	// local light based on the distance from the source
	float dist = distance(localLight, fragPosition);
	vec3 calcLocalLightColor = localLightColor / max(dist * dist, 0.5);
	vec3 localLightVector = localLight - fragPosition;
	float dotProductLocalLight = dot(fragNormal, normalize(localLightVector));
	int quantizeFactor = 4;
	float localLightDiff = quantize(dotProductLocalLight, 0, 1, quantizeFactor);
	calcLocalLightColor = calcLocalLightColor * localLightDiff;

	vec4 texColor = texture(texture0, fragTexCoord);
	
	float dotProductLight = dot(fragNormal, normalize(lightPos));
	float diff = quantize(dotProductLight, 0, 1, quantizeFactor);
	vec3 diffuse = diff * lightColor + (calcLocalLightColor * map(sin(time), -1, 1, 0.6, 1));

	vec3 result = (ambient + diffuse) * texColor.xyz * colDiffuse.xyz * fragColor.xyz;
	FragColor = vec4(result, 1.0);
}
