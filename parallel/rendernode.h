/* -----------------------------------------------
 * Super nanovol
 * Khairi Reda, 2013
 * Aaron Knoll, 2011-2013
 * Electronic Visualization Laboratory
 * www.evl.uic.edu/kreda
 * 
 * GPL v2 License:
 * http://www.gnu.org/licenses/gpl-2.0.html
 * rendernode.h
 * 
 * -----------------------------------------------
 */
 
#ifndef _SUPER_NANOVOL_RENDERNODE__
#define _SUPER_NANOVOL_RENDERNODE__

#include <string>
#include <vector>
#include "../graphics/graphics.h"
#include "../graphics/framebuffer.h"
#include "../graphics/shader.h"
#include "../camera.h"
#include "../Matrix44.hxx"
#include "../Timer.h"

using namespace std;

class RenderSurface;
class Node;

// available stereo modes
enum STEREO_MODE
{
	MONO,
	INTERLACED,
};

void setStereoMode(STEREO_MODE mode);
void setStereoDisparity(float d);

class GLWindow
{
public:
	GLWindow(int _x, int _y, int _w, int _h, bool _makefull, float resScaleX = 1.0, float resScaleY = 1.0);
	~GLWindow();

	// this should be called from the main loop
	// in order to receive new parameters from master
	void sync();

	// draw
	double draw(void);
public:

	// set rendering resolution (relative to size of window)
	int w() const { return width; }
	int h() const { return height; }
	void setResScale(float x, float y);
	string getHostName() const { return myHostName; }
private:
	string myHostName;

	// private member functions
	void dumpFrame(string image_format = "jpg");
	void setupProjection();
	void ray_cast(const RenderSurface *, vec3 cameraOffset);
	void receiveCamera();
	bool receiveTransferFunction(bool blocking);
	bool receiveCommand(bool blocking);
	
	// rendering surfaces (set by outside)
	vector<RenderSurface> surfaces;
	
	STEREO_MODE		stereoMode;
	int x, y, width, height, frameW, frameH;
	
	// framebuffer
	Framebuffer		* left, * right;
	Framebuffer		* main;
	
	// information synced with master	
	Matrix44		caveTransform;
	Camera			camera;
	
	// interlacing shader
	Shader interlacer;
	
	friend vector<GLWindow*> openRenderWindows(float resScaleX, float resScaleY, Node *& myNode); 
	
};

void bilinear(const unsigned char * image, int inWidth, int inLength, int channels, unsigned char * output, int width, int length);

#endif

