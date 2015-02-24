/* -----------------------------------------------
 * Super nanovol
 * Khairi Reda, 2013
 * Electronic Visualization Laboratory
 * www.evl.uic.edu/kreda
 * 
 * GPL v2 License:
 * http://www.gnu.org/licenses/gpl-2.0.html
 * framebuffer.h
 *
 * -----------------------------------------------
 */

#ifndef _FRAMEBUFFER_H__
#define _FRAMEBUFFER_H__

#include "graphics.h"

class Framebuffer
{
public:
	Framebuffer();
	Framebuffer(int w, int h);
	~Framebuffer();
	
	// bind the buffer
	void bind();
	static void unbind();
	void clearBuffer();
	void deleteBuffer();
	
	// chnage the size of buffer
	void setSize(int _w, int _h);
	
	// get / use / read texture from GPU memory
	GLuint getTexture() const { return texture; }
	void useTexture();
	bool readTexture(void *);
	
	// get other information
	bool hasFloatingTexture() const { return _floatingTexture; }
	bool hasDepthBuffer() const { return _depthBuffer; };

	void setFloatingTexture(bool f) { _floatingTexture = f; }
	void setDepthBuffer(bool d) { _depthBuffer = d; }
	void setComponents(int c) { components = c; }

	int getWidth() const { return width; }
	int getHeight() const { return height; }
private:
	// private functions
	void init();
	
	// private variables
	int width, height, components;
	
	// whether to use floating point texture and depth buffer
	bool _floatingTexture, _depthBuffer;
	
	bool inited, failure, textureUsed;
	GLuint framebuffer;
	GLuint depthbuffer;
	GLuint texture;
};

#endif


