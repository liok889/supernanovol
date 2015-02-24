/* -----------------------------------------------
 * Super nanovol
 * Khairi Reda, 2013
 * Electronic Visualization Laboratory
 * www.evl.uic.edu/kreda
 * 
 * GPL v2 License:
 * http://www.gnu.org/licenses/gpl-2.0.html
 * tf.h
 *
 * -----------------------------------------------
 */
 
#ifndef _TRANSFER_FUNCTION_H___
#define _TRANSFER_FUNCTION_H___

#include <string>
#include <vector>
#include "graphics/graphics.h"
#include "graphics/framebuffer.h"
#include "scalable_widget.h"
#include "VectorT.hxx"
#include "graphics/color.h"

using namespace std;

struct TFPoint
{
	float	value;		// normalized	0 ... 1
	Color 	color;		// RGBA		(height represents alpha)
	
	TFPoint(float _value, Color _color, float alpha = -1.0)
	{
		value = _value;
		color = _color;
		if (alpha >= 0.0)
		{
			color.a() = alpha;
		}
	}
};

class TransferFunction : public ScalableWidget
{
public:
	TransferFunction(string filename = "", bool _loadToGPU = true);
	TransferFunction(const TransferFunction &);
	~TransferFunction();

	int getPickedPoint() const { return pickedPoint; }
	
	
	// draw
	virtual void draw();

	// load / save to file
	bool load(string filename);
	bool save(string filename);
	
	// marshal / unmarshal
	void marshal(float * data, int & count);
	void unmarshal(const float * data, int count);
	
	// additional TF specific manipulation
	void rightPick(int pX, int pY);
	void addPoint(int pX, int pY);
	
	// mouse
	virtual bool pointerPick(int mouseX, int mouseY);
	virtual void pointerMove(int mouseX, int mouseY, int dX, int dY);
	virtual void pointerUnpick();
	
	// return the texture ID of the rendered transfer function
	// so that we can bind it to the shader
	GLuint getTFTex() { return tfFB.getTexture(); }

	// updates the color of the currently selected point
	void updatePointColor( Color c );
	
	void setUpdateCallback( void (* _callback)(void) ) { callback = _callback; }
	void setInterpTarget(const TransferFunction * other);
	void tick(float dT);
	float getInterpAlpha() const { return interpAlpha; }
	bool isInterpolating() const { return interpTarget != NULL; }
private:
	
	// private functions
	bool testPointPick(int i, int mouseX, int mouseY);
	void makeDefault();
	vec2 mapPoint(float u, float v);
	void upload_GLSL(bool enableCallback = true);
	
	// interpolation target
	float interpAlpha;
	const TransferFunction * interpTarget;
	
	
	// points in the transfer function
	vector<TFPoint>			points;
	
	// data min / max
	float				dataMin, dataMax, dataLen, maxAlpha;
	
	// OpenGL texture
	GLuint				tfTex;
	bool				texInited;
	bool				loadToGPU;
	bool				hasGPUData;
	
	// what's being picked
	int				pickedPoint;
	
	// framebuffer to render transfer function
	Framebuffer			tfFB;
	
	// update callback
	void (*callback)(void);
};

Color interpolate(float a, Color c1, Color c2);

#endif


