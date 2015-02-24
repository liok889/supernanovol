/* -----------------------------------------------
 * Super nanovol
 * Khairi Reda, 2013
 * Electronic Visualization Laboratory
 * www.evl.uic.edu/kreda
 * 
 * GPL v2 License:
 * http://www.gnu.org/licenses/gpl-2.0.html
 * texture_unit.h
 *
 * -----------------------------------------------
 */

#ifndef _TEXTURE_UNIT_H__
#define _TEXTURE_UNIT_H__

#include <stdlib.h>
#include "graphics.h"

static const int TEXTURE_MONO	= 1;
static const int TEXTURE_RGB	= 3;
static const int TEXTURE_RGBA	= 4;

class TextureUnit;

class TextureUnit
{
private:
	bool loaded;
	GLuint texture_number;
	int width, height, channels;
	unsigned char * raw_texture;
	
	// private member function
	unsigned char * transform( const void * data, int w, int h, int bpp, int channels);
	void flipVertically(unsigned char *, int w, int h, int channels, int bpc);
	GLenum getGLTextureFormat(int channels);
	
	
public:
	TextureUnit();
	~TextureUnit();

	// performs edge detection and resturns new texture unit
	TextureUnit * edge_detect(const char * filename = NULL) const;
	void merge(const TextureUnit * other, const char * filename = NULL);

	void clampToEdge();
	

	// texture loading
	bool loadTexture(const char * filename, int channels, bool flip = false, bool mipMap = true);
	bool loadRaw(const char * filename, int w, int h, int channels, int bpc, bool flip = false);
	
	// file reading
	bool readFile(const char * filename, int _channels);
	
	// pixel access
	const unsigned char * getPixel(int r, int c);
	
	// unloading function
	bool unloadTexture();
	
	int w() const { return width; }
	int h() const { return height; }
	int getWidth() const { return width; }
	int getHeight() const { return height; }
	
	// bind/activate
	void bindTexture();
	GLuint getNumber() const { return texture_number; }
	int  activateTexture(int & texture_id); 
};

bool writeTIFF(const char * filename, unsigned char * theImage, int w, int h, int bpp);

#endif

