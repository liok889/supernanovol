/* -----------------------------------------------
 * Super nanovol
 * Khairi Reda, 2013
 * Electronic Visualization Laboratory
 * www.evl.uic.edu/kreda
 * 
 * GPL v2 License:
 * http://www.gnu.org/licenses/gpl-2.0.html
 * color.h
 * An awesome color class
 * -----------------------------------------------
 */
#ifndef _COLOR_H___
#define _COLOR_H___

#include <string.h>

// A 4-channel color class
class Color
{
private:
	float theColor[4];
	void translateChar(float * p, int c) { *p = (float) c / 255.0f; }
public:
	// copy constructor
	Color(const Color & rhs)
	{
		memcpy(theColor, rhs.theColor, 4*sizeof(float));
	}
	
	Color & operator=(const Color & rhs)
	{
		memcpy(theColor, rhs.theColor, 4*sizeof(float));
		return *this;
	}
	
	// other constructors
	Color() {
		theColor[0] = 0.0; theColor[1] =0.0; theColor[2] = 0.0; theColor[3] = 1.0; 
	}
	
	Color(float r, float g, float b) {
		theColor[0] = r; theColor[1] = g; theColor[2] = b; theColor[3] = 1.0; 
	}
	
	Color(float r, float g, float b, float a) {
		theColor[0] = r; theColor[1] = g; theColor[2] = b; theColor[3] = a; 
	}
	
	Color(int _r, int _g, int _b)
	{
		setR(_r); setG(_g); setB(_b); theColor[3] = 1.0;
	}
	
	Color(int _r, int _g, int _b, int _a)
	{
		setR(_r); setG(_g); setB(_b); setA(_a);
	}
	
	Color(unsigned char c)
	{
		setR(c);
		setG(c);
		setB(c);
		setA(255);
	}
	
	void setHTML(int c)
	{
		if (c > 0x00FFFFFF) {
			// 4 components
			setR( (0xFF000000 & c) >> 24 );
			setG( (0x00FF0000 & c) >> 16 );
			setB( (0x0000FF00 & c) >> 8  );
			setA(  0x000000FF & c);
		}
		else
		{
			setR( (0xFF0000 & c) >> 16 );
			setG( (0x00FF00 & c) >> 8  );
			setB(  0x0000FF & c);
			theColor[3] = 1.0f;
		}
	}
	
	
	Color(int c)
	{
		setHTML(c);
	}
	
	Color(int cc, int aa)
	{
		setHTML(cc);
		setA(aa);
	}
	
	
	const float * v() const { return theColor; }
	float & r() { return theColor[0]; }
	float & g() { return theColor[1]; }
	float & b() { return theColor[2]; }
	float & a() { return theColor[3]; }
	
	const float & r() const { return theColor[0]; }
	const float & g() const { return theColor[1]; }
	const float & b() const { return theColor[2]; }
	const float & a() const { return theColor[3]; }
	
	void setR(int _r) { translateChar(theColor  , _r); }
	void setG(int _g) { translateChar(theColor+1, _g); }
	void setB(int _b) { translateChar(theColor+2, _b); }
	void setA(int _a) { translateChar(theColor+3, _a); }
};


#endif


