/* -----------------------------------------------
 * Atomizer
 * Khairi Reda, 2013
 * Electronic Visualization Laboratory
 * www.evl.uic.edu/kreda
 * 
 * GPL v2 License:
 * http://www.gnu.org/licenses/gpl-2.0.html
 * graphics.h
 *
 * -----------------------------------------------
 */
 
#ifndef _GRAPHICS_H__
#define _GRAPHICS_H__

#include <vector>
#include <string>
#include <iostream>
#include <GL/glew.h>

#ifdef __APPLE__
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#else
#include <GL/gl.h>
#include <GL/glu.h>
#endif

#include <SDL/SDL.h>

using namespace std;

/* -------------------------------------------
 * shader loading stuff
 */
 
bool loadShaders(
	const vector<string> & defines,
	const char * vert, const char * frag, GLuint * prog, GLuint * pVert, GLuint * pFrag
);

bool loadShadersWithGeometry(
	const char * vert, const char * frag, const char * geom, 
	GLuint * prog, GLuint * pVert, GLuint * pFrag, GLuint * pGeom,
	GLenum inputMode,
	GLenum outputMode
);

bool checkCompileStatus(GLuint obj);
bool checkLinkStatus(GLuint obj);
bool printShaderLog(GLuint obj);
bool printProgramLog(GLuint obj);
void normalize( float x[3] );

/* -------------------------------------------
 * OpenGL error checking
 * -------------------------------------------
 */
bool glOK();


bool textureErrorCheck( 
	GLenum target, GLuint texture,
	GLint _width, GLint _height, 
	GLint _format, GLint _tRed, GLint _tGreen, GLint _tBlue, GLint _tAlpha
);


#endif

