/* -----------------------------------------------
 * Super nanovol
 * Khairi Reda, 2013
 * Electronic Visualization Laboratory
 * www.evl.uic.edu/kreda
 * 
 * GPL v2 License:
 * http://www.gnu.org/licenses/gpl-2.0.html
 * rift.h
 * Support class for Oculus Rift
 * -----------------------------------------------
 */

 
#if !defined(_SUPER_NANOVOL_OVR___) && defined(DO_RIFT)
#define _SUPER_NANOVOL_OVR___

#include <OVR.h>
#include "graphics/graphics.h"
#include "graphics/framebuffer.h"
#include "graphics/shader.h"

class ovrRender
{
public:
	static ovrRender & getInstance();
	void update();
	void draw();
	
	// get / set IPD
	void setIPD(float x) { IPD = x; }
	float getIPD() const { return IPD; }
	const OVR::Matrix4f & getMatrix() const { return hmdMat; }
private:
	// private constructor
	ovrRender();
	void init();
	
	// distortion function
	void distortScene(int VPx, int VPy, int VPw, int VPh, float);
	
	// interpupilary distance
	float IPD;
	
	// framebuffers
	Framebuffer * fb;
	
	// distortion shader
	Shader distortionShader;
	
	// static instance
	static ovrRender * instance;
	
	private:

	//OVR::System sys;
	OVR::Ptr<OVR::DeviceManager>		pManager;
	OVR::Ptr<OVR::HMDDevice>		pHMD;
	OVR::Ptr<OVR::SensorDevice>		pSensor;
	
	OVR::SensorFusion * SFusion;		// sensor complex
	OVR::Matrix4f hmdMat;			// head mounted display transform
	float lastSensorYaw;			// last known yaw from sensor

public:	
	// other OVR information
	string monitorName;
	float distortionK[4];
	float lensSeperationDistance;
	float lensCenterOffset;
	float eyeToScreenDistance;
	
	vec2 screenDim;
	vec2i resolution;
};

void setIPD(float x);

#endif

