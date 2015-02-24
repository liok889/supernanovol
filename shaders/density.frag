
varying vec3		pixel;

uniform float		nearClip;		// near clipping plane (in eye coordinate)
uniform vec3		origin;			// camera origin
uniform vec3		maxDomain;		// grid max
uniform int		tLevel;			// traversal level (0) for first

// background color
uniform vec4		background;

// data textures
uniform float		maxDensity;
uniform sampler3D	densityTexture;
uniform int		modifier;


#define RAY_INCREMENT 	0.0015
void main()
{
	
	vec3 ray = normalize(pixel - origin);
	vec3 inv_ray = vec3(1.0, 1.0, 1.0) / ray;
	
	// determine intersection with data box
	vec3 diff = maxDomain - origin + vec3(0.2);
	
	vec3 domain_t0 = (          - origin) * inv_ray; 
	vec3 domain_t1 = diff / ray;
	
	vec3 tmin = min(domain_t0, domain_t1);
	vec3 tmax = max(domain_t0, domain_t1);
	
	// max(naerClip
	float tenter = max(0.0, max(tmin.x, max(tmin.y, tmin.z)));
	float texit  = min( tmax.x, min(tmax.y, tmax.z));
	
	//ivec3 rayDir = ivec3( greaterThan(ray, vec3(0)) ) - ivec3( lessThan(ray, vec3(0)) );
	ivec3 rayDir;
	rayDir.x = ray.x >= 0.0 ? 1 : -1;
	rayDir.y = ray.y >= 0.0 ? 1 : -1;
	rayDir.z = ray.z >= 0.0 ? 1 : -1;
	

	
	ivec3 stepper = ivec3(0);
	if (tenter <= texit)
	{
		vec3 intersect = origin + ray * tenter;
		float nextT = tenter;
		ivec3 macrocell = ivec3(intersect);
		vec3 tMax;
		ivec3 prevMacrocell = macrocell;
		float tnext;
		// traverse the macrocell strcuture, until exiting the ray
		for (int i = 0; i < tLevel; i++)
		{
			/*
			vec3 intersect = origin + ray * nextT;
			macrocell = ivec3(intersect);
	
			domain_t0 = (vec3(macrocell) - origin) * inv_ray;
			domain_t1 = (vec3(macrocell+ivec3(1)) - origin) * inv_ray;
			//tmin = min(domain_t0, domain_t1);
			tmax = max(domain_t0, domain_t1);
				
			// step one time
			nextT =  min( tmax.x, min(tmax.y, tmax.z)) + RAY_INCREMENT;
			if (nextT > texit) {
				discard;
			}
			*/
						
			tMax = (vec3(macrocell + ((rayDir+1) >> 1)) - intersect) * inv_ray;
			tnext = min(tMax.x, min(tMax.y, tMax.z));
			
			if (tMax.x < 0.0 || tMax.y < 0.0 || tMax.z < 0) {
				gl_FragColor = vec4(1.0);
				return;
			}
			
			prevMacrocell = macrocell;
			
			stepper = ivec3(0);
			if (tnext == tMax.x) 
			{
				macrocell.x += rayDir.x;
				stepper.x = rayDir.x;
			} 
			else if (tnext == tMax.y) 
			{
				macrocell.y += rayDir.y;
				stepper.y = rayDir.y;
			} 
			else
			{
				
				macrocell.z += rayDir.z;
				stepper.z = rayDir.z;
			}
			
			//stepper = ivec3(equal(vec3(tnext, tnext, tnext), tMax)) * rayDir;
			//macrocell += stepper;
			
			intersect = origin + ray * (tnext + tenter);
			
			
			if ( any(greaterThan(macrocell, ivec3(maxDomain))) || any(lessThan(macrocell, ivec3(0,0,0))) )
			{
				// we are out of box
				gl_FragColor = background;
				return;
			}
			
				
		}
		
		float d = texture3D(densityTexture, vec3(macrocell) / maxDomain).r / maxDensity;
		
		if (modifier == 1 || modifier == 2)
		{
			if (tLevel == 0)
			{
				int tot = macrocell.x + macrocell.y + macrocell.z;
				gl_FragColor = (tot % 2 == 0 ? vec4(1,0,0,1) : vec4(0,0,1,1));
			}
			else
			{
					
				//gl_FragColor = vec4(vec3(tnext / 5.0), 1.0);
				gl_FragColor = vec4(abs(vec3(stepper)), 1.0);
			}
		}
		else if (modifier == 0)
		{
			int tot = prevMacrocell.x + prevMacrocell.y + prevMacrocell.z;
			gl_FragColor = (tot % 2 == 0 ? vec4(1,0,0,1) : vec4(0,0,1,1));
			//gl_FragColor = vec4(tMax / 30.0, 1.0);
			//gl_FragColor = vec4( vec3( (tenter / 350.0)), 1.0 );
		}
		
		
		//gl_FragColor = 0.002 * vec4(1.0, 1.0 - d, 0.0, 0.6) + 1.0 *
			
			//vec4(vec3(macrocell) / maxDomain, 1.0);
			//(macrocell.z % 2 > 0 ? vec4(1.0, 0.0, 0.0, 1.0) : vec4(0.0, 0.0, 1.0, 1.0));
			/*((macrocell.x + macrocell.y + macrocell.z) % 2 > 0 ?
			vec4(1.0, 0.0, 0.0, 1.0) :
			vec4(0.0, 0.0, 1.0, 1.0));*/
			
		
		/*
		if (mod(sum, 2.0) > 0.0)
		{
			gl_FragColor = vec4(1.0, intersect.z / maxDomain.z, 0.0, 0.6);
		}
		else
		{
			gl_FragColor = vec4(0.0, intersect.z / maxDomain.z, 1.0, 0.6);
		}
		*/
	}
	else
	{
		gl_FragColor = background;
	}
}
