#version 330 core
//TODO update cloud shaping
//TODO fix up weather texture
//TODO add more flexibility to parameters
	// rain/coverage
	// earth radius
	// sky color //maybe LUT // or just vec3 for coefficients
//TODO add 2d cloud layer on top

uniform sampler3D perlworl;
uniform sampler3D worl;
uniform sampler2D curl;
uniform sampler2D weather;
uniform sampler2D blueNoise;
uniform sampler2D ESM;

uniform int check;
uniform int debugValueInt;
uniform mat4 sunView;
uniform mat4 sunProj;
uniform mat4 MVPM; 
uniform mat4 invMVPM;
uniform mat4 invView;
uniform mat4 invProj;
uniform float aspect;
uniform float time;
uniform float blueNoiseRate;
uniform float debugValue;
uniform float lowSampleNum;
uniform float highSampleNum;
uniform float downscale;
uniform vec2 resolution;
uniform vec3 cameraPos;

//preetham variables
in vec3 vSunDirection;
in float vSunfade;
in vec3 vBetaR;
in vec3 vBetaM;
in float vSunE;
in vec3 vAmbient;
in vec3 vSunColor;

out vec4 color;

//I like the look of the small sky, but could be tweaked however
const float g_radius = 200000.0; //ground radius
const float sky_b_radius = 201000.0;//bottom of cloud layer
const float sky_t_radius = 204000.0;//top of cloud layer 202300.0
//float sky_t_radius = 204000.0 + debugValue;//top of cloud layer 202300.0
const float c_radius = 6008400.0; //2d noise layer

const float cwhiteScale = 1.1575370919881305;//precomputed 1/U2Tone(40)
const float c_goldenRatioConjugate = 0.61803398875f;
/*
	 SHARED FUNCTIONS
*/

// HDR tone mapping function
vec3 U2Tone(const vec3 x)  // uncharted 2 tone mapping
{
	const float A = 0.15;
	const float B = 0.50;
	const float C = 0.10;
	const float D = 0.20;
	const float E = 0.02;
	const float F = 0.30;

   return ((x*(A*x+C*B)+D*E)/(x*(A*x+B)+D*F))-E/F;
}


float HG(float costheta, float g) 
{
	const float k = 0.0795774715459; 
	return k * (1.0 - g * g) / pow(1.0 + g * g - 2.0 * g * costheta, 1.5);
}


/*
	 stars from a different project
	 */

uniform vec3 moonpos = vec3(0.3, 0.3, 0.0);


float SimplexPolkaDot3D( 	vec3 P, float density )	//	the maximal dimness of a dot ( 0.0->1.0   0.0 = all dots bright,  1.0 = maximum variation )
{
    //	calculate the simplex vector and index math
    vec3 Pi;
    vec3 Pi_1;
    vec3 Pi_2;
    vec4 v1234_x;
    vec4 v1234_y;
    vec4 v1234_z;

	vec3 Pn = P;
    //	simplex math constants
    float SKEWFACTOR = 1.0/3.0;
    float UNSKEWFACTOR = 1.0/6.0;
    float SIMPLEX_CORNER_POS = 0.5;
    float SIMPLEX_PYRAMID_HEIGHT = 0.70710678118654752440084436210485;	// sqrt( 0.5 )	height of simplex pyramid.

    Pn *= SIMPLEX_PYRAMID_HEIGHT;		// scale space so we can have an approx feature size of 1.0  ( optional )

    //	Find the vectors to the corners of our simplex pyramid
    Pi = floor( Pn + vec3(dot( Pn, vec3( SKEWFACTOR) ) ));
    vec3 x0 = Pn - Pi + vec3(dot(Pi, vec3( UNSKEWFACTOR ) ));
    vec3 g = step(x0.yzx, x0.xyz);
    vec3 l = vec3(1.0) - g;
    Pi_1 = min( g.xyz, l.zxy );
    Pi_2 = max( g.xyz, l.zxy );
    vec3 x1 = x0 - Pi_1 + vec3(UNSKEWFACTOR);
    vec3 x2 = x0 - Pi_2 + vec3(SKEWFACTOR);
    vec3 x3 = x0 - vec3(SIMPLEX_CORNER_POS);

    //	pack them into a parallel-friendly arrangement
    v1234_x = vec4( x0.x, x1.x, x2.x, x3.x );
    v1234_y = vec4( x0.y, x1.y, x2.y, x3.y );
    v1234_z = vec4( x0.z, x1.z, x2.z, x3.z );

	vec3 gridcell = Pi;
	vec3 v1_mask = Pi_1;
	vec3 v2_mask = Pi_2;

    vec2 OFFSET = vec2( 50.0, 161.0 );
    float DOMAIN = 69.0;
    float SOMELARGEFLOAT = 6351.29681;
    float ZINC = 487.500388;

    //	truncate the domain
    gridcell.xyz = gridcell.xyz - floor(gridcell.xyz * ( 1.0 / DOMAIN )) * DOMAIN;
    vec3 gridcell_inc1 = step( gridcell, vec3( DOMAIN - 1.5 ) ) * ( gridcell + vec3(1.0) );

    //	compute x*x*y*y for the 4 corners
    vec4 Pp = vec4( gridcell.xy, gridcell_inc1.xy ) + vec4(OFFSET.xy, OFFSET.xy);
    Pp *= Pp;
    vec4 V1xy_V2xy = mix( vec4(Pp.xy, Pp.xy), vec4(Pp.zw, Pp.zw), vec4( v1_mask.xy, v2_mask.xy ) );		//	apply mask for v1 and v2
    Pp = vec4( Pp.x, V1xy_V2xy.x, V1xy_V2xy.z, Pp.z ) * vec4( Pp.y, V1xy_V2xy.y, V1xy_V2xy.w, Pp.w );

    vec2 V1z_V2z = vec2(gridcell_inc1.z);
	if (v1_mask.z <0.5) {
		V1z_V2z.x = gridcell.z;
	}
	if (v2_mask.z <0.5) {
		V1z_V2z.y = gridcell.z;
	}
	vec4 temp = vec4(SOMELARGEFLOAT) + vec4( gridcell.z, V1z_V2z.x, V1z_V2z.y, gridcell_inc1.z ) * ZINC;
    vec4 mod_vals =  vec4(1.0) / ( temp );

    //	compute the final hash
    vec4 hash = fract( Pp * mod_vals );
	

    //	apply user controls
    float INV_SIMPLEX_TRI_HALF_EDGELEN = 2.3094010767585030580365951220078;	// scale to a 0.0->1.0 range.  2.0 / sqrt( 0.75 )
    float radius = INV_SIMPLEX_TRI_HALF_EDGELEN;///(1.15-density);
    v1234_x *= radius;
    v1234_y *= radius;
    v1234_z *= radius;

    //	return a smooth falloff from the closest point.  ( we use a f(x)=(1.0-x*x)^3 falloff )
    vec4 point_distance = max( vec4( 0.0 ), vec4(1.0) - ( v1234_x*v1234_x + v1234_y*v1234_y + v1234_z*v1234_z ) );
    point_distance = point_distance*point_distance*point_distance;
	vec4 b = (vec4(density)-hash)*(1.0/density);
	b = max(vec4(0.0), b);
	b = min(vec4(1.0), b);
	b = pow(b, vec4(1.0/density));
    return dot(b, point_distance);
}

vec3 stars(vec3 ndir) 
{

	vec3 COLOR = vec3(0.0);
	float star = SimplexPolkaDot3D(ndir*100.0, 0.15)+SimplexPolkaDot3D(ndir*150.0, 0.25)*0.7;
	vec3 col = vec3(0.05, 0.07, 0.1);
	COLOR.rgb = col+max(0.0, (star-smoothstep(0.2, 0.95, 0.5-0.5*ndir.y)));
	COLOR.rgb += vec3(0.05, 0.07, 0.1)*(1.0-smoothstep(-0.1, 0.45, ndir.y));
	//need to add a little bit of atmospheric effects, both for when the moon is high
	//and for the end when color comes into the sky
	//For moon halo
	//COLOR.rgb += smoothstep(0.9, 1.0, dot(ndir, normalize(moonpos)));
	//float d = length(ndir-normalize(moonpos));
	//COLOR.rgb += 0.8*exp(-4.0*d)*vec3(1.1, 1.0, 0.8);
	//COLOR.rgb += 0.2*exp(-2.0*d);
	
	return COLOR;
}















/*
//This implementation of the preetham model is a modified: https://github.com/mrdoob/three.js/blob/master/examples/js/objects/Sky.js
//written by: zz85 / https://github.com/zz85
	 */

const float mieDirectionalG = 0.8;

// constants for atmospheric scattering
const float pi = 3.141592653589793238462643383279502884197169;

// optical length at zenith for molecules
const float rayleighZenithLength = 8.4E3;
const float mieZenithLength = 1.25E3;
const vec3 up = vec3( 0.0, 1.0, 0.0 );
// 66 arc seconds -> degrees, and the cosine of that
const float sunAngularDiameterCos = 0.999956676946448443553574619906976478926848692873900859324;

// 3.0 / ( 16.0 * pi )
const float THREE_OVER_SIXTEENPI = 0.05968310365946075;

const float whiteScale = 1.0748724675633854; // 1.0 / Uncharted2Tonemap(1000.0)

vec3 preetham(const vec3 vWorldPosition) 
{
// optical length
// cutoff angle at 90 to avoid singularity in next formula.
	float zenithAngle = acos( max( 0.0, dot( up, normalize( vWorldPosition ) ) ) );
	float inv = 1.0 / ( cos( zenithAngle ) + 0.15 * pow( 93.885 - ( ( zenithAngle * 180.0 ) / pi ), -1.253 ) );
	float sR = rayleighZenithLength * inv;
	float sM = mieZenithLength * inv;

	// combined extinction factor
	vec3 Fex = exp( -( vBetaR * sR + vBetaM * sM ) );

		// in scattering
	float cosTheta = dot( normalize( vWorldPosition ), vSunDirection );

	float rPhase = THREE_OVER_SIXTEENPI * ( 1.0 + pow( cosTheta*0.5+0.5, 2.0 ) );
	vec3 betaRTheta = vBetaR * rPhase;

	float mPhase = HG( cosTheta, mieDirectionalG );
	vec3 betaMTheta = vBetaM * mPhase;

	vec3 Lin = pow( vSunE * ( ( betaRTheta + betaMTheta ) / ( vBetaR + vBetaM ) ) * ( 1.0 - Fex ), vec3( 1.5 ) );
	Lin *= mix( vec3( 1.0 ), pow( vSunE * ( ( betaRTheta + betaMTheta ) / ( vBetaR + vBetaM ) ) * Fex, vec3( 1.0 / 2.0 ) ), clamp( pow( 1.0 - dot( up, vSunDirection ), 5.0 ), 0.0, 1.0 ) );

	vec3 L0 = vec3( 0.5 ) * Fex;

	// composition + solar disc
	float sundisk = smoothstep( sunAngularDiameterCos, sunAngularDiameterCos + 0.00002, cosTheta );
	L0 += ( vSunE * 19000.0 * Fex ) * sundisk;

	vec3 texColor = ( Lin + L0 ) * 0.04 + vec3( 0.0, 0.0003, 0.00075 );

	vec3 curr = U2Tone( texColor );
	vec3 color = curr * whiteScale;

	vec3 retColor = pow( color, vec3( 1.0 / ( 1.2 + ( 1.2 * vSunfade ) ) ) );

	return retColor;
}
/*
	 ===============================================================
	 end of atmospheric scattering
*/

const vec3 RANDOM_VECTORS[6] = vec3[6]
(
	vec3( 0.38051305f,  0.92453449f, -0.02111345f),
	vec3(-0.50625799f, -0.03590792f, -0.86163418f),
	vec3(-0.32509218f, -0.94557439f,  0.01428793f),
	vec3( 0.09026238f, -0.27376545f,  0.95755165f),
	vec3( 0.28128598f,  0.42443639f, -0.86065785f),
	vec3(-0.16852403f,  0.14748697f,  0.97460106f)
	);

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

// very simple geometry and trigonometry \
// (https://www.scratchapixel.com/lessons/3d-basic-rendering/minimal-ray-tracer-rendering-simple-shapes/ray-sphere-intersection)
float intersectSphere(const vec3 pos, const vec3 dir, const float r) 
{
    float a = dot(dir, dir);
    float b = 2.0 * dot(dir, pos);
    float c = dot(pos, pos) - (r * r);
	float d = sqrt((b * b) - 4.0 * a * c);
	float p = -b - d;
	float p2 = -b + d;
    return max(0, max(p, p2) / (2.0 * a));
}

vec2 intersectSphereMix(const vec3 pos, const vec3 dir, const float r0, const float r1) 
{
    float a = dot(dir, dir);
    float b = 2.0 * dot(dir, pos);
    float c = dot(pos, pos) - (r0* r0);
	float d = sqrt((b * b) - 4.0 * a * c);
	float p0_0 = -b - d;
	float p0_1 = -b + d;
    
    c = dot(pos, pos) - (r1* r1);
	d = sqrt((b * b) - 4.0 * a * c);
	float p1_0 = -b - d;
	float p1_1 = -b + d;

	if (pos.y < r0)
	{
		return vec2(max(p0_0, p0_1) / (2.0 * a), max(p1_0, p1_1) / (2.0 * a));
	}
	else if (pos.y >= r0 && pos.y <= r1)
	{
		return vec2(0, dir.y >= 0? max(p1_0, p1_1) / (2.0 * a) : min(p0_0, p0_1) / (2.0 * a));
	}
	else 
	{
		if (dir.y >= 0) return vec2(0, 0);
		else return vec2(min(p1_0, p1_1) / (2.0 * a), min(p0_0, p0_1) / (2.0 * a));
		//else return vec2(0, min(p0_0, p0_1) / (2.0 * a));
	}
}

// Utility function that maps a value from one range to another. 
float remap(const float originalValue, const float originalMin, const float originalMax, const float newMin, const float newMax)
{
	return newMin + (((originalValue - originalMin) / (originalMax - originalMin)) * (newMax - newMin));
}



float density(vec3 p, vec3 weather, const bool hq, const float LOD) 
{
	float heightFraction = GetHeightFractionForPoint(length(p)); // the p should be used before times time, otherwise, the p will become much higher than the sky top, there is no cloud
	p.z += time * 0;
	vec4 lowNoise = textureLod(perlworl, p * 0.00035, LOD);  //textureLod and texture function are similar, I still do not understand the difference
	float fbm = lowNoise.g*0.625 + lowNoise.b*0.25 + lowNoise.a*0.125;
	//float g = densityHeightGradient(height_fraction, 0.5);  // why the cloud type is only one type ??? why do not use the weather map?
	float gradient = densityHeightGradient(heightFraction, clamp(pow(weather.b, 1.8), 0, 1));
	float base_cloud = remap(lowNoise.r, -(1.0 - fbm), 1.0, 0.0, 1.0);
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

float getLight(vec3 p, vec3 secondRayDir, vec3 viewDir, float weatherScale, float densityScale)
{
	vec3 secondRayPos = p;
	float secondRayDensity;
	float densitySum = 0.0;
	//secondRayDir = 100 * normalize(secondRayDir);
	secondRayDir = 10 * normalize(secondRayDir);

	//float beers = 1;
	//float powder = 1;
	for (int j = 0; j < 10; j++) // 40 can render most shadow
	{
		secondRayPos += (j+1) *secondRayDir;
		//secondRayPos += secondRayDir;
		vec3 secondRayWeather = texture(weather, secondRayPos.xz * weatherScale).xyz;
		//densitySum += density(secondRayPos, secondRayWeather, false, float(j));
		densitySum += densityScale * density(secondRayPos, secondRayWeather, false, float(j)) * (j+1) * length(secondRayDir);
		//float tempDensity = density(secondRayPos, secondRayWeather, false, float(j));
		//beers *= max(exp(-densityScale * tempDensity * length(secondRayDir)), 
		//				exp(-densityScale * tempDensity * length(secondRayDir) * 0.25 ) * 0.7);
		//powder *= 1.0 - exp(-densityScale * tempDensity * length(secondRayDir) * 2.0);
	}

	densitySum /= length(secondRayDir);

	float beers = max(exp(-densitySum * length(secondRayDir)), 
						exp(-densitySum * length(secondRayDir) * 0.25 ) * 0.7);  // this is from Nubis 2017 siggraph course
	float powder = 1.0 - exp(-densitySum * length(secondRayDir) * 2);
	//float powder = 1.0 - exp(-densityScale / debugValue * densitySum * length(secondRayDir));
	//powder = 1 - beers*beers;
	//float beersPowder = mix(mix(beers, 2 * beers * powder, pow(clamp(-dot(normalize(viewDir), normalize(secondRayDir)), 0, 1), 1)), beers, debugValue); // here times 2.0 to approximate a good effect (check GPU Pro 360 Guide to lighting)
	float beersPowder = mix(beers, 2 * beers * powder, pow(clamp(-dot(normalize(viewDir), normalize(secondRayDir)), 0, 1), 1));
	//float beersPowder = powder; // here times 2.0 to approximate a good effect (check GPU Pro 360 Guide to lighting)
	//float beersPowder2 = mix(beers, powder, debugValue); // we can add powder effect in the future
	//return beersPowder2;

	//float beersPowder = beers;
	//float beersPowder = powder;
	//return mix(beers, beersPowder, debugValue);
	return beersPowder;
}

vec4 march(const vec3 pos, const vec3 end, vec3 dir, const int numSamples) 
{
	vec3 p = pos;           // sample position
	float totalTrans = 1.0; // total tranparency
	float alpha = 0.0;      // alpha channel value (why not reuse tranparency)
	float stepDistance = length(dir); // step length?
	const float atomThickness = sky_t_radius - sky_b_radius; // sky thickness
	float secondRayDistance = atomThickness / float(numSamples);  // secondary ray-marching step length?
	vec3 secondRayDir = vSunDirection * stepDistance;          // direction towards sun? I think this is inverse, but when added -, the result looks strange
	vec3 L = vec3(0.0f); 
	//const float densityScale = 0.15; // scale the attenuation of cloud
	float densityScale = 0.15; // scale the attenuation of cloud 0.2
	float costheta = dot(normalize(secondRayDir), normalize(dir)); // the angle between view ray (first) and light ray (secondary)
	float phase = max(HG(costheta, 0.6), 1.4 * HG(costheta, 0.99 - 0.4)); // this is from Nubis 2017 siggraph course
	phase = mix(phase, HG(costheta, -0.5), 0.3); // when look along the sun light direction (from Frostbite 2016 talk)
	const float weatherScale = 0.00006; // original 0.00008 // 0.00005 is beautiful!!!
	//float weatherScale = 0.00001 * debugValue; // original 0.00008 // 0.00005 is beautiful!!!

	int counter = 0;

	for (int i = 0; i < numSamples; i++) // sample points until the max number
	{
		if (totalTrans < 0.001) break; // I do not know why, but if I use break or put it into for loop condition, artefact shows up
		p += dir; // move forward
		vec3 weatherSample = texture(weather, p.xz * weatherScale).xyz; // get weather information
		float viewRayDensity = densityScale * density(p, weatherSample, true, 0.0); // compute density
		float transmitance = exp(-viewRayDensity * stepDistance); 
		totalTrans *= transmitance;	

		if (viewRayDensity > 0.0)  //calculate lighting, but only when we are in a non-zero density point (when there is cloud)
		{ 
			counter++;
			float beers = getLight(p, secondRayDir, dir, weatherScale, densityScale);
			float heightFraction = GetHeightFractionForPoint(p.y);
			vec3 ambient = 15.0 * vAmbient * mix(0.15, 1.0, heightFraction);
			vec3 sunC = pow(vSunColor, vec3(0.75)); // sun color
			L += (ambient + 5 * sunC * beers * phase) * viewRayDensity * totalTrans * stepDistance;		// (did you counter the out-scattering rate?)
			alpha += (1.0 - transmitance) * (1.0 - alpha); // this is from Horizon zero dawn talk
		}
	}

	//if ((counter > 0 && counter < 10 && totalTrans > 0.6) || (counter > 0 && alpha < 0.05)) return vec4(L, pow(0.2, max(6 - counter, counter)));
	//if (alpha > 0 && alpha < 0.3) alpha = 1;
	return vec4(L, alpha);
}



void main()
{
	vec2 shift = vec2(floor(float(check) / downscale), mod(float(check), downscale));	
	vec2 uv = (gl_FragCoord.xy * downscale + shift.yx) / (resolution); // !: shift.yx, not .xy (why do not change the order in the previous) 
	uv = uv-vec2(0.5); // remapping to NDC
	uv *= 2.0; // remapping to NDC
	//uv.x *= aspect;  // after multiply aspect (may > 1), the uv may over -1 and 1 (exceed clip space), this may cause problems when calculating world space position
	vec4 uvdir = vec4(uv.xy, 1.0, 1.0);          // the z value can be from -1 to 1 (it seems all values are fine) 
	vec4 worldPos = invView * (invProj * uvdir); // the matrix shoule be seperated, if pass a whole MVP or inv MVP martrix, the precision problems will occur 
	vec3 dir = normalize(worldPos.xyz / worldPos.w - cameraPos);    // I do not sure divided by w is necessary or not, but with it is definitly right
	                                                                // it works without divided by w
																			
	vec4 col = vec4(0.0);
	vec3 camPos = vec3(0.0, g_radius, 0.0) + cameraPos;                     // camPos is for computing cloud, while cameraPos is the exact position of the game camera
	vec4 testVolume;
	// if it is higher than horizon
	//if (dir.y > 0.0) 
	//if(1 > 0)
	if (dir.y > 0.0 || (camPos.y >= sky_b_radius))
	{ 
		//vec3 start = camPos + dir * intersectSphere(camPos, dir, sky_b_radius); // the intersection with the bottom of the cloud layer
		//vec3 end = camPos + dir * intersectSphere(camPos, dir, sky_t_radius);   // the intersection with the top of the cloud layer
		
		vec2 cloudLayerInterscetion = intersectSphereMix(camPos, dir, sky_b_radius, sky_t_radius);
		vec3 start = camPos + dir * cloudLayerInterscetion.x;
		vec3 end = camPos + dir * cloudLayerInterscetion.y;
		const float t_dist = sky_t_radius - sky_b_radius;                       // the thickness of the cloud layer
		//float shelldist = min((length(end - start)), t_dist * 10);               // the exact thickness (the ray with the cloud layer, the larger angle, the thicker layer), set a max length
		float shelldist = length(end - start);
		//float steps = (mix(96.0, 54.0, dot(dir, vec3(0.0, 1.0, 0.0))));       // the larger angle, the thicker cloud layer, the more samples
		//float steps = (mix(128.0, 64.0, abs(dot(dir, vec3(0.0, 1.0, 0.0)))));        // the larger angle, the thicker cloud layer, the more samples
		float steps = (mix(highSampleNum, lowSampleNum, abs(dot(dir, vec3(0.0, 1.0, 0.0)))));        // the larger angle, the thicker cloud layer, the more samples
		//float dmod = smoothstep(0.0, 1.0, (shelldist / t_dist) / 14.0);         // these two lines are manipulating the length of every step (the parameter here, 14.0 and 4.0 are decided by the user) 
		//float s_dist = mix(t_dist, t_dist*4.0, dmod) / (steps);
		//float s_dist = mix(shelldist, shelldist*2.0, dmod) / (steps);
		float stepDistance = shelldist / steps;
		//vec3 raystep = dir * min(stepDistance, 40);                                            // the length and direction of every step
		vec3 raystep = dir * stepDistance;
		if (camPos.y >= sky_b_radius) raystep = dir * min(stepDistance, 30);
		float blueNoise = texture(blueNoise, gl_FragCoord.xy / 1024.0f).r;
		vec3 blueNoiseOffset = fract(blueNoise + float(time * 100) * c_goldenRatioConjugate) * dir * blueNoiseRate;  // use blue noise
		
		//vec3 lweather = texture(weather, vec2(0.1f, 0.1f)).xyz;
		vec4 volume = march(start + blueNoiseOffset, end, raystep, int(steps)); // ray-marching
		testVolume = volume;
		volume.xyz = U2Tone(volume.xyz)*cwhiteScale;                            // HDR Tone mapping
		//volume.xyz = volume.xyz / (volume.xyz + vec3(1.0));
		volume.xyz = sqrt(volume.xyz);
		vec3 background = vec3(1.0);
		background = preetham(dir);
		col = vec4(background * (1.0 - volume.a) + volume.xyz * volume.a, 1.0);
		//col = vec4(vec3(pow(background.x * (1.0 - volume.a) + volume.x * volume.a, 15)), 1.0);
		if (volume.a > 1.0) 
		{
			col = vec4(1.0, 0.0, 0.0, 1.0); // output error area
		} 
	} 
	// if it is lower than horizon, all grey
	else 
	{
		vec3 background = vec3(0.5);
		background = preetham(dir);
		col = vec4(background, 1.0);
		testVolume = col;
	}
	//color = testVolume;
	color = col;
}
