/*   Rising Tide exhibit                                                    */
/*   Copyright (C) 2010 Khairi Reda                                         */
/*   Electronic Visualization Laboratory                                    */
/*   University of Illinois at Chicago                                      */
/*   http://www.evl.uic.edu/                                                */
/*                                                                          */
/*   This is free software; you can redistribute it and/or modify it        */
/*   under the terms of the  GNU General Public License  as published by    */
/*   the  Free Software Foundation;  either version 2 of the License, or    */
/*   (at your option) any later version.                                    */
/*                                                                          */
/*   This program is distributed in the hope that it will be useful, but    */
/*   WITHOUT  ANY  WARRANTY;  without   even  the  implied  warranty  of    */
/*   MERCHANTABILITY or  FITNESS FOR A PARTICULAR PURPOSE.   See the GNU    */
/*   General Public License for more details.                               */
/* ======================================================================== */

#ifndef _SHADER_H__
#define _SHADER_H__

#include <vector>
#include <string>
#include <map>

#include "graphics.h"

using namespace std;

class Shader
{
private:
	GLuint v, f, g, program;
	bool vEnabled, fEnabled, gEnabled, ready;
	map<string, GLint> uniformMap;
	
	// file names
	string vertFile, fragFile, geomFile;
	
	// defines
	vector<string> defines;
	vector<string> uniform_names;
	
	// private functions
	void deleteProgram();
	void addHeader() { defines.push_back( "#version 120\n"); defines.push_back("#extension GL_EXT_gpu_shader4 : enable\n" ); }
public:
	
	GLint * uniform_variables;
	
	Shader();
	~Shader();
	
	
	void clearDefines() { defines.clear(); addHeader(); }
	void addDefine(const char * d) { defines.push_back( string(d) ); }
	void addDefine(const string & d) { defines.push_back( d ); }
	
	bool load(
		const char * vert, const char * frag,
		int uniform_count, const char ** uniform_names 
	);
	
	bool loadWithGeometry(
		const char * vert, const char * frag, const char * geom,
		int uniform_count, const char ** uniform_names,
		GLenum inputMode, GLenum outputMode
	);
	
	bool recompile();
	
	GLint getUniform(string name);
	GLuint getProgram() const { return program; }
	bool inited() const { return ready; }
	bool isReady() const { return ready; }
	void use();
	static void unuse();
	
};

#endif

