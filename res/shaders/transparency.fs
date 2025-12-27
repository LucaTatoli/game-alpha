#version 330
in vec3 fragPosition;
in vec3 fragNormal;
in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;
uniform float time;

uniform vec3 lightColor;
uniform vec3 localLightColor;
uniform vec3 localLight;
uniform vec3 ambient;
uniform vec4 colDiffuse;

out vec4 FragColor;

float map(float value, float min1, float max1, float min2, float max2) {
   return min2 + (value - min1) * (max2 - min2) / (max1 - min1);
}


void main() { 

	vec4 texColor = texture(texture0, fragTexCoord);
	if(texColor.a <= 0.0)
		discard;

    // calculate the amount of light absorbed by
    // local light based on the distance from the source
    float dist = distance(localLight, fragPosition);
    vec3 calcLocalLightColor = localLightColor / max(dist * dist, 0.5);
	
	vec3 result = (ambient + lightColor + calcLocalLightColor * map(sin(time), -1, 1, 0.6, 1)) * texColor.xyz;
	FragColor = vec4(result, texColor.w);

}
