/* ------------------------------------------
 * M E T A draw: for debugging
 * ------------------------------------------*/

#include <iostream>
#include "graphics/graphics.h"
#include "graphics/framebuffer.h"
#include "data.h"
#include "macrocells.h"
#include "camera.h"
#include "VectorT.hxx"

using namespace std;

vec2i traverseTestPixel(-1, -1);
bool fullScreenMeta = false;
float metaRotY = 30.0;
float metaRotX = 0.0;
float metaZoom = .3;

extern Macrocells * macrocells;
extern AtomCube * theCube;
extern int winWidth;
extern int winHeight;

void rotateMeta(float dX, float dY)
{
	metaRotY += dX;
	metaRotX -= dY;
}
void _metaZoom(float z)
{
	metaZoom *= z;
}

void drawPoly(vec3 bL, vec3 u, vec3 v)
{
	vec3 c = bL;
	
	glBegin(GL_QUADS);
	{
		glVertex3f(c.x(), c.y(), c.z());
		c += v;
		glVertex3f(c.x(), c.y(), c.z());
		c += u;
		glVertex3f(c.x(), c.y(), c.z());
		c -= v;
		glVertex3f(c.x(), c.y(), c.z()); 
	}
	glEnd();
}

void metaDraw()
{
#define FB_DIM		4, 4
#define FB_SIZE		4*4*4
#define DO_PATH		true

	Camera & camera = Camera::getInstance();
	static Framebuffer fb(FB_DIM);
	vec3i macrocell(-1,-1,-1);
	
	vec3 testPixel;
	bool hasTestPixel = false;
	vector<vec3> path;
	
	/*
	if (traverseTestPixel.x() >= 0 && traverseTestPixel.y() >= 0)
	{
		hasTestPixel = true;
		testPixel = camera.cameraToWorld(vec2(
			(float) traverseTestPixel.x() / float(winWidth-1),
			(float) traverseTestPixel.y() / float(winHeight-1)
		));
	}
	*/
	
	glPushAttrib( GL_ALL_ATTRIB_BITS  );
	{
		if (!fullScreenMeta) {
			glViewport(0, 0, 350, 350);
		}
		else
		{
			// full screen meta
			// perform traversal test
			/*
			if (hasTestPixel && macrocells)
			{
				// translate pixel to world coordinates
				vec3 testPixel = camera.cameraToWorld(vec2(
					(float) traverseTestPixel.x() / float(winWidth-1),
					(float) traverseTestPixel.y() / float(winHeight-1)
				));
				
				int s = traversalLevel;
				int e = traversalLevel;
				
				if (DO_PATH)
				{
					s = 0;
					e = 1024;
				}
				
				// setup projection
				glMatrixMode(GL_PROJECTION);
				glPushMatrix();
				glLoadIdentity();
				glOrtho(-1.0, 1.0, -1.0, 1.0, -1.0, 1.0);
				
				glMatrixMode(GL_MODELVIEW);
				glPushMatrix();
				glLoadIdentity();

				// setup uniforms
				traverseShader.use();	
				GLint _origin		= traverseShader.getUniform("origin");
				GLint _pixel		= traverseShader.getUniform("pixel");
				GLint _maxDomain	= traverseShader.getUniform("maxDomain");
				GLint _tLevel		= traverseShader.getUniform("tLevel");

				vec3i mcDim = macrocells->getGridDim();		
				glUniform3f(_origin, camera.position.x(), camera.position.y(), camera.position.z());
				glUniform3f(_pixel, testPixel.x(), testPixel.y(), testPixel.z());
				glUniform3f(_maxDomain, mcDim.x()-1, mcDim.y()-1, mcDim.z()-1);

				// zero macrocell
				macrocell = vec3i(-1);

				// bind framebuffer
				fb.bind();
				
				// loop
				for (int i = s; i <= e; i++)
				{
					glUniform1i(_tLevel, i);
				
					// draw quad
					glBegin(GL_QUADS);
					{
						glVertex2f(-1, -1);
						glVertex2f(+1, -1);
						glVertex2f(+1, +1);
						glVertex2f(-1, +1);
					}
					glEnd();
					glFinish();

					// get the texture
					static unsigned char * theFB = new unsigned char[ 
						FB_SIZE * (fb.hasFloatingTexture() ? sizeof(float) : sizeof(unsigned char))
					];
					
					fb.readTexture(theFB);
					if (fb.hasFloatingTexture())
					{
						float * mc = (float*) theFB;
						
						vec3i thisMacrocell;
						thisMacrocell.x() = mc[0];
						thisMacrocell.y() = mc[1];
						thisMacrocell.z() = mc[2];
						
						if (
							thisMacrocell.x() == macrocell.x() &&
							thisMacrocell.y() == macrocell.y() &&
							thisMacrocell.z() == macrocell.z()
						)
						{
							// finished traversal
							break;
						}
						else
						{
							path.push_back(thisMacrocell);
							macrocell = thisMacrocell;
						}
					}
				}
				
				// unbind framebuffer & shader
				fb.unbind();
				Shader::unuse();		
				
				glMatrixMode(GL_MODELVIEW);
				glPopMatrix();
				glMatrixMode(GL_PROJECTION);
				glPopMatrix();				
			}
			*/
			
		}
		
		const vec3i gridDim = macrocells->getGridDim();
		
		glMatrixMode(GL_PROJECTION);
		glPushMatrix();
		glLoadIdentity();
		gluPerspective(45.0, 1.0, 0.01, 500.0);
		
		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
		glLoadIdentity();
		
		gluLookAt( 
			0.0, (double) gridDim.y() * 2.0 / 3.0, gridDim.z() * 2.0,		// eye
			0.0, 0.0, 0.0,
			0.0, 1.0, 0.0
			
		);
		glRotatef(metaRotX, 1.0, 0.0, 0.0);
		glRotatef(metaRotY, 0.0, 1.0, 0.0);
		glScalef(metaZoom, metaZoom, metaZoom);
		glTranslatef(
			-(float) gridDim.x() * .5,
			-(float) gridDim.y() * .5,
			-(float) gridDim.z() * .5
		);
		
		glEnable(GL_BLEND);
		glColor4f(1.0, 0.0, 0.0, 0.3);
		drawPoly( vec3(0.0), vec3(gridDim.x(), 0.0, 0.0), vec3(0.0, gridDim.y(), 0.0) );
		drawPoly( vec3(0.0), vec3(gridDim.x(), 0.0, 0.0), vec3(0.0, 0.0, gridDim.z()));
		drawPoly( vec3(0.0, gridDim.y(), 0.0), vec3(gridDim.x(), 0.0, 0.0), vec3(0.0, 0.0, gridDim.z()));
		drawPoly( vec3(0.0, 0.0, gridDim.z()), vec3(gridDim.x(), 0.0, 0.0), vec3(0.0, gridDim.y(), 0.0));
		drawPoly( vec3(0.0), vec3(0.0, 0.0, gridDim.z()), vec3(0.0, gridDim.y(), 0.0));
		
		
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		
		// draw 6 faces of dataset
		glColor3f(1.0, 0.0, 0.0);
		
		drawPoly( vec3(0.0), vec3(gridDim.x(), 0.0, 0.0), vec3(0.0, gridDim.y(), 0.0) );
		drawPoly( vec3(0.0), vec3(gridDim.x(), 0.0, 0.0), vec3(0.0, 0.0, gridDim.z()));
		drawPoly( vec3(0.0, gridDim.y(), 0.0), vec3(gridDim.x(), 0.0, 0.0), vec3(0.0, 0.0, gridDim.z()));
		drawPoly( vec3(0.0, 0.0, gridDim.z()), vec3(gridDim.x(), 0.0, 0.0), vec3(0.0, gridDim.y(), 0.0));
		drawPoly( vec3(0.0), vec3(0.0, 0.0, gridDim.z()), vec3(0.0, gridDim.y(), 0.0));
		
		// draw macrocell if highlighted
		if (hasTestPixel)
		{
			vec3 M(macrocell.x(), macrocell.y(), macrocell.z());
			vec3 nextM = M + vec3(1,1,1);
			
			/*glColor3f(1.0, 1.0, 0.0);
			drawPoly( M, vec3(nextM.x(), M.y(), M.z()), vec3(M.x(), nextM.y(), M.z()) );
			drawPoly( M, vec3(nextM.x(), M.y(), M.z()), vec3(M.x(), M.y(), nextM.z()) );
			drawPoly( vec3(M.x(), nextM.y(), M.z()), vec3(nextM.x(), M.y(), M.z()), vec3(M.x(), M.y(), nextM.z()));
			drawPoly( vec3(M.x(), M.y(), nextM.z()), vec3(nextM.x(), M.y(), M.z()), vec3(M.x(), nextM.y(), M.z()));
			drawPoly( M, vec3(M.x(), M.y(), nextM.z()), vec3(M.x(), nextM.y(), M.z()));
			*/
			glPointSize(10.0);
			glColor3f(1.0, 1.0, 0.0);
			glBegin(GL_POINTS);
			glVertex3f(M.x(), M.y(), M.z());
			glEnd();
			
			if (path.size() > 1)
			{
				glLineWidth(1.0);
				glBegin(GL_LINE_STRIP);
				for (int j = 0; j < path.size(); j++)
				{
					vec3 & p = path[j];
					glVertex3f(p.x(), p.y(), p.z());
				}
				glEnd();

			}
		}
		
		
		// draw camera
		glColor3f(1.0, 0.0, 1.0);
		glPointSize(6.0); 
		//glBegin(GL_POINTS); glVertex3f( camera.position.x(), camera.position.y(), camera.position.z() ); glEnd();
		
		// draw projection plane (ie camera view)
		drawPoly( camera.bL, camera.bR - camera.bL, camera.tL - camera.bL );
		
		// draw U V W
		glLineWidth( 1.0 );
		glBegin(GL_LINES);
		{
			glColor3f(0.0, 1.0, 0.0);
			glVertex3f( camera.position.x(), camera.position.y(), camera.position.z() );
			glVertex3f( 
				camera.position.x() + camera.w.x() * 40.0, 
				camera.position.y() + camera.w.y() * 40.0, 
				camera.position.z() + camera.w.z() * 40.0 
			);
			
			glColor3f(1.0, 0.0, 0.0);
			glVertex3f( camera.position.x(), camera.position.y(), camera.position.z() );
			glVertex3f( 
				camera.position.x() + camera.u.x() * 40.0, 
				camera.position.y() + camera.u.y() * 40.0, 
				camera.position.z() + camera.u.z() * 40.0 
			);
			
			glColor3f(0.0, 0.0, 1.0);
			glVertex3f( camera.position.x(), camera.position.y(), camera.position.z() );
			glVertex3f( 
				camera.position.x() + camera.v.x() * 40.0, 
				camera.position.y() + camera.v.y() * 40.0, 
				camera.position.z() + camera.v.z() * 40.0 
			);
		}
		glEnd();
		
		/*
		if (hasTestPixel)
		{
			// drawLine
			glLineWidth(3.0);
			glColor3f(1.0, 1.0, 1.0);
			glBegin(GL_LINES);
			{
				vec3 ray = testPixel - camera.position;
				vec3 rayEnd = testPixel + 100.0 * ray;
				
				glVertex3f(testPixel.x(), testPixel.y(), testPixel.z());
				glVertex3f(rayEnd.x(), rayEnd.y(), rayEnd.z());
			}
			glEnd();
		}
		*/
		
		glMatrixMode(GL_PROJECTION);
		glPopMatrix();
		
		glMatrixMode(GL_MODELVIEW);
		glPopMatrix();
	}
	glPopAttrib();
}

