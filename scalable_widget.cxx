/* -----------------------------------------------
 * Super nanovol
 * Khairi Reda, 2013
 * Electronic Visualization Laboratory
 * www.evl.uic.edu/kreda
 * 
 * GPL v2 License:
 * http://www.gnu.org/licenses/gpl-2.0.html
 * scalable_widget.cxx
 *
 * -----------------------------------------------
 */

#include <stdlib.h> 
#include "scalable_widget.h"

ScalableWidget::ScalableWidget(int _x, int _y, int _w, int _h)
{
	x = _x;
	y = _y;
	w = _w;
	h = _h;
	
	picked = WIDGET_NOTHING;
	moveable = true;
}

bool ScalableWidget::inWidget(int pX, int pY) const
{
	if ( pX >= x && pX <= x+w && pY >= y && pY <= y+h )
	{
		return true;
	}
	else 
	{
		return false;
	}
}


bool ScalableWidget::pointerPick(int mouseX, int mouseY)
{
	picked = WIDGET_NOTHING;
	
	if ( abs(x-mouseX) < PICK_DIST && abs(y-mouseY) < PICK_DIST )
	{
		picked = WIDGET_BL;
	}
	else if ( abs(x+w-mouseX) < PICK_DIST && abs(y-mouseY) < PICK_DIST )
	{
		picked = WIDGET_BR;
	}
	else if ( abs(x-mouseX) < PICK_DIST && abs(y+h-mouseY) < PICK_DIST )
	{
		picked = WIDGET_TL;
	}
	else if ( abs(x+w-mouseX) < PICK_DIST && abs(y+h-mouseY) < PICK_DIST )
	{
		picked = WIDGET_TR;
	}
	else if ( inWidget(mouseX, mouseY) )
	{
		picked = WIDGET_INSIDE;
	}
	
	return picked != WIDGET_NOTHING;
}

void ScalableWidget::pointerUnpick()
{
	picked = WIDGET_NOTHING;
}

void ScalableWidget::pointerMove(int mouseX, int mouseY, int dX, int dY)
{
	switch(picked)
	{	
	case WIDGET_BL:
		if (w - dX < MIN_DIM && dX > 0)
		{
			dX = MIN_DIM - w; 
		}
		if (h - dY < MIN_DIM && dY > 0)
		{
			dY = MIN_DIM - h;
		}
		
		x += dX;
		y += dY;
		
		w -= dX;
		h -= dY;
		break;
		
	case WIDGET_BR:
		
		if (w + dX < MIN_DIM && dX < 0)
		{
			dX = MIN_DIM-w;
		}
		if (h - dY < MIN_DIM && dY > 0)
		{
			dY = MIN_DIM+h;
		}
		
		w += dX;
		h -= dY;
		y += dY;
		break;
		
	case WIDGET_TL:
		
		if (w - dX < MIN_DIM && dX > 0)
		{
			dX = MIN_DIM-w;
		}
		if (h + dY < MIN_DIM && dY < 0)
		{
			dY=MIN_DIM-h;
		}
		
		w -= dX;
		h += dY;
		x += dX;
		break;
		
	case WIDGET_TR:
		
		if (w+dX < MIN_DIM && dX < 0)
		{
			dX = MIN_DIM - w;
		}
		if (h+dY < MIN_DIM && dY < 0)
		{
			dY = MIN_DIM -h;
		}
		
		w += dX;
		h += dY;
		break;
		
	case WIDGET_INSIDE:
		if (moveable) {
			// move the entire widget
			x += dX;
			y += dY;
		}
		break;
	}
}



