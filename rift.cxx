/* -----------------------------------------------
 * Super nanovol
 * Khairi Reda, 2013
 * Electronic Visualization Laboratory
 * www.evl.uic.edu/kreda
 * 
 * GPL v2 License:
 * http://www.gnu.org/licenses/gpl-2.0.html
 * rift.cxx
 * Support class for Oculus Rift
 * -----------------------------------------------
 */
 
#include <OVR.h>
#include "graphics/graphics.h"
#include "graphics/common.h"
#include "camera.h"
#include "main.h"
#include "rift.h"

using namespace OVR;

//const float DEFAULT_IPD = 0.060f * OVR_METERS_2_GRID;
const float DEFAULT_IPD = 0.0249 * OVR_METERS_2_GRID;

// defined in main.cxx
extern void balls_ray_cast(Camera * cam = NULL, const vec2 * win_coordinates = NULL );
extern int getWinWidth();
extern int getWinHeight();

void ovrRender::init()
{
	System::Init(Log::ConfigureDefaultLog(LogMask_All));
	pManager = *DeviceManager::Create();
	pHMD     = *pManager->EnumerateDevices<HMDDevice>().CreateDevice();
	if (!pHMD) 
	{
		cerr << "No Oculus Rift device connected!" << endl;
		exit(1);
	}
	
	HMDInfo hmd;
	if (pHMD->GetDeviceInfo(&hmd))
	{
		monitorName    = hmd.DisplayDeviceName;
		//ipd    = hmd.InterpupillaryDistance;
		distortionK[0] = hmd.DistortionK[0];
		distortionK[1] = hmd.DistortionK[1];
		distortionK[2] = hmd.DistortionK[2];
		distortionK[3] = hmd.DistortionK[3];

		// near clipping plane = distance from eye to screen
		eyeToScreenDistance = hmd.EyeToScreenDistance * OVR_METERS_2_GRID;
		
		// physical dimensions of screen
		screenDim.x() = hmd.HScreenSize;
		screenDim.y() = hmd.VScreenSize;
		screenDim *= OVR_METERS_2_GRID;

		// distance between lens centers
		lensSeperationDistance = hmd.LensSeparationDistance * OVR_METERS_2_GRID;
	
		// how much we need to shift center of projection by
		lensCenterOffset = screenDim.x() / 4.0f - lensSeperationDistance / 2.0f;

		// screen resplution
		resolution.x() = hmd.HResolution;
		resolution.y() = hmd.VResolution;
		
		pSensor = *pHMD->GetSensor();		
		SFusion = new SensorFusion;
		if (pSensor) {
			cout << "We have an OVR sensor!" << endl;
			SFusion->AttachToSensor(pSensor);
		}
	}
	else
	{
		cerr << "Could not get HMD Info!" << endl;
		exit(1);
		
	}
	
}

void ovrRender::update()
{
	Quatf hmdOrient = SFusion->GetOrientation();
	hmdMat = Matrix4f(hmdOrient);
}

ovrRender::ovrRender()
{
	// initialize the OVR system
	init();
	
	IPD = DEFAULT_IPD;
	
	extern float resScaleX;
	extern float resScaleY;
		
	// make framebuffers
	float fbX = float(resolution.x()) * resScaleX;
	float fbY = float(resolution.y()) * resScaleY;
	
	int fbW = floor(fbX + .5f); fbW += fbW % 2;
	int fbH = floor(fbY + .5f);
	
	cout << "Oculus framebuffer resolution: " << fbW << " x " << fbH << endl;
	
	fb = new Framebuffer(fbW, fbH);	
	fb->setFloatingTexture(false);
	fb->setComponents(3);
	
	// load OVR distortion shader
	const int uniform_count = 7;
	const char * uniforms[uniform_count] = {
		"LensCenter",
		"ScreenCenter",
		"Scale",
		"ScaleIn",
		"HmdWarpParam",
		"Texture0",
		"Texm",
	};
	
	cout << "Loading OVR distortion shader...\t" << flush;
	bool shaderOk = distortionShader.load(
		(getShadersDir() + "/ovr.vert").c_str(),
		(getShadersDir() + "/ovr.frag").c_str(),
		uniform_count,
		uniforms
	);
	
	if (!shaderOk) {
		cout << "Failed." << endl;
		cerr << "Could not load OVR distortion shader." << endl;
		exit(1);
	}
	else
	{
		cout << "OK" << endl;
		Shader::unuse();
	}
	
}

void ovrRender::draw()
{
	Camera & camera			= Camera::getInstance();
	
	glPushAttrib(GL_ALL_ATTRIB_BITS);
	{
		const vec3 eye		= camera.position;
		const vec3 U		= camera.u;
		
		glPushAttrib(GL_ALL_ATTRIB_BITS);
		
		// bind and clear buffer
		fb->bind();
		glClear(GL_COLOR_BUFFER_BIT);
		for (int p = 0; p < 2; p++)
		{
			const float eyeOffset = .5f * (p == 0 ? -IPD : IPD);
			const int w = fb->getWidth() / 2;
			const int h = fb->getHeight();
			
			// draw on either left or right half of buffer
			glViewport(p * w, 0, w, h);
						
			// offset camera
			camera.position = eye + eyeOffset * U;	
			
			// update projection
			camera.updateProjection();
			
			// move camera to center of lense distortion
			// (towards center of device. ie + for left, - for right)
			camera.position += ((p == 0 ? +1.0f : -1.0f) * lensCenterOffset) * U;

			// ray cast
			balls_ray_cast();
			glFinish();
		}
		fb->unbind();
		glPopAttrib();
		
		// restore camera
		camera.position = eye;
		
		glMatrixMode(GL_PROJECTION);
		glPushMatrix();
		glLoadIdentity();         
		glOrtho(0.0, 1.0, 0.0, 1.0, -1.0, 1.0);
		
		glEnable(GL_TEXTURE_2D);
		glDisable(GL_TEXTURE_3D);
		glDisable(GL_TEXTURE_1D);
		glDisable(GL_BLEND);
		
		
	


		// bind texture
		GLint _Texture0	= distortionShader.getUniform("Texture0");
		glActiveTexture(GL_TEXTURE0);
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, fb->getTexture());

		//drawTexturedBoxF(0, 0, 1, 1);

	
		
		// init distortion shader
		distortionShader.use();
		
		// source texture
		glUniform1i(_Texture0, 0);

		int vW = getWinWidth() / 2;
		int vH = getWinHeight();
		
		// left
		glViewport(0, 0, vW, vH);		
		distortScene(0, 0, vW, vH, 1.0);
		drawTexturedBoxF(0, 0, 1, 1, 0, 0, .5, 1);

		// right
		glViewport(vW, 0, vW, vH);		
		distortScene(vW, 0, vW, vH, -1.0);
		drawTexturedBoxF(0, 0, 1, 1, .5, 0, 1, 1);
	
		Shader::unuse();
		
		glPopMatrix();
	}
	glPopAttrib();
}


void ovrRender::distortScene(int VPx, int VPy, int VPw, int VPh, float offset)
{
	int WindowWidth = getWinWidth();
	int WindowHeight = getWinHeight();
	
	float w = float(VPw) / float(WindowWidth);
	float h = float(VPh) / float(WindowHeight);
	float x = float(VPx) / float(WindowWidth);
	float y = float(VPy) / float(WindowHeight);
	
	float as = float(VPw) / float(VPh);
	
	// We are using 1/4 of DistortionCenter offset value here, since it is
	// relative to [-1,1] range that gets mapped to [0, 0.5].
	GLint _LensCenter	= distortionShader.getUniform("LensCenter");
	GLint _ScreenCenter	= distortionShader.getUniform("ScreenCenter");
	GLint _Scale		= distortionShader.getUniform("Scale");
	GLint _ScaleIn		= distortionShader.getUniform("ScaleIn");
	GLint _HmdWarpParam	= distortionShader.getUniform("HmdWarpParam");
	GLint _Texm		= distortionShader.getUniform("Texm");
	GLint _View		= distortionShader.getUniform("View");
	
	float XCenterOffset = 4.0 * lensCenterOffset / screenDim.x();
	XCenterOffset *= offset > 0.0 ? 1.0 : -1.0;
	
	glUniform2f(_LensCenter, x + (w + XCenterOffset * 0.5f)*0.5f, y + h*0.5f);
	glUniform2f(_ScreenCenter, x + w*0.5f, y + h*0.5f);

	// MA: This is more correct but we would need higher-res texture vertically; we should adopt this
	// once we have asymmetric input texture scale.
	float scaleFactor = 1.0f / 1.25;//Distortion.Scale;

	glUniform2f(_Scale,   (w/2) * scaleFactor, (h/2) * scaleFactor * as);
	glUniform2f(_ScaleIn, (2/w),               (2/h) / as);

	glUniform4f(_HmdWarpParam,
		distortionK[0], 
		distortionK[1], 
		distortionK[2], 
		distortionK[3]
	);

	Matrix4f texm(w, 0, 0, 0,
	0, h, 0, 0,
	0, 0, 0, 0,
	0, 0, 0, 1);
	glUniformMatrix4fv(_Texm, 1, GL_TRUE, (GLfloat*) texm.M);
	
	
	Matrix4f view(2, 0, 0, -1,
		0, 2, 0, -1,
		0, 0, 0, 0,
		0, 0, 0, 1);
	glUniformMatrix4fv(_View, 1, GL_TRUE, (GLfloat*) view.M);
}


ovrRender & ovrRender::getInstance()
{
	if (!instance) {
		instance = new ovrRender;
	}
	return *instance;
}

ovrRender * ovrRender::instance = NULL;

