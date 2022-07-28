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

//void main()
//{
//	vec2 sum = vec2(0);
//    float weightSum = 0;
//    vec2 t = gaussianDir ? vec2(1.0, 0.0) : vec2(0.0, 1.0); // seperate gaussian filtering
//    vec2 texelSize = 1.0 / textureSize(fbo, 0);
//
//    float gaussianWeight = gaussian(sigma, 0);
//    sum += gaussianWeight * texture(fbo, TexCoords, 0.0).gb; 
//    weightSum += gaussianWeight;
//
//    float frontDepth = texture(fbo, TexCoords, 0.0).r;
//
//	for(int i = 1; i < kernelRadius; ++i)
//    {
//        vec2 offset = i * t; // compute offset for gaussin filtering
//        gaussianWeight = gaussian(sigma, i);
//
//        // One side with positive offset
//        sum += gaussianWeight * texture(fbo, TexCoords + offset * texelSize, 0.0).gb;       
//
//        // The other side with negative offset
//        sum += gaussianWeight * texture(fbo, TexCoords - offset * texelSize, 0.0).gb; 
//
//        weightSum += 2 * gaussianWeight;
//    }
//
//	color = vec4(frontDepth, sum / weightSum , 1.0);  
//}


void main()
{
	vec3 sum = vec3(0);
    float weightSum = 0;
    vec2 t = gaussianDir ? vec2(1.0, 0.0) : vec2(0.0, 1.0); // seperate gaussian filtering
    vec2 texelSize = 1.0 / textureSize(fbo, 0);

    float gaussianWeight = gaussian(sigma, 0);
    sum += gaussianWeight * texture(fbo, TexCoords, 0.0).rgb; 
    weightSum += gaussianWeight;

    float frontDepth = texture(fbo, TexCoords, 0.0).r;

	for(int i = 1; i < kernelRadius; ++i)
    {
        vec2 offset = i * t; // compute offset for gaussin filtering
        gaussianWeight = gaussian(sigma, i);

        // One side with positive offset
        sum += gaussianWeight * texture(fbo, TexCoords + offset * texelSize, 0.0).rgb;       

        // The other side with negative offset
        sum += gaussianWeight * texture(fbo, TexCoords - offset * texelSize, 0.0).rgb; 

        weightSum += 2 * gaussianWeight;
    }

	color = vec4(sum / weightSum , 1.0);  
}