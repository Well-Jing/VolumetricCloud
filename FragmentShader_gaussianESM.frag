#version 330 core
in vec2 TexCoords;

uniform sampler2D fbo;
uniform bool gaussianDir;
uniform int kernelRadius;
uniform float sigma;

out vec4 color;


const float PI = 3.1415926;

float gaussian(float sigma, float delta)
{
	return 1 / (sigma * sqrt(2 * PI)) * exp(-pow(delta, 2) / (2 * pow(sigma, 2)));
}

void main()
{
	float sum = 0;
    float weightSum = 0;
    vec2 t = gaussianDir ? vec2(1.0, 0.0) : vec2(0.0, 1.0); // seperate gaussian filtering
    vec2 texelSize = 1.0 / textureSize(fbo, 0);

    float gaussianWeight = gaussian(sigma, 0);
    sum += gaussianWeight * texture(fbo, TexCoords, 0.0).r; 
    weightSum += gaussianWeight;

	for(int i = 1; i < kernelRadius; ++i)
    {
        vec2 offset = i * t; // compute offset for gaussin filtering
        gaussianWeight = gaussian(sigma, i);

        // One side with positive offset
        sum += gaussianWeight * texture(fbo, TexCoords + offset * texelSize, 0.0).r;       

        // The other side with negative offset
        sum += gaussianWeight * texture(fbo, TexCoords - offset * texelSize, 0.0).r; 

        weightSum += 2 * gaussianWeight;
    }

	color = vec4(vec3(sum / weightSum), 1.0);  
}
