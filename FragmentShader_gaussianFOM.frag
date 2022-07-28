#version 330 core
in vec2 TexCoords;

layout (location = 0) out vec4 FOMtex0;
layout (location = 1) out vec4 FOMtex1;

uniform sampler2D fbo0;
uniform sampler2D fbo1;
uniform bool gaussianDir;
uniform int kernelRadius;
uniform float sigma;

//out vec4 color;


const float PI = 3.1415926;

float gaussian(float sigma, float delta)
{
	return 1 / (sigma * sqrt(2 * PI)) * exp(-pow(delta, 2) / (2 * pow(sigma, 2)));
}


void main()
{
	vec4 sumA = vec4(0);
    vec4 sumB = vec4(0);
    float weightSum = 0;
    vec2 t = gaussianDir ? vec2(1.0, 0.0) : vec2(0.0, 1.0); // seperate gaussian filtering
    vec2 texelSize = 1.0 / textureSize(fbo0, 0);

    float gaussianWeight = gaussian(sigma, 0);
    sumA += gaussianWeight * texture(fbo0, TexCoords, 0.0);
    sumB += gaussianWeight * texture(fbo1, TexCoords, 0.0);
    weightSum += gaussianWeight;


	for(int i = 1; i < kernelRadius; ++i)
    {
        vec2 offset = i * t; // compute offset for gaussin filtering
        gaussianWeight = gaussian(sigma, i);

        // One side with positive offset
        sumA += gaussianWeight * texture(fbo0, TexCoords + offset * texelSize, 0.0);   
        sumB += gaussianWeight * texture(fbo1, TexCoords + offset * texelSize, 0.0);   

        // The other side with negative offset
        sumA += gaussianWeight * texture(fbo0, TexCoords - offset * texelSize, 0.0); 
        sumB += gaussianWeight * texture(fbo1, TexCoords - offset * texelSize, 0.0);

        weightSum += 2 * gaussianWeight;
    }

	//color = vec4(sum / weightSum , 1.0);  
    FOMtex0 = sumA/weightSum;
	FOMtex1 = sumB/weightSum;
}