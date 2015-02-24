
varying vec3		pixel;
varying float		clipLength;

uniform float		nearClip;		// near clipping plane (in eye coordinate)
uniform vec3		origin;			// camera origin
uniform vec3		minDomain;		// grid min (usually 0,0,0)
uniform vec3		maxDomain;		// grid max

uniform int		tLevel;			// traversal level (0) for first

uniform float		maxDensity;
uniform sampler3D	densityTexture;

void main()
{
	
	vec3 ray = normalize(pixel - origin);
	vec3 inv_ray = vec3(1.0, 1.0, 1.0) / ray;
	
	// determine intersection with data box
	vec3 domain_t0 = (minDomain - origin) * inv_ray; 
	vec3 domain_t1 = (maxDomain - origin) * inv_ray;
	
	vec3 tmin = min(domain_t0, domain_t1);
	vec3 tmax = max(domain_t0, domain_t1);
	
	// max(naerClip
	float tenter = max( tmin.x, max(tmin.y, tmin.z));
	float texit  = min( tmax.x, min(tmax.y, tmax.z));
	
	if (tenter <= texit)
	{
		vec3 intersect = origin + ray * tenter;
		ivec3 macrocell = ivec3(intersect);

		// ray direction
		ivec3 di;
		di.x = ray.x > 0.0 ? 1 : -1;
		di.y = ray.y > 0.0 ? 1 : -1;
		di.z = ray.z > 0.0 ? 1 : -1;

		vec3 dtd = vpm * abs(inv_ray);
		
		// traverse the macrocell strcuture, until exiting the ray
		for (int i = 0; i < tLevel; i++)
		{
			vec3 tnext_xyz = tenter + (vec3(macrocell) * vpm - intersect) * inv_ray;
			float tnext = min(tnext_xyz.x, min(tnext_xyz.y, tnext_xyz.z));
			
			if (tnext >= texit)
			{
				discard;
			}
			
			if (tnext == tnext_xyz.x)
			{
				macrocell.x += di.x;
				tnext_xyz.x += dtd.x;

			}
			else if (tnext == tnext_xyz.y)
			{
				macrocell.y += di.y;
				tnext_xyz.y += dtd.y;

			}
			else
			{
				macrocell.z += di.z;
				tnext_xyz.z += dtd.z;
			}
		}
		
		// shade it
		//float sum = float(macrocell.x) + float(macrocell.y);
		/*
		if (fract(intersect.x) < 0.1 || fract(intersect.y) < 0.1)
		{
			gl_FragColor = vec4(0.0, 0.0, 1.0, 1.0);
		}
		else*/
		{
			float d = texture3D(densityTexture, vec3(macrocell) / maxDomain).r / maxDensity;
			gl_FragColor =
				vec4(float(macrocell.x) / maxDomain.x, float(macrocell.y) / maxDomain.y, 0.0 / maxDomain.z, 1.0) +
				0.001 * vec4(1.0, 1.0 - d, 0.0, 0.6);
			
		}
		
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
		discard;
	}
}
