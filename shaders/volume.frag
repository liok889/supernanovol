
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
 * ------------------------------------------------------------------------------
 */
 
 // varyings
varying vec3			pixel;

// uniforms
uniform float			nearClip;		// near clipping plane (in eye coordinate)
uniform vec3			origin;			// camera origin

uniform sampler3D		volume;
uniform sampler2D		tf;
uniform float			dT;			// amount to step through the volume
uniform float			colorScale;		// how much to scale opacity by

#ifdef RAY_DIFFERENTIALS
uniform vec2			half_pixel_uv;
uniform vec3			camera_u;
uniform vec3			camera_v;
uniform float			ray_diff

#endif

// volume color accumilated so far
vec4 volumeColor			= vec4(0.0);

#ifdef VOLUME_RENDER
float sampleVolume(vec3 p)
{
	return texture3D(volume, p * inv_maxVDomain).x;
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



vec3 unnnormalized_ray, ray, inv_ray;		// ray and 1/ray
float tenter, texit;				// T of entery and exit points into the data box


// computes basic view parameters
bool compute_view()
{
	unnormalized_ray = pixel - origin;
	ray = normalize(unnormalized_ray);
	inv_ray = vec3(1.0, 1.0, 1.0) / ray;	

	// determine intersection with data box
	vec3 domain_t0 = (          - origin) * inv_ray; 
	vec3 domain_t1 = (maxDomain - origin) * inv_ray;
	
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
	
#ifdef RAY_DIFFERENTIALS

	//actual ray differential dD/dx according to paper
	float udud = dot(unnormalized_ray_dir, unnormalized_ray_dir);
	float unnormalized_ray_dir_np3o2 = pow(udud, 1.5);
	vec3 dDdx = (udud*camera_u - dot(unnormalized_ray_dir, camera_u)*unnormalized_ray_dir) / unnormalized_ray_dir_np3o2;
	float ldDdx = length(dDdx);
		
	float ceil_x = max(floor((sqrt(rdb * rdb + 2.0 * rda * tcurr) - rdb) * inv_rda), 0.0);
	
	//this is the t_x
	ps1.w = max(0.5*rda*ceil_x*ceil_x + rdb*ceil_x - (1.0 / 65536.0), ps1.w);
	//ps1.w = max(0.5*rda*rda*ceil_x*ceil_x + rda*ceil_x - (1.0 / 65536.0), ps1.w);
	
	float dt = sqrt(rdb*rdb + 2.0 * rda * ps1.w);
	//float dt = sqrt(rda*rda + 2.0 * rda * rda * ps1.w);
	float frda = M*(M + 1.0)*.5*rda;
	float fdt = M*dt + frda;
      
#endif

	
	
	float tvnext = tenter;
	for (int i = 0; i < 4024; i++)
	{
		#ifdef VOLUME_RENDER
		dvr_sample( sampleVolume(origin + ray * tvnext) );
		#endif
		tvnext += dT;
		
		if (tvnext >= texit || volumeColor.a >= 1.0)
		{
			// we have exited this macrocell,
			// or we are past a ball intersection. No need to continue
			break;
		}
		
	}
	gl_FragColor = volumeColor;
}

