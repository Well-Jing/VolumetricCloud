#version 330 core

uniform float time;
uniform float extinction;
uniform vec2 resolution;
uniform vec3 sunDirection;
uniform mat4 invView;
uniform mat4 invProj;

uniform sampler3D perlworl;
uniform sampler3D worl;
uniform sampler2D curl;
uniform sampler2D weather;
uniform sampler2D blueNoise;

const float g_radius = 200000.0; //ground radius
const float sky_b_radius = 201000.0; //bottom of cloud layer
const float sky_t_radius = 204000.0; //top of cloud layer 202300.0
const float c_goldenRatioConjugate = 0.61803398875f;

//const float extinction = 25;

out vec4 color;

// fractional value for sample position in the cloud layer
float GetHeightFractionForPoint(float inPosition)
{ // get global fractional position in cloud zone
	float heightFraction = (inPosition -  sky_b_radius) / (sky_t_radius - sky_b_radius); 
	return clamp(heightFraction, 0.0, 1.0);
}

vec4 mixGradients(const float cloudType)
{
	const vec4 STRATUS_GRADIENT = vec4(0.02f, 0.05f, 0.09f, 0.11f);
	const vec4 STRATOCUMULUS_GRADIENT = vec4(0.02f, 0.2f, 0.48f, 0.625f);
	const vec4 CUMULUS_GRADIENT = vec4(0.01f, 0.0625f, 0.78f, 1.0f); // these fractions would need to be altered if cumulonimbus are added to the same pass
	float stratus = 1.0f - clamp(cloudType * 2.0f, 0.0, 1.0);
	float stratocumulus = 1.0f - abs(cloudType - 0.5f) * 2.0f;
	float cumulus = clamp(cloudType - 0.5f, 0.0, 1.0) * 2.0f;
	return STRATUS_GRADIENT * stratus + STRATOCUMULUS_GRADIENT * stratocumulus + CUMULUS_GRADIENT * cumulus;
}

float densityHeightGradient(const float heightFrac, const float cloudType) 
{
	vec4 cloudGradient = mixGradients(cloudType);
	return smoothstep(cloudGradient.x, cloudGradient.y, heightFrac) - smoothstep(cloudGradient.z, cloudGradient.w, heightFrac);
}

// Utility function that maps a value from one range to another. 
float remap(const float originalValue, const float originalMin, const float originalMax, const float newMin, const float newMax)
{
	return newMin + (((originalValue - originalMin) / (originalMax - originalMin)) * (newMax - newMin));
}

float density(vec3 p, vec3 weather, const bool hq, const float LOD) 
{
	float heightFraction = GetHeightFractionForPoint(length(p)); // the p should be used before times time, otherwise, after a long time, the p will become much higher than the sky top, there is no cloud
	p.z += time * 0;
	vec4 lowNoise = textureLod(perlworl, p * 0.00035, LOD);  //textureLod and texture function are similar, I still do not understand the difference
	float fbm = lowNoise.g*0.625 + lowNoise.b*0.25 + lowNoise.a*0.125;
	float base_cloud = remap(lowNoise.r, -(1.0 - fbm), 1.0, 0.0, 1.0);
	float gradient = densityHeightGradient(heightFraction, clamp(pow(weather.b, 1.8), 0, 1));
	float cloud_coverage = smoothstep(0.6, 1.3, weather.x);  // the choice of smoothstep is smart here, you can search smoothstep in wikipedia
	base_cloud = remap(base_cloud*gradient, 1.0 - cloud_coverage, 1.0, 0.0, 1.0);
	base_cloud *= cloud_coverage;
	
	if (hq) 
	{
		//vec2 whisp = texture(curl, p.xy * 0.0003).xy;
		//p.xy += whisp * 400.0 * (1.0 - heightFraction);
		vec3 highNoise = texture(worl, p * 0.00011, LOD - 2.0).xyz; // origianl speed 0.004
		float hfbm = highNoise.r*0.625 + highNoise.g*0.25 + highNoise.b*0.125;
		hfbm = mix(hfbm, 1.0 - hfbm, clamp(heightFraction * 3.0, 0.0, 1.0));
		base_cloud = remap(base_cloud, hfbm * 0.2, 1.0, 0.0, 1.0);
	}
	return clamp(base_cloud, 0.0, 1.0);
}

vec4 march(vec3 pos, vec3 dir, float stepDist, int numSamples) 
{
	vec3 p = pos;           // sample position
	float totalTrans = 1.0; // total tranparency
	float depth = 0;
	const float densityScale = 0.025; // scale the attenuation of cloud // this value is not match the sky shader, but large value leads to problem
	const float weatherScale = 0.00006; // original 0.00008
	const float breakTrans = 0.05; // hardcode value, can be manipulated by the artists
	float THRESHOLD = 0.001;

	float frontDepth = -1;
	float maxOpticalDepth = 0;
	float meanExtinction = 0;
	float extinctionAccumulator = 0;
	int sampleCounter = 0;

	for (int i = 0; i < numSamples; i++) // sample points until the max number
	{
		p += dir * stepDist; // move forward
		depth += stepDist;
		vec3 weatherSample = texture(weather, p.xz * weatherScale).xyz; // get weather information
		float viewRayDensity = densityScale * density(p, weatherSample, false, 0.0); // compute density

		if(viewRayDensity > THRESHOLD)
		{
			if (frontDepth < 0)	frontDepth = depth;

			extinctionAccumulator += viewRayDensity;
			maxOpticalDepth += viewRayDensity * stepDist;
			
			sampleCounter++;
		}

		float transmitance = exp(-viewRayDensity * stepDist); 
		totalTrans *= transmitance;	

		if (totalTrans < breakTrans) break; 
	}

	if(sampleCounter > 0)
	{
		meanExtinction = extinctionAccumulator / sampleCounter;
	}
	else
	{
		meanExtinction = 0.0f;
		frontDepth = stepDist * numSamples;
		maxOpticalDepth = 0;
	}

	return vec4(frontDepth, meanExtinction, maxOpticalDepth, 1);
	//return vec4(frontDepth / (stepDist * numSamples));
}


void main()
{             
    vec2 uv = gl_FragCoord.xy / resolution;
    uv = uv-vec2(0.5);
	uv *= 2.0;
	vec4 uvdir = vec4(uv.xy, 1.0, 1.0);
	vec3 worldPos =  vec3(0.0, g_radius, 0.0) + (invView * (invProj * uvdir)).xyz; 
	//vec4 worldPos = abs(invView *(invProj * uvdir)) / 5000;
	vec3 dir = -normalize(sunDirection);

	float extinction = 2;
	int numStep = 100; // default 100
	//float stepDist = 50;
	float stepDist = 15000 / numStep;
	//vec4 expDist = exp(extinction * march(worldPos, dir, stepDist, numStep));

	float blueNoiseRate = 20;
	float blueNoise = texture(blueNoise, gl_FragCoord.xy / 1024.0f).r;
	vec3 blueNoiseOffset = fract(blueNoise + float(0 * 100) * c_goldenRatioConjugate) * dir * blueNoiseRate;  // use blue noise
		
	//vec3 lweather = texture(weather, vec2(0.1f, 0.1f)).xyz;
	//vec4 volume = march(start + blueNoiseOffset, end, raystep, int(steps));
	//vec4 expDist = march(worldPos + blueNoiseOffset, dir, stepDist, numStep);
	vec4 expDist = march(worldPos, dir, stepDist, numStep);
	color = expDist;
} 