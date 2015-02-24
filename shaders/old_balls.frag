#version 120
#extension GL_EXT_gpu_shader4 : enable


varying vec3			pixel;
varying float			clipLength;

uniform float			nearClip;		// near clipping plane (in eye coordinate)
uniform vec3			origin;			// camera origin
uniform vec3			minDomain;		// grid min (usually 0,0,0)
uniform vec3			maxDomain;		// grid max

uniform float			vpm;			// voxel per macrocell
uniform int			tLevel;			// traversal level (0) for first

uniform float			maxMCDensity;
uniform sampler3D		macrocells;

uniform sampler2D		indices;
uniform int			indicesSqrt;

uniform sampler2D		atoms;
uniform int			atomsSqrt;

#define MAX_INDEX 12000000.0

int getIndex(int idx)
{
	int r = idx / indicesSqrt;
	return int( texelFetch2D(indices, ivec2(idx - r*indicesSqrt, r), 0).x );
}

float getFloatIndex(int idx)
{
	int r = idx / indicesSqrt;
	return texelFetch2D(indices, ivec2(idx - r*indicesSqrt, r), 0).x;
}

vec4 getAtom(int idx)
{
	int r = idx / atomsSqrt;
	return texelFetch2D(atoms, ivec2(idx - r*atomsSqrt, r), 0);
}

#define BALL_RADIUS 100.05625
void main()
{
	
	vec3 ray = normalize(pixel - origin);
	vec3 inv_ray = vec3(1.0, 1.0, 1.0) / ray;
	
	// determine intersection with data box
	vec3 domain_t0 = (minDomain - origin) * inv_ray; 
	vec3 domain_t1 = (maxDomain - origin) * inv_ray;
	
	vec3 tmin = min(domain_t0, domain_t1);
	vec3 tmax = max(domain_t0, domain_t1);
	
	//max(nearClip
	float tenter = max(tmin.x, max(tmin.y, tmin.z));
	float texit  = min( tmax.x, min(tmax.y, tmax.z));
	
	if (tenter <= texit)
	{
		vec3 intersect = origin + ray * tenter;
		ivec3 macrocell = ivec3(intersect * (1.0 / vpm));
		
		// ray direction
		ivec3 di;
		di.x = ray.x > 0.0 ? 1 : -1;
		di.y = ray.y > 0.0 ? 1 : -1;
		di.z = ray.z > 0.0 ? 1 : -1;

		vec3 dtd = vpm * abs(inv_ray);
		
		// traverse the macrocell strcuture, until exiting the ray
		for (int i = 0; i < tLevel; i++)
		{
			vec3 tnext_xyz = max(0.0, tenter) + (vec3(macrocell) * vpm - intersect) * inv_ray;
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
		
		// shade it according to density
		vec4 cell = texelFetch3D(macrocells, macrocell, 0);
		
		int start = int(cell.x);
		int count = int(cell.y);
		//float T = 999999.9;
	
		for (int b = 0; b < count; b++)
		{
			//int offset = (b + int(gl_FragCoord.x) * 4) % count;
			
			vec4 atom = getAtom( getIndex(b+start) );
			
			vec3 L = atom.xyz - origin;
			float Tca = dot(L, ray);
			float dd = dot(L, L) - Tca*Tca;

			if (Tca > 0 && dd <= atom.w*4.0)
			{
				//T = min(T, Tca - sqrt(BALL_RADIUS - dd));
				gl_FragColor = vec4(0.0, 0.0, 0.0, 1.0);
				return;
			}
		}

		gl_FragColor = vec4(1.0, 1.0 - getFloatIndex(start+count) / MAX_INDEX, 0.0, 0.6);
		
		/*
		//gl_FragColor = vec4(1.0, 1.0 - float(cell.z) / maxMCDensity, intersect.z / maxDomain.z, 0.6);
		if (T < 999999.9)
			gl_FragColor = vec4(1.0, 0.0, 0.0, 1.0);
		else
			discard;
		*/
	}
	else
	{
		discard;
	}
}
