
uniform vec3	origin;
uniform vec3	maxDomain;
uniform vec3	pixel;
uniform int	tLevel;

void main()
{
	vec3 ray = normalize(pixel - origin);
	vec3 inv_ray = vec3(1.0, 1.0, 1.0) / ray;
	
	// determine intersection with data box
	vec3 domain_t0 = (          - origin) * inv_ray; 
	vec3 domain_t1 = (maxDomain - origin) * inv_ray;
	
	vec3 tmin = min(domain_t0, domain_t1);
	vec3 tmax = max(domain_t0, domain_t1);
	
	//max(nearClip
	float tenter = max(.125, max(tmin.x, max(tmin.y, tmin.z)));
	float texit  = min(tmax.x, min(tmax.y, tmax.z));
	ivec3 rayDir = ivec3( greaterThan(ray, vec3(0)) ) - ivec3( lessThan(ray, vec3(0)) );
	
	
	if (tenter <= texit)
	{
		//tenter += RAY_INCREMENT;
		//ivec3 macrocell;
		vec3 intersect = origin + ray * tenter;
		ivec3 macrocell = ivec3(intersect);

		// traverse the macrocell strcuture until we exit the box
		for (int i = 0; i < tLevel ; i++)
		{
			ivec3 nextCell = ( (rayDir + 1) >> 1 ) + macrocell;
			vec3 tMax = tenter + ( vec3(nextCell) - intersect ) * inv_ray;
			float tnext = min(tMax.x, min(tMax.y, tMax.z));
						
			// move macrocell
			macrocell += ivec3(equal(vec3(tnext, tnext, tnext), tMax)) * rayDir;
			
			if ( any(greaterThan(macrocell, ivec3(maxDomain))) || any(lessThan(macrocell, ivec3(0))) )
			{
				// we are out of box
				break;
			}
		}
		
		gl_FragColor = vec4( vec3(macrocell), 1.0 );
	}
	else
	{
		gl_FragColor = vec4(-1.0);
	}
	
}


