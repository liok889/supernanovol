/* -----------------------------------------------
 * Super nanovol
 * Khairi Reda, 2013
 * Aaron Knoll, 2011-2013
 * Electronic Visualization Laboratory
 * www.evl.uic.edu/kreda
 * 
 * GPL v2 License:
 * http://www.gnu.org/licenses/gpl-2.0.html
 * rendernode.cxx
 * 
 * -----------------------------------------------
 */

#include <stdlib.h>
#include <mpi.h>
#include "../graphics/graphics.h"
#include "../graphics/shader.h"
#include "../graphics/common.h"
#include "../graphics/image.h"
#include "../tf.h"
#include "../main.h"
#include "../data.h"
#include "rendernode.h"
#include "cluster.h"

// default stereo mode
static STEREO_MODE defaultStereoMode = INTERLACED;
static float STEREO_DISPARITY = 1.05f * 0.125f;

float getStereoDisparity()
{
	return STEREO_DISPARITY;
}

void setStereoMode(STEREO_MODE mode) 
{
	defaultStereoMode = mode;
}

void setStereoDisparity(float d) 
{
	STEREO_DISPARITY = d;
}

GLWindow::GLWindow(int _x, int _y, int _w, int _h, bool _makefull, float resScaleX, float resScaleY)
:x(_x), y(_y), width(_w), height(_h), main(NULL), left(NULL), right(NULL), stereoMode(defaultStereoMode)
{
	// open SDL window
	int error;
	error = SDL_Init(SDL_INIT_VIDEO);
	if (error < 0)
	{
		cerr << "Could not initialize SDL." << endl;
		exit(1);
	}	

	int flags = SDL_OPENGL | SDL_DOUBLEBUF | (_makefull ? SDL_FULLSCREEN : 0);
	if (!SDL_SetVideoMode(width, height, 32, flags))
	{
		cerr << "Could not open SDL window." << endl;
		exit(1);
	}
	SDL_ShowCursor(SDL_DISABLE);

	// set the render resolution and create framebuffers
	// -- framebuffers not initialized until draw() is called
	setResScale(resScaleX, resScaleY);
}

GLWindow::~GLWindow()
{
}

void GLWindow::sync()
{
	receiveCamera();
	receiveTransferFunction(false);
	receiveCommand(false);	
}

void GLWindow::setupProjection()
{
	static bool setBefore = false;
	
	if (!setBefore) 
	{	
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(0.0, 1.0, 0.0, 1.0, -1.0, 1.0);
		
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		setBefore = true;
		
		// load interlacing shader
		const char * uniforms[2] = {"left", "right"};
		if (!interlacer.load(
			NULL, 
			(getShadersDir() + "interlace_3d.frag").c_str(), 2, uniforms)
		)
		{
			cerr << "\n\n **** Could not load 3D interlacer shader.\n" << endl;
			exit(1);
		}
		Shader::unuse();
		
		// pixel store pack to 1 so we can download frame correctly
		glPixelStorei(GL_PACK_ALIGNMENT, 1);
	}
}


void GLWindow::dumpFrame(string IMAGE_FORMAT)
{
	extern CubeSequence * cubeSequence;
	extern string movieDir;
	extern int movieWidth;
	extern int movieHeight;
		
	static unsigned char * frame = NULL;
	static unsigned char * stereo_frame = NULL;
	static unsigned char * resampled_frame = NULL;
	
	static int mainW = 0;
	static int mainH = 0;
		
	static int leftW = 0;
	static int leftH = 0;
	
	if (
		mainW != main->getWidth() || mainH != main->getHeight() ||
		leftW != left->getWidth() || leftH != left->getHeight()
	) 
	{
		// reset buffers
		delete [] frame;
		delete [] stereo_frame;
		delete [] resampled_frame;
		
		frame = NULL;
		stereo_frame = NULL;
		resampled_frame = NULL;
		
		mainW = main->getWidth();
		mainH = main->getHeight();
		leftW = left->getWidth();
		leftH = left->getHeight();
		
	}
	
	// allocate memory for frame
	if (frame == NULL || stereo_frame == NULL) 
	{
		frame = new unsigned char[mainW * mainH * 3];
		stereo_frame = new unsigned char[leftW * leftH * 3];
		
		if (movieWidth > 0 && movieHeight > 0) 
		{
			resampled_frame = new unsigned char[movieWidth * movieHeight * 3];
		}
	}
	
	assert(cubeSequence);
	string basename = fileName(cubeSequence->getCurrentTimestep()->dataFile);
	string framename;
	
	switch (stereoMode)
	{
	case MONO:
	
		framename = movieDir + "/" + basename + "." + IMAGE_FORMAT;
		main->readTexture(frame);
		if (resampled_frame)
		{
			cout << "Resample: " << mainW << " x " << mainH << "\tto " << movieWidth << " x " << movieHeight << endl;
			bilinear(
				frame, mainW, mainH, 3, 
				resampled_frame, movieWidth, movieHeight
			);
			
			image_flip(movieWidth, movieHeight, 3, 1, frame);
			image_write( framename.c_str(), movieWidth, movieHeight, 3, 1, resampled_frame);
		
		}
		else
		{
			image_flip(mainW, mainH, 3, 1, frame);
			image_write( framename.c_str(), mainW, mainH, 3, 1, frame);
		}
		break;
	
	case INTERLACED:
		
		// left 
		framename = movieDir + "/" + basename + "_left" + "." + IMAGE_FORMAT;
		left->readTexture(stereo_frame);
		image_flip( left->getWidth(), left->getHeight(), 3, 1, stereo_frame);
		image_write( framename.c_str(), left->getWidth(), left->getHeight(), 3, 1, stereo_frame);
		
		// right
		framename = movieDir + "/" + basename + "_right" + "." + IMAGE_FORMAT;
		right->readTexture(stereo_frame);
		image_flip( right->getWidth(), right->getHeight(), 3, 1, stereo_frame);
		image_write( framename.c_str(), right->getWidth(), right->getHeight(), 3, 1, stereo_frame);
	}
}


double GLWindow::draw()
{
	extern bool dumpMovie;
	extern bool fixedCamera;
	extern bool noSync;
	
	// sync
	if (!noSync)
		sync();
	
	Timer timer; timer.start();	
	setupProjection();
	glPushAttrib(GL_ALL_ATTRIB_BITS);
	
	// how many passes
	int passes = stereoMode == INTERLACED ? 2 : 1;
	
	for (int pass = 0; pass < passes; pass++)
	{
		Framebuffer * fb;
		switch (stereoMode)
		{
		case INTERLACED:
			fb = (pass == 0 ? left : right);
			break;
		
		case MONO:
			fb = main;
			break;
		}
		
		fb->bind();
		glViewport(0, 0, fb->getWidth(), fb->getHeight());
		glClear(GL_COLOR_BUFFER_BIT);
		
		// transform surfaces using the latest transform
		// received from the master
		vector<RenderSurface> transformedSurfaces;
		for (int s = 0; s < surfaces.size(); s++)
		{
			RenderSurface tS = surfaces[s].transform( caveTransform );
			transformedSurfaces.push_back( tS );	
		}
		
		if (fixedCamera)
		{
			balls_ray_cast(NULL, NULL);
		}
		else
		{
			for (int s = 0; s < transformedSurfaces.size(); s++)
			{
				vec3 cameraOffset(0.0f);
				if (stereoMode == INTERLACED && pass == 0) 
				{
					// left
					cameraOffset = -STEREO_DISPARITY * transformedSurfaces[s].U;
				}
				else if (stereoMode == INTERLACED && pass == 1) 
				{
					// right
					cameraOffset = +STEREO_DISPARITY * transformedSurfaces[s].U;
				}
				ray_cast( &transformedSurfaces[s], cameraOffset );
			}
		}
		
		fb->unbind();
		glFinish();

	}
	glPopAttrib();
	timer.stop();

	Framebuffer::unbind();
	Shader::unuse();
	
	glPushAttrib(GL_ALL_ATTRIB_BITS);
	
	// compose the buffers
	glDisable(GL_TEXTURE_1D);
	glEnable(GL_TEXTURE_2D);
	glDisable(GL_TEXTURE_3D);
	glDisable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);
	glColor3f(1.0f, 1.0f, 1.0f);

	glViewport(0, 0, width, height);
	switch (stereoMode)
	{
	case MONO:

		main->useTexture();
		drawTexturedBoxF(0.0f, 0.0f, 1.0f, 1.0f);
		break;
		
	case INTERLACED:
		interlacer.use();
		{
			static GLint _left = interlacer.getUniform("left");
			static GLint _right = interlacer.getUniform("right");
			
			glActiveTexture(GL_TEXTURE0 + 0);
			glEnable(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D, left->getTexture());
			glUniform1i(_left, 0);
			
			glActiveTexture(GL_TEXTURE0 + 1);
			glEnable(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D, right->getTexture());
			glUniform1i(_right, 1);
		}
		drawTexturedBoxF(0.0f, 0.0f, 1.0f, 1.0f);			
		Shader::unuse();
		break;
	}
	
	
	if (dumpMovie) 
	{
		dumpFrame();
	}

	// finish render
	glPopAttrib();
	glFinish();
	
	// get render time
	double renderTime = timer.getElapsedTimeInSec();
	
	// resync and swap buffer
	if (!noSync)
		MPI_Barrier(MPI_COMM_WORLD);
	SDL_GL_SwapBuffers();
	
	return renderTime;
}

void GLWindow::setResScale(float scaleX, float scaleY)
{
	// window width / height is our base rendering resolution
	float fW = float(w());
	float fH = float(h());
	
	// if the scale is >= 10, interpret this it the desired rendering resolution
	if (scaleX >= 10.0) {
	
		fW = scaleX;
		scaleX = 1.0;
	}
	if (scaleY >= 10.0) {
		fH = scaleY;
		scaleY = 1.0;
	}
	
	// figure out the resolution of the framebuffer
	float fbStereoWidth = fW;
	float fbStereoHeight = fH / 2.0f;
	
	float fbMainWidth = fW;
	float fbMainHeight = fH;
	
	// scale rendering resolution by desired factor
	fbStereoWidth *= scaleX;
	fbStereoHeight *= scaleY;
	
	fbMainWidth *= scaleX;
	fbMainHeight *= scaleY;
	
	int stereoWidth = floor(fbStereoWidth + .5);
	int stereoHeight = floor(fbStereoHeight + .5);
	
	int mainWidth = floor(fbMainWidth + .5);
	int mainHeight = floor(fbMainHeight + .5);
	
	// set frame resolution
	frameW = mainWidth;
	frameH = mainHeight;
	
	if (left) delete left;
	if (right) delete right;
	if (main) delete main;
	
	left		= new Framebuffer(fbStereoWidth, fbStereoHeight);
	right		= new Framebuffer(fbStereoWidth, fbStereoHeight);
	main		= new Framebuffer(fbMainWidth, fbMainHeight);
	
	left->setFloatingTexture(false);
	right->setFloatingTexture(false);
	main->setFloatingTexture(false);
	
	left->setComponents(3);
	right->setComponents(3);
	main->setComponents(3);
	
}

void GLWindow::receiveCamera()
{
	SyncBlock sync;
	
	MPI_Bcast(&sync, sizeof(SyncBlock), MPI_UNSIGNED_CHAR, 0, MPI_COMM_WORLD);
	
	// unpack camera
	camera.unmarshal(sync.camera);
	
	// unpack cave transform
	memcpy(&caveTransform, sync.caveTransform, sizeof(Matrix44));
	
	// see if we are interpolating between transfer functions
	if (sync.interpTarget > -1) 
	{
		extern vector<TransferFunction *> extraTFs;
		getCurrentTF()->setInterpTarget( extraTFs[sync.interpTarget] );
	}
	getCurrentTF()->tick( sync.deltaTime );

	// sync
	MPI_Barrier(MPI_COMM_WORLD);
}

bool GLWindow::receiveTransferFunction(bool blocking)
{
	static const int  TF_BUFFER_SIZE = 1024*2;
	static unsigned char buffer[TF_BUFFER_SIZE];	

	int flag;
	MPI_Status status;
	if (!blocking)
	{
		MPI_Iprobe(MPI_ANY_SOURCE, TRANSFER_FUNCTION_UPDATE_TAG, MPI_COMM_WORLD, &flag, &status); 
		if (flag != 1)
		{
			// no update
			return false;	
		}
	}

	// actual receive
	MPI_Recv(buffer, TF_BUFFER_SIZE, MPI_UNSIGNED_CHAR, MPI_ANY_SOURCE, TRANSFER_FUNCTION_UPDATE_TAG, MPI_COMM_WORLD, &status);

	// unpack
	getCurrentTF()->unmarshal( (const float*) buffer, 0 );
	
	return true;
}

bool GLWindow::receiveCommand(bool blocking)
{
	
	int flag;
	MPI_Status status;
	static const int MAX_BUFFER_SIZE = 1024*2;
	static unsigned char buffer[MAX_BUFFER_SIZE];
	
	if (!blocking)
	{
		MPI_Iprobe(MPI_ANY_SOURCE, COMMAND_TAG, MPI_COMM_WORLD, &flag, &status); 
		if (flag != 1)
		{
			return false;	
		}
	}
	
	MPI_Recv(buffer, MAX_BUFFER_SIZE, MPI_UNSIGNED_CHAR, MPI_ANY_SOURCE, COMMAND_TAG, MPI_COMM_WORLD, &status);
	const unsigned char * ptr = buffer;
	
	// unpack command
	vector<int> Is;
	vector<float> Fs;
	string command;
	
	int strLen;
	int iCount, fCount;
	
	memcpy(&strLen,	ptr, sizeof(int));		ptr += sizeof(int);
	memcpy(&iCount, ptr, sizeof(int));		ptr += sizeof(int);
	memcpy(&fCount, ptr, sizeof(int));		ptr += sizeof(int);
	command = (const char *) ptr;			ptr += strLen;
	
	for (int i = 0; i < iCount; i++)
	{
		int x;
		memcpy(&x, ptr, sizeof(int));		ptr += sizeof(int);
		Is.push_back(x);
	}
	
	for (int i = 0; i < fCount; i++)
	{
		float x;
		memcpy(&x, ptr, sizeof(float));		ptr += sizeof(float);
		Fs.push_back(x);
	}
		
	// now see what command we have
	if ( command == "delta_t" )
	{
		_dT(Fs[0]);	
	}
	else if (command == "color_scale")
	{
		_colorScale(Fs[0]);		
	}
	else if (command == "draw_balls")
	{
		_drawBalls(Is[0] != 0);
	}
	else if (command == "draw_volume")
	{
		_drawVolume(Is[0] != 0);
	}
	else if (command == "background")
	{
		_bgColor(Fs[0], Fs[1], Fs[2]);
	}
	else if (command == "res_scale")
	{
		setResScale(Fs[0], Fs[1]);
	}
	else if (command == "toggle_stereo")
	{
		bool stereo = stereoMode == INTERLACED;
		if (!stereo) 
		{
			stereoMode = INTERLACED;
		}
		else
		{
			stereoMode = MONO;
		}
	}
	else if (command == "stereo_disparity")
	{
		setStereoDisparity(Fs[0]);
	}
	else if (command == "clip_box")
	{
		_clipBox(Is[0] != 0);
	}
	else if (command == "clip_box_extents")
	{
		_clipBoxMin(vec3(Fs[0], Fs[1], Fs[2]));
		_clipBoxMax(vec3(Fs[3], Fs[4], Fs[5]));
	}
	return true;
}


void GLWindow::ray_cast(const RenderSurface * surface, vec3 cameraOffset)
{
	Camera cam;
	
	cam.bL = surface->bL();
	cam.bR = surface->bR();
	cam.tL = surface->tL();
	cam.tR = surface->tR();
	
	const vec2 win_n_coordinates[4] = 
	{
		surface->n_bottomLeft,
		surface->n_topLeft,
		surface->n_topRight,
		surface->n_bottomRight
	};
	
	cam.position = camera.position + cameraOffset;
	balls_ray_cast( &cam, win_n_coordinates );	
}

unsigned char roundNclamp(double x)
{
	int i = floor(x + .5);
	if (i < 0) {
		i = 0;
	}
	else if (i > 255)
	{
		i = 255;
	}
	return (unsigned char) i;
}

void bilinear(const unsigned char * image, int inWidth, int inLength, int channels, unsigned char * output, int width, int length)
{
	double coef_x = (double) inLength / (double) length;
	double coef_y = (double) inWidth  / (double) width;
	
	int x, y, v;
	
	int inRowOffset = inWidth * channels;
	int rowOffset =  width * channels;
	
	for (int x = 0; x < length; x++)
	{
		for (int y = 0; y < width; y++)
		{
			for(int v = 0; v < channels; v++)
			{
				int l = (int)(x*coef_x);
				int k = (int)(y*coef_y);
				double a = x*coef_x - l;
				double b = y*coef_y - k;
				
				output[ x*rowOffset + y*channels + v ] =
				
				roundNclamp(
					(1-a)*(1-b)* (double) image[l*inRowOffset + k*channels + v]+
					a*(1-b)* (double) image[(l+1)*inRowOffset + k*channels + v]+
					b*(1-a)* (double) image[l*inRowOffset + (k+1)*channels + v]+
					a*b* (double) image[(l+1)*inRowOffset + (k+1)*channels + v]
				);
			}
		}
	}
}

