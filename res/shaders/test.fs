#version 330
in vec3 fragPosition;
in vec3 fragNormal;
in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;
uniform vec2 resolution;
out vec4 FragColor;

vec2 barrelDistortion(vec2 uv, float strength) {
	vec2 cc = uv - 0.5;
	float dist = dot(cc, cc);
	return uv + cc * dist * strength;
}

float map(float value, float a, float b, float n_a, float n_b) {
	return (value - a) / (b - a) * (n_b - n_a) + n_a;
}

void main() { 
	
	// barrel distortion
	vec2 uv = barrelDistortion(fragTexCoord, 0.15);

	if(uv.x < 0.0 || uv.x > 1.0 || uv.y < 0 || uv.y > 1.0) {
		FragColor = vec4(0.01, 0.01, 0.01, 1);
		return;
	}

	// chromatic aberration
	float offset = 1.0 / resolution.x * 5;
	float r = texture(texture0, uv + vec2(-offset, 0)).r;
	float g = texture(texture0, uv).g;
	float b = texture(texture0, uv + vec2(offset, 0)).b;
	vec3 color = vec3(r, g, b);

	// scanlines
	float scan = sin(uv.y * resolution.y * 3.14159);
	float scanFactor = mix(0.85, 1.0, scan * scan);
	color *= scanFactor;

	// aperture grille
	float grille = fract(uv.x * 2000.5146);
	float amount = 0.85;
	vec3 mask = (grille < 0.33) ? vec3(1, amount, amount) :
				(grille < 0.66) ? vec3(amount, 1, amount) :
	 	                          vec3(amount, amount, 1);

	// pixelated effect
	ivec2 iuv = ivec2(fragTexCoord * 500);
	bool even = ((iuv.x + iuv.y) % 2) == 0;
	float intensity = 0.9;
	if(even) color *= vec3(intensity, intensity, intensity);

	color *= mask;

	// vignettatura leggera
	float dist = length(uv - 0.5);
	float vignette = smoothstep(0.8, 0.5, dist);
	color *= vignette;
	
	FragColor = vec4(color,  1.0);
}
