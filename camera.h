/* -----------------------------------------------
 * Super nanovol
 * Khairi Reda, 2013
 * Electronic Visualization Laboratory
 * www.evl.uic.edu/kreda
 * 
 * GPL v2 License:
 * http://www.gnu.org/licenses/gpl-2.0.html
 * camera.h
 *
 * -----------------------------------------------
 */
 
#ifndef _SUPER_NANOVOL_CAMREA_H___
#define _SUPER_NANOVOL_CAMREA_H___

#include <string>
#ifdef DO_RIFT
#include <OVR.h>
#endif
#include "VectorT.hxx"
#include "Matrix44.hxx"

static const double OVR_METERS_2_GRID = 65.0;		// used in OVR


// camera parameters
struct Camera
{
	static Camera & getInstance();
	Camera();
	
	
	vec3		position;
	vec3		lookAt;
	vec3		up;
	vec3		u, v, w;
	
#ifdef DO_RIFT
	// additional camera transform 
	OVR::Matrix4f	cameraTransform;
#endif

	// projection plane (camera coordinates)
	float		left, right, bottom, top;
	float		nearClip;
	float		farClip;	

	// projection plane (world coordinates)
	vec3		bL, bR, tL, tR;
	
	void updateProjection();
	void uvw();
	void rotateX(float);
	void rotateY(float);
	void rotateZ(float);
	void rotateU(float);
	
	// load / save
	bool loadCamera(std::string filename);
	bool saveCamera(std::string filename) const;
	
	void moveCamera(vec3f delta);
	
	// serialize camera
	void marshal(unsigned char *);
	void unmarshal(const unsigned char *);
	
	// camera to world
	vec3 cameraToWorld(vec2);

	static const int MARSHAL_SIZE = sizeof(vec3);

private:
	static Camera * instance;
};

#endif

