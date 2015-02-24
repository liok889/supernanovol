
// GLSL header passed from main program

/* ------------------------------------------------------------------------------
 * CONSTANTS (passed from main program)
 * vec3 maxDomain:			size of macrocell grid - 1
 * ivec3 imaxDomain:			same as above except of ivec3
 * float atomScale:			scale * vpa
 * int atomsSqrt:			square root of number of atoms (ceiled)
 * int indicesSqrt:			square root of number of indices (ceiled)
 * vec4 background:			background color
 *
 * Additional consts when VOLUME_RENDER
 * float voxelScale			scale of charge density volume voxel relative 
 					to a macrocell (vpa / vvpa)
 * float maxVolumeValue		maximum charge density in the volume
 * vec3 maxVDomain			same as above but + 1	
 * vec3 inv_maxVDomain			1/maxVDomain
 * float volumeScale			inverse scale of charge density voxels rel to macrocells
 * float densityScale			scales volume charge value
 * float densityOffset			offsets volume charge value 
 *
 * DEFINED macros
 * PRECISE_GEOMETRY			whether we are doing precise geom 
 *					by not exiting too early
 * GEOMETRY_PRECISION			precision level (starts from 0)
 * ------------------------------------------------------------------------------
 */

// more constants
const float	MAX_T			= 9999999.0;
const vec3 	LIGHT 			= vec3(-0.7, 0.0, -0.7);
const float	SPECULAR_EXPONENT	= 20.0;
const vec3	SPECULAR_COLOR		= vec3(.9);

// atom type data ( vec3(color), Van der Waals radius )
const vec4 ATOM_DATA[5] = vec4[5]
(
	vec4(.988, .25, .25, 1.52),		// O
	vec4(.12, .257, 1, 2.1),		// Si
	vec4(.6, .6, .6, 3.1),			// Al
	vec4(.0, .0, .0, 2.1),			// Al in the background
	vec4(.6, .6, .6, 1.7)			// Carbon
);

// varyings
varying vec3			pixel;

// uniforms
uniform float			nearClip;		// near clipping plane (in eye coordinate)
uniform vec3			origin;			// camera origin
uniform int			tLevel;			// traversal level (0) for first

#ifdef CLIP_BOX
uniform vec3			clipBoxMin;
uniform vec3			clipBoxMax;
#endif

// Macrocells structure
uniform sampler3D		macrocells;

// indices (for indexed ref to atoms)
#ifndef DIRECT_ATOMS_REF
uniform sampler2D		indices;
#endif

// actual atom data
uniform sampler2D		atoms;

// volume data
#ifdef VOLUME_RENDER
uniform sampler3D		volume;
uniform sampler2D		tf;
uniform float			dT;			// amount to step through the volume
uniform float			colorScale;		// how much to scale opacity by

// density color accumilated so far
vec4 volumeColor		= vec4(0.0);

float sampleVolume(vec3 p)
{
	return texture3D(volume, p * inv_maxVDomain).x * densityScale + densityOffset;
}

void dvr_sample(float v)
{	
	vec4 colorSample = texture2D(tf, vec2(v / maxVolumeValue, 0.0));
	//vec4 colorSample = vec4(1.0, 0.0, 0.0, .5 * (v / maxVolumeValue));
	float alpha = 1.0 -exp(-colorSample.w * colorScale * dT * volumeScale );

	/*	
	#if (defined LAMBERT || defined PHONG)
		if (color_sample.w > light_threshold)
			color_sample.xyz = shade(color_sample.xyz, pcurr, gradient(pcurr, fcurr));
	#endif
	*/
	
	float alpha_1msa = alpha * (1.0 - volumeColor.w);
	volumeColor.xyz += colorSample.xyz * alpha_1msa;
	volumeColor.w += alpha_1msa;
}
#endif

#ifndef DIRECT_ATOMS_REF
int getIndex(int idx)
{
	int r = idx / indicesSqrt;
	return int( texelFetch2D(indices, ivec2(idx - r*indicesSqrt, r), 0).x );
}
#endif

vec4 getAtom(int idx)
{
	int r = idx / atomsSqrt;
	return texelFetch2D(atoms, ivec2(idx - r*atomsSqrt, r), 0);
}

bool getMacrocell(ivec3 macrocell, out int ballStart, out int ballEnd)
{
	vec3 cell = texelFetch3D(macrocells, macrocell, 0).xyz;
	ballStart = int(cell.x);
	ballEnd = int(cell.y) + ballStart;
	
	return ballStart != ballEnd;
}

vec3 specular(vec3 n, vec3 r)
{
	return pow( max(dot(r, reflect(LIGHT, n)), 0.0), SPECULAR_EXPONENT ) * SPECULAR_COLOR;	
}

vec3 shadePhong(vec3 n, vec3 r, vec3 diffuse)
{
	return diffuse * max(dot(n, r), 0.0)
		 + specular(n, r);
}


vec3 ray, inv_ray;			// ray and 1/ray
float tenter, texit, _nearClip;	// T of entery and exit points into the data box

// computes basic view parameters
bool compute_view()
{
	ray = normalize(pixel - origin);
	inv_ray = vec3(1.0, 1.0, 1.0) / ray;
	_nearClip = length(pixel - origin);	

	// determine intersection with data box
#ifdef CLIP_BOX
	vec3 domain_t0 = (clipBoxMin - origin) * inv_ray; 
	vec3 domain_t1 = (clipBoxMax - origin) * inv_ray;	
#else
	vec3 domain_t0 = (          - origin) * inv_ray; 
	vec3 domain_t1 = (maxDomain - origin) * inv_ray;
#endif

	vec3 tmin = min(domain_t0, domain_t1);
	vec3 tmax = max(domain_t0, domain_t1);	

	texit  = min(tmax.x, min(tmax.y, tmax.z));
	tenter = max(.125, max(tmin.x, max(tmin.y, tmin.z)));
	
	// returns true if we are outside the box
	return (tenter > texit);	
}

void main()
{	
	// set color tto background
	gl_FragColor = background;

	// compute basic view parameters
	if (compute_view())
	{
		// outside of data box
		return;
	}
	
	// compute ray stepping direction 
	// (+1, 0, or -1 for each component depending on sign of ray components
	ivec3 rayDir = ivec3( greaterThanEqual(ray, vec3(0)) ) - ivec3( lessThan(ray, vec3(0)) );
	ivec3 rayDirStep = (rayDir + 1) >> 1;

	vec3 intersect = origin + ray * tenter;
	ivec3 macrocell = ivec3(intersect);
		
	int hasBallIntersect = 0;
	#ifdef PRECISE_GEOMETRY
		// how many steps through the macrocell have 
		// we taken since we had an intersection
		// (used for determining early termination)
		int stepsSinceIntersect = 0;
	#endif

	// T of cloest geometry intersection point
	float T = MAX_T;

	// information about the closest atom we have encountered so far
	vec3 closestAtom;
	vec3 closestAtomColor;

	#ifdef VOLUME_RENDER
		float tvnext = tenter;
	#endif
	
	// traverse the macrocell strcuture (until we exit the box/clip box)
	for (
	#ifdef T_LEVEL
		int i = 0; i < tLevel ; i++
	#else
		;;
	#endif
	)
	{	
		int b, count;
		bool dontSkip = getMacrocell(macrocell, b, count);
	#ifdef SKIP_EMPTY
		if (dontSkip)
	#endif
		{	
		#ifdef BALLS_RENDER

			for (; b < count; b++)
			{		
			#ifdef DIRECT_ATOMS_REF
				vec4 atom = getAtom(b);
			#else
				vec4 atom = getAtom( getIndex(b) );
			#endif
				
				// get atom type / information
				vec4 atomType = ATOM_DATA[ int(atom.w) ];
				atomType.w *= atomScale;
				atomType.w *= atomType.w;
	
				// test ray-sphere intersection
				vec3 L = atom.xyz - origin;
				float Tca = dot(L, ray);
				float dd = dot(L, L) - Tca*Tca;			
				if (Tca >= 0 && dd <= atomType.w)
				{
					float newT = Tca - sqrt(atomType.w - dd);
					if (newT < T) // && newT > _nearClip)
					{
						closestAtom = atom.xyz;
						closestAtomColor = atomType.xyz;
						T = newT;
						hasBallIntersect = 1;
						gl_FragColor = vec4(shadePhong( normalize(origin + T*ray - closestAtom), -ray, closestAtomColor), 1.0);

					}
				}
			}
			#ifdef PRECISE_GEOMETRY
				stepsSinceIntersect += hasBallIntersect;
			#endif
	
		#ifndef VOLUME_RENDER
			if ( 
				#ifdef PRECISE_GEOMETRY
					stepsSinceIntersect > GEOMETRY_PRECISION
				#else
					hasBallIntersect > 0
				#endif
			)
			{
				// we have sphere-ray intersection, shade it and exit
				//gl_FragColor = vec4(shadePhong( normalize(origin + T*ray - closestAtom), -ray, closestAtomColor), 1.0);
				break;
			}
		#endif  // if not VOLUME_RENDER
		#endif	// if RENDER_BALLS

		}
		
		{
			// figure out next macrocell and tnext
			vec3 tMax = tenter + ( vec3(rayDirStep + macrocell) - intersect ) * inv_ray;
			float tnext = min(tMax.x, min(tMax.y, tMax.z));

			// move to next macrocell
			macrocell += ivec3(equal(vec3(tnext), tMax)) * rayDir;
			intersect = origin + ray * tnext;
			tenter = tnext;
			
		#ifdef VOLUME_RENDER
			// do volume rendering
		#ifdef SKIP_EMPTY
			if (dontSkip)
		#endif
			{
				float tvstop = hasBallIntersect > 0 ? T : tnext;
				for (;;)
				{
					dvr_sample( sampleVolume(origin + ray * tvnext) );
					tvnext += dT;
					if (tvnext >= tvstop || volumeColor.a >= 1.0)
					{
						// we have exited this macrocell,
						// or we are past a ball intersection. No need to continue
						break;
					}	
				}
			}
			
		#endif
			if (	// out of box? 
				any(greaterThan(macrocell, imaxDomain)) || any(lessThan(macrocell, ivec3(0)))
				
				// opacity termination / bit balls termination
				#ifdef VOLUME_RENDER
					|| volumeColor.a >= 1.0
				#ifdef BALLS_RENDER
					#ifdef PRECISE_GEOMETRY
						|| stepsSinceIntersect > GEOMETRY_PRECISION
					#else
						|| hasBallIntersect > 0
					#endif
					
				#endif
				#endif
			)
			{
				#ifdef VOLUME_RENDER
				#ifdef BALLS_RENDER
					if (hasBallIntersect > 0) 
					{
						// integrate ball color
						volumeColor.xyz += (1.0 - volumeColor.a) * shadePhong( normalize(origin + T*ray - closestAtom), -ray, closestAtomColor);
						volumeColor.a = 1.0;
					}
				#endif
					gl_FragColor = volumeColor;
				#endif
				break;
			}
			
			// OR, another way to do the exit test is
			/* 
			if (tenter > texit)
				break;
			*/
		}
	}
#if (defined T_LEVEL && defined VOLUME_RENDER)
	gl_FragColor = volumeColor;
#endif
}

