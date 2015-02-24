/* -----------------------------------------------
 * Super nanovol
 * Khairi Reda, 2013
 * Electronic Visualization Laboratory
 * www.evl.uic.edu/kreda
 * 
 * GPL v2 License:
 * http://www.gnu.org/licenses/gpl-2.0.html
 * graphics.cpp
 *
 * -----------------------------------------------
 */

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <math.h>
#include "graphics.h"
#include "misc.h"

using namespace std;

/* -------------------------------------------
 * shader loading and initialization
 * -------------------------------------------
 */

bool textureErrorCheck( GLenum target, GLuint texture, GLint _width, GLint _height, GLint _format, GLint _tRed, GLint _tGreen, GLint _tBlue, GLint _tAlpha)
{
	GLint format, tRed, tGreen, tBlue, tAlpha, width, height;
	
	glBindTexture(target, target);
	glGetTexLevelParameteriv( target, 0, GL_TEXTURE_INTERNAL_FORMAT, &format);
	glGetTexLevelParameteriv( target, 0, GL_TEXTURE_WIDTH, &width);
	glGetTexLevelParameteriv( target, 0, GL_TEXTURE_WIDTH, &height);
	glGetTexLevelParameteriv( target, 0, GL_TEXTURE_RED_TYPE, &tRed);
	glGetTexLevelParameteriv( target, 0, GL_TEXTURE_GREEN_TYPE, &tGreen);
	glGetTexLevelParameteriv( target, 0, GL_TEXTURE_BLUE_TYPE, &tBlue);
	glGetTexLevelParameteriv( target, 0, GL_TEXTURE_ALPHA_TYPE, &tAlpha);
	
	if (width != _width || height != _height) {
		cout << "\t ! Incorrecte texture size: " << width << " x " << height << endl;
		return false;
	}

	if (format != _format) {
		cout << "\t ! Incorrecte format: " << format << endl;
		return false;
	}
	
	if (tRed != _tRed) {
		cout << "\t ! Incorrect red channel: " << tRed << endl;
		return false;
	}
	
	if (tGreen != _tGreen) {
		cout << "\t ! Incorrect green channel: " << tRed << endl;
		return false;
	}
	
	if (tBlue != _tBlue) {
		cout << "\t ! Incorrect blue channel: " << tRed << endl;
		return false;
	}
	
	if (tAlpha != _tAlpha) {
		cout << "\t ! Incorrect alpha channel: " << tRed << endl;
		return false;
	}
	
	return true;	
}

bool loadShaders(
	const vector<string> & defines,
	const char * vert, const char * frag, GLuint * prog, GLuint * pVert, GLuint * pFrag
)
{
	GLuint v = 0, f = 0, program;
	char *vText = NULL;
	char *fText = NULL;
	
	if (vert != NULL) {
		v = glCreateShader(GL_VERTEX_SHADER);
		vText = readTextFile(vert);
	}
	
	if (frag != NULL) {
		f = glCreateShader(GL_FRAGMENT_SHADER);
		fText = readTextFile(frag);
	}
	
	
	if (vert && vText == NULL) {
		cerr << "Could not load vertex shader: " << vert << endl;
		delete [] vText; delete [] fText;
		return false;
	}
	else if (frag && fText == NULL) {
		cerr << "Could not load fragment shader: " << frag << endl;
		delete [] vText; delete [] fText;
		return false;
	}
	
	int strNum = defines.size() + 1;
	const char ** vv = new const char*[strNum];
	const char ** ff = new const char*[strNum];
	for (int i = 0; i < defines.size(); i++)
	{
		vv[i] = defines[i].c_str();
		ff[i] = defines[i].c_str();
	}
		
	vv[strNum-1] = vText;
	ff[strNum-1] = fText;
	
	if (vert) glShaderSource(v, strNum, vv, NULL);
	if (frag) glShaderSource(f, strNum, ff, NULL);
	
	delete [] vText; delete [] fText;
	vText = NULL; fText = NULL;
	
	if (vert)
	{
		glCompileShader(v);
		if ( !checkCompileStatus(v) )
		{
			cerr << "Problem with vertex shader: " << vert << endl;
			printShaderLog(v);
			delete [] vv; delete [] ff;
			return false;
		}
	}
	
	
	if (frag)
	{
		glCompileShader(f);
		if ( !checkCompileStatus(f) )
		{
			cerr << "Problem with fragment shader: " << frag << endl;
			printShaderLog(f);
			
			if (vert) {
				glDeleteShader(v);
			}
			delete [] vv; delete [] ff;
			return false;
		}
	}
	
	delete [] vv; delete [] ff;
	
	// link the program
	program = glCreateProgram();
	if (vert) glAttachShader(program, v);
	if (frag) glAttachShader(program, f);
	
	glLinkProgram(program);
	if (! checkLinkStatus(program) ) 
	{
		cerr << "Problem with shader program linkage." << endl;
		printProgramLog(program);
		return false;
	}
	
	// use it
	glUseProgram(program);
	
	// everything OK
	*prog = program;
	
	if (pVert && vert) { *pVert = v; }
	if (pFrag && frag) { *pFrag = f; }
	
	return true;
}

bool loadShadersWithGeometry(
	const char * vert, const char * frag, const char * geom, 
	GLuint * prog, GLuint * pVert, GLuint * pFrag, GLuint * pGeom,
	GLenum inputMode,
	GLenum outputMode
)
{
	GLuint v = 0, f = 0, g = 0, program;
	char *vText = NULL;
	char *fText = NULL;
	char *gText = NULL;
	
	if (vert != NULL) {
		v = glCreateShader(GL_VERTEX_SHADER);
		vText = readTextFile(vert);
	}
	
	if (frag != NULL) {
		f = glCreateShader(GL_FRAGMENT_SHADER);
		fText = readTextFile(frag);
	}
	
	if (geom != NULL) {
		g = glCreateShader(GL_GEOMETRY_SHADER);
		gText = readTextFile(geom);
	}
		
		
	if (vert && vText == NULL) {
		cerr << "Could not load vertex shader: " << vert << endl;
		delete [] vText; delete [] fText; delete [] gText;
		return false;
	}
	else if (frag && fText == NULL) {
		cerr << "Could not load fragment shader: " << frag << endl;
		delete [] vText; delete [] fText; delete [] gText;
		return false;
	}
	else if (geom && gText == NULL) 
	{
		cerr << "Could not load fragment shader: " << geom << endl;
		delete [] vText; delete [] fText; delete [] gText;
		return false;
	}
	
	const char * vv = vText;
	const char * ff = fText;
	const char * gg = gText;
	
	if (vert) glShaderSource(v, 1, &vv, NULL);
	if (frag) glShaderSource(f, 1, &ff, NULL);
	if (geom) glShaderSource(g, 1, &gg, NULL);
	
	delete [] vText; delete [] fText; delete [] gText;
	vText = NULL; fText = NULL; gText = NULL;
	
	if (vert)
	{
		glCompileShader(v);
		if ( !checkCompileStatus(v) )
		{
			cerr << "Problem with vertex shader: " << vert << endl;
			printShaderLog(v);
			return false;
		}
	}
	
	
	if (frag)
	{
		glCompileShader(f);
		if ( !checkCompileStatus(f) )
		{
			cerr << "Problem with fragment shader: " << frag << endl;
			printShaderLog(f);
			
			if (vert) {
				glDeleteShader(v);
			}
			return false;
		}
	}
	
	if (geom)
	{
		glCompileShader(g);
		if ( !checkCompileStatus(g) )
		{
			cerr << "Problem with geometry shader: " << geom << endl;
			printShaderLog(g);
			
			if (vert) {
				glDeleteShader(v);
			}
			if (frag) {
				glDeleteShader(f);
			}
			return false;
		}
	}
	
	// link the program
	program = glCreateProgram();
	if (vert) glAttachShader(program, v);
	if (frag) glAttachShader(program, f);
	if (geom) glAttachShader(program, g);

	// set max geometry out to maximum of graphics card
	glProgramParameteriEXT(program, GL_GEOMETRY_INPUT_TYPE_EXT, inputMode);
	glProgramParameteriEXT(program, GL_GEOMETRY_OUTPUT_TYPE_EXT, outputMode);

	GLint n;
	GLint setN = 40;

	glGetIntegerv(GL_MAX_GEOMETRY_OUTPUT_VERTICES_EXT,&n);
	glProgramParameteriEXT(program,GL_GEOMETRY_VERTICES_OUT_EXT, n);
	cout << "Max vertices OUT from geometry shader: " << n << ", set to: " << n << endl;

	glLinkProgram(program);
	if (! checkLinkStatus(program) ) 
	{
		cerr << "Problem with shader program linkage." << endl;
		printProgramLog(program);
		return false;
	}
	
	// use it
	glUseProgram(program);
	
	// everything OK
	*prog = program;
	
	if (pVert && vert) { *pVert = v; }
	if (pFrag && frag) { *pFrag = f; }
	if (pGeom && geom) { *pGeom = g; }
	
	return true;
}

/* -------------------------------------------
 * shader error checking
 * -------------------------------------------
 */

bool checkCompileStatus(GLuint obj)
{
	GLint status;
	glGetShaderiv(obj, GL_COMPILE_STATUS, &status);
	return status == GL_TRUE;	
}

bool checkLinkStatus(GLuint obj)
{
	GLint status;
	glGetProgramiv(obj, GL_LINK_STATUS, &status);
	return status == GL_TRUE;	
}


bool printShaderLog(GLuint obj)
{
	int infologLength = 0;
	int charsWritten  = 0;
	char *infoLog;

	glGetShaderiv(obj, GL_INFO_LOG_LENGTH,&infologLength);

	if (infologLength > 0)
	{
		// something bad happened
		
		infoLog = new char[ infologLength ];
		glGetShaderInfoLog(obj, infologLength, &charsWritten, infoLog);
		cerr << infoLog << endl;
		delete [] infoLog;
		
		return false;
	}
	else
	{
		return true;
	}
}

bool printProgramLog(GLuint obj)
{

	int infologLength = 0;
	int charsWritten  = 0;
	char *infoLog;

	glGetProgramiv(obj, GL_INFO_LOG_LENGTH,&infologLength);

	if (infologLength > 0)
	{
		// something bad happened
		
		infoLog = new char[ infologLength ];
	        glGetProgramInfoLog(obj, infologLength, &charsWritten, infoLog);
	        
	        cerr << infoLog << endl;
	        delete [] infoLog;
	        
	        return false;
	}
	else
	{
		return true;
	}
}


/* -------------------------------------------
 * OpenGL error checking
 * -------------------------------------------
 */
bool glOK()
{
	// check for OpenGL errors
	GLenum errCode = glGetError();
	if (errCode != GL_NO_ERROR)
	{
		std::cerr << "OpenGL error: " << gluErrorString(errCode) << endl;
		return false;
	} 
	else
	{
		return true;
	}
}

void normalize( float x[3] )
{
	float l = sqrt( x[0]*x[0] + x[1]*x[1] + x[2]*x[2] );
	
	if (l > 0.0f)
	{
		l = 1.0f / l;
		
		
		x[0] *= l;
		x[1] *= l;
		x[2] *= l;
	}
}

