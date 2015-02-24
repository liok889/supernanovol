/* -----------------------------------------------
 * Super nanovol
 * Khairi Reda, 2013
 * Electronic Visualization Laboratory
 * www.evl.uic.edu/kreda
 * 
 * GPL v2 License:
 * http://www.gnu.org/licenses/gpl-2.0.html
 * framebuffer.cpp
 *
 * -----------------------------------------------
 */


#include <iostream>
#include <assert.h>
#include "framebuffer.h"

using namespace std;

Framebuffer::Framebuffer()
{
	inited = false;
	failure = false;
	textureUsed = false;
	
	_depthBuffer = false;			// no depth buffer by default
	_floatingTexture = true;		// use floating point texture by default
	components = 4;
}

Framebuffer::Framebuffer(int _width, int _height)
{
	inited = false;
	failure = false;
	textureUsed = false;
	
	width = _width;
	height = _height;

	// textures
	_depthBuffer = true;
	_floatingTexture = true;
	components = 4;
}

void Framebuffer::setSize(int _w, int _h)
{
	assert(!inited);
	width = _w;
	height = _h;
}

Framebuffer::~Framebuffer()
{
	deleteBuffer();
}

void Framebuffer::deleteBuffer()
{
	if (inited)
	{
		glDeleteFramebuffers(1, &framebuffer);
		if (_depthBuffer) {
			glDeleteRenderbuffers(1, &depthbuffer);
		}
		glDeleteTextures(1, &texture);
		inited = false;
	}
}


void Framebuffer::init()
{
	if (inited || failure) {
		return;
	}
	
	// buffer object
	GLuint fbo;
	glGenFramebuffers(1, &fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);	
	

	GLuint depth;
	if (_depthBuffer)
	{
		// depth buffer
		glGenRenderbuffers(1, &depth);
		glBindRenderbuffer(GL_RENDERBUFFER, depth);
		
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth);
	}

	// color
	GLuint img;
	glGenTextures(1, &img);
	glBindTexture(GL_TEXTURE_2D, img);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );
	
	if (_floatingTexture)
	{
		glTexImage2D(
			GL_TEXTURE_2D, 0, 
			components == 4 ? GL_RGBA32F : GL_RGB32F,  
			width, height, 0, 
			components == 4 ? GL_RGBA : GL_RGB, 
			GL_FLOAT, NULL
		);
	}
	else
	{
		glTexImage2D(
			GL_TEXTURE_2D, 0, 
			components == 4 ? GL_RGBA8 : GL_RGB8, 
			width, height, 0, 
			components == 4 ? GL_RGBA : GL_RGB, 
			GL_UNSIGNED_BYTE, NULL
		);
	}
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, img, 0);	
	
	// check status
	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (status != GL_FRAMEBUFFER_COMPLETE_EXT)
	{
		cerr << "Error initializing Framebuffer of " << width << " x " << height << endl;
		failure = true;
	}
	else
	{
	
		inited = true;
		failure = false;
		
		// store identifiers
		framebuffer = fbo;
		depthbuffer = depth;
		texture = img;
		
		// remove the bind for now
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}
	
}

bool Framebuffer::readTexture(void * p)
{
	glBindTexture(GL_TEXTURE_2D, texture);
	glGetTexImage(
		GL_TEXTURE_2D, 0, 
		components == 4 ? GL_RGBA : GL_RGB,
		hasFloatingTexture() ? GL_FLOAT : GL_UNSIGNED_BYTE,
		p
	);
	glFinish();
	
	return true;
}

void Framebuffer::clearBuffer()
{
	unsigned char * data = new unsigned char[width*height*4];
	memset(data, 0, width*height*4);
	
	glBindTexture(GL_TEXTURE_2D, texture);
	if (_floatingTexture)
	{
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F,  width, height, 0, GL_RGBA, GL_FLOAT, data);
	}
	else
	{
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8,  width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
	}
	delete [] data;
}

void Framebuffer::bind()
{
	if (!inited)
	{
		init();
		assert(inited);
	}
	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
}

void Framebuffer::unbind()
{
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Framebuffer::useTexture()
{
	assert(inited);
	glBindTexture(GL_TEXTURE_2D, texture);
}

