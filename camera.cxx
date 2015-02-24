/* -----------------------------------------------
 * Super nanovol
 * Khairi Reda, 2013
 * Electronic Visualization Laboratory
 * www.evl.uic.edu/kreda
 * 
 * GPL v2 License:
 * http://www.gnu.org/licenses/gpl-2.0.html
 * camera.cxx
 *
 * -----------------------------------------------
 */
#include <fstream>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "Matrix44.hxx"
#include "camera.h"
#ifdef DO_RIFT
#include <OVR.h>
#include "rift.h"
using namespace OVR;
#endif
using namespace std;

static const double DEGREE_TO_RAD = 0.0174532925;

Camera::Camera()
{
	static bool OVRinited = false;
	
	up.x() = 0.0;
	up.y() = 1.0;
	up.z() = 0.0;


#ifdef DO_RIFT
	const ovrRender & ovr = ovrRender::getInstance();
	
	// figure out projection parameters by reading
	// screen dimensions from OVR 
	const float eyeW = ovr.screenDim.x() *.5f;
	const float eyeH = ovr.screenDim.y();
	
	left	= -eyeW * .5f;
	right	= +eyeW * .5f;
	bottom	= -eyeH * .5f;
	top	= +eyeH * .5f;
	nearClip = ovr.eyeToScreenDistance;
	updateProjection();	
#endif
}

bool Camera::saveCamera(string fileName) const
{
	ofstream output(fileName.c_str(), ios::binary);
	if (!output.is_open()) {
		return false;
	}
	else
	{
		output.write((const char*) this, sizeof(Camera));
		output.flush();
		output.close();
		return true;
	}
}

bool Camera::loadCamera(string fileName)
{
	ifstream input(fileName.c_str(), ios::binary);
	if (!input.is_open()) 
	{
		return false;
	}
	else
	{
		input.read((char*)this, sizeof(Camera));
		input.close();
		return true;
	}
}



void Camera::updateProjection()
{
	vec3 mid = position -nearClip * w;
	
	bL = mid + left*u + bottom*v;
	bR = mid + right*u + bottom*v;
	tL = mid + left*u + top*v;
	tR = mid + right*u + top*v;
	
}

void Camera::uvw()
{
	// our UVW is a right handed system
	#ifdef DO_RIFT
			
		// read from sensor directly
		const OVR::Matrix4f & hmdMat = ovrRender::getInstance().getMatrix();	
		
		// additional transforms
		Matrix4f composedTransform = cameraTransform * hmdMat;

		Vector3f U = composedTransform.Transform(Vector3f(1.0, 0.0, 0.0));
		Vector3f V = composedTransform.Transform(Vector3f(0.0, 1.0, 0.0));
		Vector3f W = composedTransform.Transform(Vector3f(0.0, 0.0, 1.0));
		
		u.x() = U.x;
		u.y() = U.y;
		u.z() = U.z;
		
		v.x() = V.x;
		v.y() = V.y;
		v.z() = V.z;
		
		w.x() = W.x;
		w.y() = W.y;
		w.z() = W.z;		
	#else
		
		// determine from lookAt and current position
		w = this->lookAt - this->position;
		w *= -1.0;
		w.normalize();
		
		u = cross( up, w );
		v = cross( w, u );
		
		u.normalize();
		v.normalize();
	#endif

	// also calculate the location of the projection plane in world coordinates
	// so we can use this information in the ray casting shader
	updateProjection();
}

vec3 Camera::cameraToWorld(vec2 p)
{
	return bL +
		u * (p.x() * fabs(right - left)) +
		v * (p.y() * fabs(top - bottom));
}

void Camera::rotateZ( float degrees )
{
	float radians = degrees * DEGREE_TO_RAD;
	Matrix44 rotMat;
	rotMat.Zrotation( radians );
	
	vec3f newPos, curPos(position);
	curPos -= lookAt;
	Matrix44::transform(newPos, rotMat, curPos);
	position = newPos + lookAt;

	#ifdef DO_RIFT
		cameraTransform = Matrix4f::RotationZ(-radians) * cameraTransform;
	#endif

}


void Camera::rotateY( float degrees )
{
	float radians = degrees * DEGREE_TO_RAD;
	Matrix44 rotMat;
	rotMat.Yrotation( radians );
	
	
	vec3f newPos, curPos(position);
	curPos -= lookAt;
	Matrix44::transform(newPos, rotMat, curPos);
	position = newPos + lookAt;

	#ifdef DO_RIFT
		// additional camera transform
		cameraTransform = Matrix4f::RotationY(-radians) * cameraTransform;
	#endif
}

void Camera::rotateX( float degrees )
{	
	float radians = degrees * DEGREE_TO_RAD;
	Matrix44 rotMat;
	rotMat.Xrotation( radians );
	
	vec3f newPos, curPos(position);
	curPos -= lookAt;
	Matrix44::transform(newPos, rotMat, curPos);
	position = newPos + lookAt;
	#ifdef DO_RIFT
		cameraTransform = Matrix4f::RotationX(-radians) * cameraTransform;
	#endif
}

void Camera::rotateU( float degrees )
{
	float radians = degrees * DEGREE_TO_RAD;
	Matrix44 rotMat;
	rotMat.rotation(u, radians );
	
	vec3f newPos, curPos(position);
	curPos -= lookAt;
	Matrix44::transform(newPos, rotMat, curPos);
	position = newPos + lookAt;	
	#ifdef DO_RIFT
		// OVR::Matrix4f doesn't have rotation about arbitrary axis!
		Matrix44 rotMatMinus;
		Matrix4f ovrURot;
		rotMatMinus.rotation(u, -radians);
		memcpy(ovrURot.M, rotMatMinus.m, sizeof(float)*16);
		cameraTransform = ovrURot.Transposed() * cameraTransform;
	#endif
}

void Camera::moveCamera( vec3f delta )
{
	position += delta.x() * u + delta.y() * v;
	lookAt += delta.x() * u + delta.y() * v;
}

void Camera::marshal(unsigned char * p)
{
	memcpy(p, &position, sizeof(vec3));
}

void Camera::unmarshal(const unsigned char * p)
{
	memcpy(&position, p, sizeof(vec3));
}

Camera & Camera::getInstance()
{
	if (!instance) {
		instance = new Camera;
	}
	return *instance;
}

Camera * Camera::instance = NULL; 

