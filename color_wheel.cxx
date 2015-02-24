/* -----------------------------------------------
 * Super nanovol
 * Khairi Reda, 2013
 * Electronic Visualization Laboratory
 * www.evl.uic.edu/kreda
 * 
 * GPL v2 License:
 * http://www.gnu.org/licenses/gpl-2.0.html
 * color_wheel.cxx
 *
 * -----------------------------------------------
 */

#include <math.h>
#include "graphics/common.h"
#include "color_wheel.h"
#include "tf.h"

extern int getWinWidth();
extern int getWinHeight();
extern TransferFunction * getCurrentTF();

ColorWheel::ColorWheel(int _x, int _y):
ScalableWidget(_x, _y, WHEEL_WIDTH, WHEEL_HEIGHT)
{		
	hasWheel = texColor.loadTexture("assets/color_wheel.png", 4);
	selectedColor = Color(1.0f, 1.0f, 1.0f, 1.0f);
	luminance = 1.0f;
	moveable = false;
}

void ColorWheel::draw()
{
	if (!hasWheel) {
		return;
	}
	
	// draw the color wheel
	glPushAttrib(GL_ALL_ATTRIB_BITS);
	{
		glMatrixMode(GL_PROJECTION);
		glPushMatrix();
		glLoadIdentity();
		glOrtho(0, getWinWidth()-1, 0, getWinHeight()-1, -1.0, 1.0);
		
		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
		glLoadIdentity();

		// enables
		glEnable(GL_POINT_SMOOTH);
		glEnable(GL_LINE_SMOOTH);
		glEnable(GL_BLEND);		
		glEnable(GL_TEXTURE_2D);
		
		// give color
		glColor3f(luminance, luminance, luminance);
		
		// bind texture
		texColor.bindTexture();
		drawTexturedBox(x, y, w, h);
		glDisable(GL_TEXTURE_2D);
		
		// draw borders	
		glLineWidth(2.0);
		glColor3f(0.0, 0.0, 0.0);
		glBegin(GL_LINE_STRIP);
		{
			glVertex2f(x,   y);
			glVertex2f(x+w, y);
			glVertex2f(x+w, y+h);
			glVertex2f(x,   y+h);
			glVertex2f(x,   y);
		}
		glEnd();
		
		glMatrixMode(GL_PROJECTION);
		glPopMatrix();
		
		glMatrixMode(GL_MODELVIEW);
		glPopMatrix();
	}
	glPopAttrib();
}

void ColorWheel::rightPick(int mouseX, int mouseY)
{
	if (inWidget(mouseX, mouseY))
	{
		picked = WIDGET_CUSTOM;
	}
}

bool ColorWheel::pointerPick(int mouseX, int mouseY)
{
	if (!hasWheel) {
		return false;
	}
	
	bool ret = ScalableWidget::pointerPick(mouseX, mouseY);
	pickColor(mouseX, mouseY);
	return ret;
}

void ColorWheel::pointerMove(int mouseX, int mouseY, int dX, int dY)
{
	if (!hasWheel) {
		return;
	}
	
	ScalableWidget::pointerMove(mouseX, mouseY, dX, dY);
	if (picked == WIDGET_CUSTOM)
	{
		luminance += float(dY) / 30.0f;
		luminance = min(max(0.0f, luminance), 1.0f);
	}
	else
	{
		pickColor(mouseX, mouseY);
	}
}

void ColorWheel::pickColor(int mouseX, int mouseY)
{
	if (picked == WIDGET_INSIDE && inWidget(mouseX, mouseY))
	{
		float x = float(mouseX - this->x) / float(w-1);
		float y = float(mouseY - this->y) / float(h-1);
		
		x = min(max(x, 0.0f), 1.0f);
		y = min(max(y, 0.0f), 1.0f);
		
		float xx = x * float(texColor.w()-1);
		float yy = y * float(texColor.h()-1);
		
		//int c = min(max(0, int(x)), texColor.w()-1);
		//int r = min(max(0, int(y)), texColor.h()-1);
		
		int c = int(xx);
		int r = int(yy);
					
		const unsigned char * cc = texColor.getPixel(r, c);
		selectedColor = Color(cc[0], cc[1], cc[2]);
		selectedColor.r() *= luminance;
		selectedColor.g() *= luminance;		
		selectedColor.b() *= luminance;

		TransferFunction * tf = getCurrentTF();
		if (tf) {
			tf->updatePointColor( selectedColor );
		}
	}
	
}

