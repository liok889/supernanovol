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

#include <iostream>
#include <assert.h>

#include "shader.h"
#include "graphics.h"

using namespace std;

Shader::Shader()
{
	uniform_variables = NULL;
	ready = false;
	vEnabled = false;
	fEnabled = false;
	gEnabled = false;
	addHeader();
}

Shader::~Shader()
{
	deleteProgram();
}

void Shader::deleteProgram()
{
	if (ready) {
		if (vEnabled) glDeleteShader(v);
		if (fEnabled) glDeleteShader(f);
		if (gEnabled) glDeleteShader(g);
		glDeleteProgram(program);
	}
	
	ready = false;
	delete [] uniform_variables;
	uniform_variables = NULL;
	uniformMap.clear();
}

GLint Shader::getUniform(string name)
{
	if (uniformMap.find( name ) == uniformMap.end())
	{
		// no such uniform
		return -2000;
	}
	else
	{
		return uniformMap[ name ];
	}
}

bool Shader::load(
	const char * vert, const char * frag,
	int uniform_count, const char ** uniform_names
)
{
	if (isReady())
	{
		// recompile the shader
		return recompile();
	}
	else if (!loadShaders(defines, vert, frag, &program, &v, &f)) {
		return false;
	}
	
	vEnabled = vert != NULL;
	fEnabled = frag != NULL;
	
	if (vEnabled) {
		vertFile = vert;
	}
	if (fEnabled) {
		fragFile = frag;
	}
	
	vector<string> uniformNames;
	if (uniform_count > 0)
	{
		uniform_variables = new GLint[ uniform_count ];
		bool uniformError = false;
		
		// get address to uniform variables
		for (int i = 0; i < uniform_count; i++)
		{
			uniform_variables[i] = glGetUniformLocation( program, uniform_names[i] );
			if (uniform_variables[i] == -1) {
				std::cerr << "\t  ! Can't get uniform " << uniform_names[i] << std::endl;
				uniformError = true;
			}
			
			// add name/variable to map
			uniformMap[ uniform_names[i] ] = uniform_variables[i];
			uniformNames.push_back( uniform_names[i] );

		}
		
		if (uniformError) {
			//std::cerr << "\tCould not get one or more uniform variable." << std::endl;
		}
	}
	this->uniform_names = uniformNames;
	ready = true;
	return true;
}

bool Shader::recompile()
{
	if (!ready) {
		return false;
	}
	deleteProgram();

	if (!loadShaders(
		defines, 
		vertFile.length() > 0 ? vertFile.c_str() : NULL, 
		fragFile.length() > 0 ? fragFile.c_str() : NULL, &program, &v, &f)
	) 
	{
		return false;
	}
	
	// get the uniforms again
	if (uniform_names.size() > 0) 
	{
		uniform_variables = new GLint[ uniform_names.size() ];
	}
	for (int i = 0; i < uniform_names.size(); i++)
	{
		uniform_variables[i] = glGetUniformLocation( program, uniform_names[i].c_str() );
		if (uniform_variables[i] == -1) {
			//std::cerr << "\t  ! Can't get uniform " << uniform_names[i] << std::endl;
		}
			
		// add name/variable to map
		uniformMap[ uniform_names[i] ] = uniform_variables[i];
	}

	ready = true;
	return true;
}

void Shader::use()
{
	assert(ready);
	glUseProgram(program);
}

void Shader::unuse()
{
	glUseProgram(0);
}

