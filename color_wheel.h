/* -----------------------------------------------
 * Super nanovol
 * Khairi Reda, 2013
 * Electronic Visualization Laboratory
 * www.evl.uic.edu/kreda
 * 
 * GPL v2 License:
 * http://www.gnu.org/licenses/gpl-2.0.html
 * color_wheel.h
 *
 * -----------------------------------------------
 */

#ifndef _COLOR_WHEEL_H__
#define _COLOR_WHEEL_H__

#include "graphics/color.h"
#include "graphics/texture_unit.h"
#include "scalable_widget.h"

static const int WHEEL_WIDTH		= 100;
static const int WHEEL_HEIGHT		= 100;

class ColorWheel : public ScalableWidget
{

public:
	ColorWheel(int _x, int _y);
	
	// public accessors
	Color getSelectedColor() const { return selectedColor; }
	
	// draw
	virtual void draw();
	
	// move
	virtual bool pointerPick(int mouseX, int mouseY);
	virtual void rightPick(int mouseX, int mouseY);
	virtual void pointerMove(int mouseX, int mouseY, int dX, int dY);
	virtual void pointerMoveAlt(int, int, int, int) {}
private:
	// private functions
	void pickColor(int mouseX, int mouseY);
	
	// private member variables
	bool			hasWheel;
	TextureUnit		texColor;
	
	Color			selectedColor;
	float			luminance;	
};

#endif

