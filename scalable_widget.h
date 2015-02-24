/* -----------------------------------------------
 * Super nanovol
 * Khairi Reda, 2013
 * Electronic Visualization Laboratory
 * www.evl.uic.edu/kreda
 * 
 * GPL v2 License:
 * http://www.gnu.org/licenses/gpl-2.0.html
 * scalable_widget.h
 *
 * -----------------------------------------------
 */
 
#ifndef _WIDGET_H___
#define _WIDGET_H___

const int PICK_DIST		= 6;
const int MIN_DIM		= 15;	// min width or height for scalable widget

// what is being picked with the mouse?
enum WIDGET_PICK
{
	WIDGET_NOTHING,		// nothing being picked
	WIDGET_BL,		// bottom left  corner
	WIDGET_BR,		//   =    right corner
	WIDGET_TL,		//  top   left  corner
	WIDGET_TR,		//  top   right corner
	WIDGET_POINT,		// one of the points in the transfer function
	WIDGET_INSIDE,		// inside, but not picking anything
	WIDGET_CUSTOM,		// custom
};

class ScalableWidget
{
	
public:
	ScalableWidget(int _x, int _y, int _w, int _h);
	virtual ~ScalableWidget() {}
	
	// public accessors
	WIDGET_PICK getPicked() const { return picked; }
	bool isPicked() const { return picked != WIDGET_NOTHING; }
	bool inWidget(int pX, int pY) const;
	
	virtual void draw() = 0;
	
	// offset movement
	virtual void moveWidget(int dX, int dY) { x += dX; y += dY; }
	
	// pointer interaction
	virtual bool pointerPick(int mouseX, int mouseY);
	virtual void pointerUnpick();	
	virtual void pointerMove(int mouseX, int mouseY, int dX, int dY);		// left click
	virtual void pointerMoveAlt(int mouseX, int mouseY, int dX, int dY) {}	// right click
	
protected:
	bool moveable;
	int x, y, w, h;
	WIDGET_PICK picked;
};

#endif

