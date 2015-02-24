/* -----------------------------------------------
 * Super nanovol
 * Khairi Reda, 2013
 * Electronic Visualization Laboratory
 * www.evl.uic.edu/kreda
 * 
 * GPL v2 License:
 * http://www.gnu.org/licenses/gpl-2.0.html
 * main.cxx
 *
 * -----------------------------------------------
 */
 
#include <iostream>
#include <string>
#include <sstream>
#include <stdlib.h>

#include "graphics/graphics.h"
#include "graphics/framebuffer.h"
#include "graphics/shader.h"
#include "graphics/misc.h"
#include "graphics/common.h"
#include "data.h"
#include "macrocells.h"
#include "charge_volume.h"
#include "camera.h"
#include "Matrix44.hxx"
#include "Timer.h"
#include "tf.h"
#include "color_wheel.h"
#include "main.h"
#include "video_out.h"

// MPI
#ifdef DO_MPI
	#include <mpi.h>
	#include "parallel/cluster.h"
	#include "parallel/tracker.h"
	#include "parallel/masternode.h"
	#include "parallel/rendernode.h"
	#include "parallel/tracker.h"
	
	#define CAVE2_TRACK_HOST "cave2tracker.evl.uic.edu"
	#define CAVE2_TRACK_PORT 28000
	#define NO_SLAVE_OUTPUT
	
	// make sure we're not doing MPI and RIFT at the same time!
	#ifdef DO_RIFT
	#error "Can not define DO_RIFT and DO_MPI at the same time."
	#endif
#endif



// Oculus Rift
#ifdef DO_RIFT
	#include "rift.h"
#endif

#define EVAA (;;)
#define hasExtension(X) (glewGetExtension(X) ? "X" : " ")

using namespace std;

// default window size
int winWidth = 1000;
int winHeight = 800;

int getWinWidth() { return winWidth; }
int getWinHeight() { return winHeight; }

// background color
float bgColor[4] = {0.4, 0.4, 0.4, 1.0};
void _bgColor(float r, float g, float b) 
{
	bgColor[0] = r;
	bgColor[1] = g;
	bgColor[2] = b;
}

// model view paramters
float rotationY = 0.0f, rotationX = 0.0f;
float theScale = 1.0f;

// scale of data
float voxelsPerAngstrom		= 0.5f;		// macrocell voxels per angstrom
float chargeVoxelsPerAngstrom	= 1.0f;		// density voxels per anstrom
float dT				= 0.1f;		// delta T when stepping through the volume
float colorScale			= 7.0f;		// how much to scale volume opacity by
float atomScale			= .4f;		// how big the atom is (of its Van der Waals radius)
int geometryPrecision		= 0;		// whether we perform precise geometry intersects (by not exiting too early)

void _colorScale(float f)	{ colorScale = f; }
void _dT(float f)		{ dT = f; }

// mouse
vec2i absMouse(0);

// resolution scaling
float resScaleX = 1.0f, resScaleY = 1.0f;
const float MAX_RES_SCALE 	= 2.0;		// super sampling max
const float MIN_RES_SCALE	= 0.1;

// debugging
int testModifier			= 0;		// a modifier to turn on different debugging modes

// mouse buttons that are pushed
bool leftButton = false, rightButton = false, middleButton = false;

// how many ray traversal levels (used mostly for debugging)? 
int traversalLevel = 0;

// data load / save / create
string dataFile, macrocellsFile, volumeFile, tfFile, sequenceFile, cameraFile, movieDir;
bool loadSequence		= false;
#ifdef DO_MPI
bool noSync			= false;
bool offlineSequenceRender	= false;
#endif
bool fixedCamera			= false;
bool dumpMovie			= false;
bool buildIfNeeded		= false;
bool loadRaw			= false;
bool buildMacrocells		= false;
bool buildVolume			= false;
bool loadVolume			= false;
bool loadMacrocells		= false;
bool t_level			= false;		// whether t_level stepping is on (for debugging)
bool fullscreen			= false;
bool noFrame			= false;
bool wiggle			= false;
bool noCursor			= false;

// stereo rendering (this pertains non-MPI, non-OVR mode only)
bool enableStereo		= false;
bool enableStereoTest		= false;
float stereoSeparation		= 1.0f * .5f;

// whether to raycast
bool drawMeta			= false;
bool densityRayCast		= false;
bool ballsRayCast		= true;	
bool skipEmpty			= false;
bool drawVolume			= true;			void _drawVolume(bool b)	{ drawVolume = b; compileShaders(); }
bool drawBalls			= true;			void _drawBalls(bool b)	{ drawBalls = b; compileShaders(); }		
bool drawTF			= true;
bool drawColorWheel		= true;
bool clipBox			= false;			void _clipBox(bool b)	{ clipBox = b; compileShaders(); }
vec3 clipBoxMin			= vec3(0.0f);		void _clipBoxMin(vec3 m)	{ clipBoxMin = m; }
vec3 clipBoxMax			= vec3(1.0f);		void _clipBoxMax(vec3 m)	{ clipBoxMax = m; }
bool thinClient			= false;			// if true, we won't render data on master, just interface

// meta keys
bool shiftKey			= false;
bool controlKey			= false;
bool altKey			= false;
bool xKey = false, yKey = false, zKey = false;

// size of movie (defaults to size of framebuffer)
int movieWidth = 0, movieHeight = 0;

// degree <-> radians conversion
const double DEGREE_TO_RAD = 0.0174532925;

// the data
AtomCube * theCube		= NULL;
CubeSequence * cubeSequence	= NULL;
Macrocells * macrocells		= NULL;
ChargeDensityVolume * volume	= NULL;

// transfer functions
TransferFunction * tf		= NULL;			// default transfer function: this is the one we are primarily using
vector<TransferFunction *> 	extraTFs;		// additional TFs
int currentTFInterp		= 0;			// interpolation TF
ColorWheel * colorWheel		= NULL;


TransferFunction * getCurrentTF() { return tf; }

// video output
VideoOut * videoout = NULL;

// shader
string shadersDir = "shaders/";
void setShadersDir(string _dir) { shadersDir = _dir + "/"; }
string getShadersDir() { return shadersDir; }
Shader densityShader, ballsShader, interlaceShader;

#ifdef DO_MPI	
	MasterWindow * masterNode = NULL;
	vector<GLWindow *> renderNodes;
	Node * myNode = NULL;
#endif

// update rate for master window (applicable only in thin_client && MPI mode)
static const double THIN_CLIENT_UPDATE_RATE = 1.0 / 15.0;

bool parseCmdLine(int argc, char ** argv)
{
	for (int i = 1; i < argc; i++)
	{
		if (0 == strcasecmp("-width", argv[i]) && i < argc-1)
		{
			winWidth = atoi(argv[++i]);
		}
		else if (0 == strcasecmp("-height", argv[i]) && i < argc-1)
		{
			winHeight = atoi(argv[++i]);
		}
		else if (0 == strcasecmp("-load_raw", argv[i]))
		{
			loadRaw = true;	
		}
		else if (0 == strcasecmp("-vpa", argv[i]) && i < argc-1)
		{
			voxelsPerAngstrom = atof(argv[++i]);
		}
		else if (0 == strcasecmp("-cvpa", argv[i]) && i < argc-1)
		{
			chargeVoxelsPerAngstrom = atof(argv[++i]);
		}
		else if (0 == strcasecmp("-build_macrocells", argv[i]))
		{
			buildMacrocells = true;
		}
		else if (0 == strcasecmp("-load_macrocells", argv[i]))
		{
			loadMacrocells = true;
		}
		else if (0 == strcasecmp("-build_volume", argv[i]))
		{
			buildVolume = true;
		}
		else if (0 == strcasecmp("-load_volume", argv[i]))
		{
			loadVolume = true;
		}
		else if (0 == strcasecmp("-fixed_camera", argv[i]))
		{
			fixedCamera = true;
		}
		else if (0 == strcasecmp("-color_scale", argv[i]) && i < argc-1)
		{
			colorScale = atof(argv[++i]);
		}
		else if (0 == strcasecmp("-delta_t", argv[i]) && i < argc-1)
		{
			dT = atof(argv[++i]);
		}
		else if (0 == strcasecmp("-no_balls", argv[i]))
		{
			drawBalls = false;
		}
		else if (0 == strcasecmp("-no_volume", argv[i]))
		{
			drawVolume = false;
		}
		else if (0 == strcasecmp("-res_scale", argv[i]) && i < argc-2)
		{
			resScaleX = atof(argv[++i]);
			resScaleY = atof(argv[++i]);
		}
		else if (0 == strcasecmp("-movie_size", argv[i]) && i < argc-2)
		{
			movieWidth = atoi(argv[++i]);
			movieHeight = atoi(argv[++i]);
		}
		
		else if (0 == strcasecmp("-background", argv[i]) && i < argc-3)
		{
			bgColor[0] = atof(argv[++i]);
			bgColor[1] = atof(argv[++i]);
			bgColor[2] = atof(argv[++i]);
			
		}
		else if (0 == strcasecmp("-no_cursor", argv[i])) {
			noCursor = true;
		}
		else if (0 == strcasecmp("-geometry_precision", argv[i]) && i < argc-1)
		{
			geometryPrecision = atoi(argv[++i]);
		}
		else if (0 == strcasecmp("-atom_scale", argv[i]) && i < argc-1)
		{
			atomScale = atof(argv[++i]);	
		}
		else if (0 == strcasecmp("-traversal_level", argv[i]) && i < argc-1)
		{
			traversalLevel = atoi(argv[++i]);
		}
		else if (0 == strcasecmp("-dump_movie", argv[i]) && i < argc-1) 
		{
			movieDir = argv[++i];
			//dumpMovie = true;
			if (!verifyCreateDir( movieDir.c_str() )) 
			{
				cerr << "Can not use directory to dump movie. Aborting..." << endl;
				exit(1);
			}
			static const int MOVIE_FPS = 24;
			
			videoout = new VideoOut(getWinWidth(), getWinHeight(), movieDir, MOVIE_FPS); 
			cout << "Dumping movie to: " << movieDir << " at " << MOVIE_FPS << endl;
		}
		else if (0 == strcasecmp("-clip_box", argv[i]) && i < argc-6)
		{
			clipBoxMin.x() = atof(argv[++i]);
			clipBoxMin.y() = atof(argv[++i]);
			clipBoxMin.z() = atof(argv[++i]);
			
			clipBoxMax.x() = atof(argv[++i]);
			clipBoxMax.y() = atof(argv[++i]);
			clipBoxMax.z() = atof(argv[++i]);
			clipBox = true;
		}
		else if (0 == strcasecmp("-shaders_dir", argv[i]) && i < argc-1)
		{
			setShadersDir(argv[++i]);
		}
		else if (0 == strcasecmp("-tf", argv[i]) && i < argc-1)
		{
			// see if file exists
			string _tfFile =argv[++i]; 
			if (fileExists(_tfFile)) 
			{
				cout << "Adding transfer function: " << _tfFile << endl;
				extraTFs.push_back( new TransferFunction(_tfFile, false) );
			}
		}
	
		
	#ifndef DO_MPI
		else if (0 == strcasecmp("-stereo", argv[i]))
		{
			enableStereo = true;
		}
	#else	// DO_MPI
		else if (0 == strcasecmp("-stereo", argv[i])) 
		{
			setStereoMode(INTERLACED);	
		}
		else if (0 == strcasecmp("-mono", argv[i])) 
		{
			setStereoMode(MONO);
		}
		else if (0 == strcasecmp("-stereo_disparity", argv[i]) && i < argc-1)
		{
			setStereoDisparity(atof(argv[++i]));
		}
		else if (0 == strcasecmp("-render_sequence", argv[i]))
		{
			offlineSequenceRender = true;
		}
		else if (0 == strcasecmp("-no_sync", argv[i]))
		{
			noSync = true;
		}
		else if (0 == strcasecmp("-thin_client", argv[i]))
		{
			thinClient = true;
		}
		else if (0 == strcasecmp("-conf_file", argv[i]) && i < argc-1)
		{
			setConfFile(argv[++i]);
		}
	#endif
	
	#ifdef DO_RIFT
		else if (0 == strcasecmp("-ovr", argv[i])) 
		{
			winWidth = 1280;
			winHeight = 800;
			fullscreen = true;
		}
		else if (0 == strcasecmp("-no_frame", argv[i]))
		{
			noFrame = true;
			fullscreen = false;
		}
	#endif
		else if (argv[i][0] == '-') 
		{
			cerr << "Ignoring unrecognized option " << argv[i] << endl;
		}
		else 
		{
			dataFile = argv[i];
			string ext = fileExtension( dataFile );
			string fName = fileName( dataFile );
			
			if (0 == strcasecmp(ext.c_str(), "seq")) 
			{
				loadSequence = true;
			}
			else
			{
				loadSequence = false;
			}
			volumeFile	= fName + ".volume";
			macrocellsFile	= fName + ".macrocells";
			tfFile		= fName + ".tf";
			cameraFile	= fName + ".camera";
		}
	}
	
	// is there data?
	if (dataFile.length() == 0) {
		cerr << "No data file specified!\n";
		cerr << "Please include name of data file to load as command line argument.\n";
		exit(1);
	}

	// sanity check
	if (!loadRaw && buildMacrocells) {
		cerr << "Need to use '-load_raw' in conjunction with '-build_macrocells'.\n";
		cerr << "Need to read the raw atoms file in order to build macrocells.\n";
		exit(1);
	}
	
	if (!loadRaw && buildVolume) {
		cerr << "Need to use '-load_raw' in conjunction with '-build_volume'.\n";
		cerr << "Need to read the raw atoms file in order to build density volume.\n";
		exit(1);
	}
	
	if (buildMacrocells && loadMacrocells) {
		cerr << "Can not use '-load_macrocells' in conjunction with '-build_macrocells'.\n";
		exit(1);
	}
	
	if (buildVolume && loadVolume) {
		cerr << "Can not use '-load_volume' in conjunction with '-build_volume'.\n";
		exit(1);
	}
	
	if (!buildMacrocells && !loadMacrocells) 
	{
		cerr << "Need either '-load_macrocells' or '-build_macrocells'.\n";
		exit(1);
	}
	
	#ifdef DO_MPI	
	if (offlineSequenceRender && !loadSequence) {
		cerr << "I expect a .seq file listing file names to render in sequence.\n";
		exit(1);
	}
	
	if (thinClient && offlineSequenceRender) {
		cerr << "Can not use '-thin_client' with '-render_sequence'.\n";
		cerr << "Please remove '-thin_client' argument and try again.\n";
		exit(1);
	}
	#endif
	
	return true;
}

void initSDL()
{	
	// init SDL
	int error;
	error = SDL_Init(SDL_INIT_VIDEO);
	if (error < 0)
	{
		cerr << "Could not initialize SDL." << endl;
		exit(1);
	}	

	int flags = SDL_OPENGL | SDL_DOUBLEBUF /*| SDL_RESIZABLE |  SDL_HWSURFACE */;
	if (fullscreen) {
		flags |= SDL_FULLSCREEN;
	}
	if (noFrame) {
		flags |= SDL_NOFRAME;
	}
	if (!SDL_SetVideoMode(winWidth, winHeight, 32, flags))
	{
		cerr << "Could not open SDL window." << endl;
		exit(1);
	}
	if (noCursor)
		SDL_ShowCursor(SDL_DISABLE);
}

/* ------------------------------------
 *	SHADERS
 * ------------------------------------*/
 
void customizeShader()
{
	char buffer[2*1024];
	
	// add defines
	// =======================================
	ballsShader.clearDefines();
	
	if (t_level)
		ballsShader.addDefine("#define T_LEVEL\n");
	
	if (macrocells && drawBalls)
		ballsShader.addDefine("#define BALLS_RENDER\n");
	
	if (skipEmpty)
		ballsShader.addDefine("#define SKIP_EMPTY\n");
	
	if (macrocells && macrocells->getRefMode() == MC_DIRECT)
		ballsShader.addDefine("#define DIRECT_ATOMS_REF\n"); 
	
	if (volume && drawVolume)
		ballsShader.addDefine("#define VOLUME_RENDER\n");
	
	if (clipBox)
		ballsShader.addDefine("#define CLIP_BOX\n");
	
	bool preciseGeometry = geometryPrecision > 0;
	if (preciseGeometry)
	{
		sprintf(buffer, "#define GEOMETRY_PRECISION %d\n", geometryPrecision);
		ballsShader.addDefine("#define PRECISE_GEOMETRY\n");
		ballsShader.addDefine(buffer);
	}
	else
	{
		ballsShader.addDefine("#define GEOMETRY_PRECISION 0\n");
	}
	
	// background color
	sprintf(buffer, "const vec4 background = vec4(%f, %f, %f, %f);\n", bgColor[0], bgColor[1], bgColor[2], bgColor[3]);
	ballsShader.addDefine(buffer);
	
	if (macrocells) 
	{
		// maxDomain
		const vec3i & mcDim = macrocells->getGridDim();
		vec3 maxDomain(float(mcDim.x())-0.1, float(mcDim.y())-0.1, float(mcDim.z())-0.1);
		vec3 inv_maxDomain = vec3(1.0/maxDomain.x(), 1.0/maxDomain.y(), 1.0/maxDomain.z());
		
		sprintf(buffer ,"const vec3 maxDomain = vec3(%f, %f, %f);\n", maxDomain.x()+.1, maxDomain.y()+.1, maxDomain.z()+.1);
		ballsShader.addDefine(buffer);

		sprintf(buffer, "const vec3 inv_maxDomain = vec3(%f, %f, %f);\n", inv_maxDomain.x(), inv_maxDomain.y(), inv_maxDomain.z());
		ballsShader.addDefine(buffer);
		
		// imaxDomain
		sprintf(buffer ,"const ivec3 imaxDomain = ivec3(%d, %d, %d);\n", mcDim.x()-1, mcDim.y()-1, mcDim.z()-1);
		ballsShader.addDefine(buffer);
		
		
		// atomScale
		sprintf(buffer, "const float atomScale = %f;\n", macrocells->getAtomScale() * macrocells->getVPA());
		ballsShader.addDefine(buffer);
		
		if (macrocells->getRefMode() == MC_INDEXED)
		{
			// indices sqrt
			sprintf(buffer, "const int indicesSqrt = %d;\n", macrocells->getIndicesSqrt());
			ballsShader.addDefine(buffer);
			
			// atoms sqrt
			sprintf(buffer, "const int atomsSqrt = %d;\n", macrocells->getAtomsSqrt());
			ballsShader.addDefine(buffer);
		}
		else if (macrocells->getRefMode() == MC_DIRECT)
		{
			sprintf(buffer, "const int atomsSqrt = %d;\n", macrocells->getIndicesSqrt());
			ballsShader.addDefine(buffer);
		}
	}
	
	if (volume && drawVolume)
	{
		sprintf(buffer, "const float maxVolumeValue = %f;\n", volume->getMaxValue());
		ballsShader.addDefine(buffer);
	
		const vec3 & vExtent = volume->getVolumeGSExtent();
		sprintf(buffer ,"const vec3 maxVDomain = vec3(%f, %f, %f);\n", vExtent.x(), vExtent.y(), vExtent.z() );
		ballsShader.addDefine(buffer);

		sprintf(buffer, "const vec3 inv_maxVDomain = vec3(%f, %f, %f);\n", 1.0f / vExtent.x(), 1.0f / vExtent.y(), 1.0f / vExtent.z());
		ballsShader.addDefine(buffer);
		
		sprintf(buffer, "const float volumeScale = %f;\n", volume->getVPA() / macrocells->getVPA());
		ballsShader.addDefine(buffer);
		
		sprintf(buffer, "const float densityScale = %f;\n", volume->getDensityScale());
		ballsShader.addDefine(buffer);
		
		sprintf(buffer, "const float densityOffset = %f;\n", volume->getDensityOffset());
		ballsShader.addDefine(buffer);	
		
		cout << "\t Vol density scale: " << volume->getDensityScale() << ", offset: " << volume->getDensityOffset() << endl;
	}
}

void compileShaders()
{
	
	#ifdef DO_MPI
		if (thinClient && masterNode != NULL) 
		{
			return;
		}
	#endif
	
	customizeShader();

	// DENSITY
	// ========
	const int DENSITY_UNIFORM = 8;
	const char * density_uniforms[DENSITY_UNIFORM] = {
		"origin",
		"nearClip",
		"maxDomain",
		"tLevel",
		"densityTexture",
		"maxDensity",
		"background",
		"modifier"
	};
	
	// load / re-compile shader
	//cout << "\nDENSITY shader" << endl; 
	if (!densityShader.load(
		(shadersDir + "density.vert").c_str(), 
		(shadersDir + "density.frag").c_str(),
		DENSITY_UNIFORM, density_uniforms)
	)
	{
		cerr << "Could not compile DENSITY shader." << endl;
		exit(1);
	}

	// BALLS
	// ======
	const int BALLS_UNIFORM = 12;
	const char * balls_uniforms[BALLS_UNIFORM] = 
	{
		"origin",			// camera position in world space
		"nearClip",			// near clipping
		"tLevel",			// max traversal level
		"macrocells",			// macrocells texture
		"indices",			// indices texture
		"atoms",			// atoms texture
		"volume",			// volume texture
		"tf",				// transfer function texture
		"dT",				// delta T when stepping through volume
		"colorScale",			// used to scale opacity of the volume
		"clipBoxMin",
		"clipBoxMax",
	};

	// load / re-compile shader
	//cout << "\nBALLS shader" << endl;
	if (!ballsShader.load(
		(shadersDir + "balls.vert").c_str(),
		(shadersDir + "balls.frag").c_str(),
		BALLS_UNIFORM, balls_uniforms)
	)
	{
		cerr << "Could not compile BALLS shader." << endl;
		exit(1);
	}

	// INTERLACE shader (only compiled once)
#ifndef DO_MPI
	if (!interlaceShader.isReady()) {
			
		const char * uniforms[2] = {"left", "right"};
		if (!interlaceShader.load(
			NULL,
			(shadersDir + "interlace_3d.frag").c_str(), 2, uniforms)
		)
		{
			cerr << "Could not compile INTERLACE shader." << endl;
			exit(1);
		}		
		
	}
#endif
	
	Shader::unuse();	
}

void initGL()
{	
	// init GLEW
	GLenum err = glewInit();
	if (err != GLEW_OK)
	{
		cerr << "Could not initialize GLEW! BAD BAD!!" << endl;
		exit(1);
	}
	
	// print summary
	cout << "Using GLEW " << glewGetString(GLEW_VERSION) << " and OpenGL " << glGetString(GL_VERSION) << endl;
	cout << "Extensions:\n";
	
	cout << "\tGL_ARB_framebuffer_object" << "\t\t" << hasExtension("GL_ARB_framebuffer_object") << "\n";
	cout << "\tGL_EXT_gpu_shader4" << "\t\t\t" << hasExtension("GL_EXT_gpu_shader4") << "\n";
	cout << "\tGL_ARB_texture_buffer_object" << "\t\t" << hasExtension("GL_ARB_texture_buffer_object") << "\n";
	cout << "\tGL_EXT_texture_integer" << "\t\t\t" << hasExtension("GL_EXT_texture_integer") << "\n";
	
	cout << "\n";
	
	// viewport
	glViewport(0, 0, winWidth, winHeight);

	// enables / disables
	glEnable(GL_BLEND);
	glEnable(GL_POINT_SMOOTH);
	glDisable(GL_DEPTH_TEST);
	
	// background color
	glClearColor(bgColor[0], bgColor[1], bgColor[2], bgColor[3]);
	
	// additive blending
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// packing alignment to 1 (relevant only when we are saving a movie)
	glPixelStorei(GL_PACK_ALIGNMENT, 1);	
}

void initCamera()
{
	assert(macrocells);
	Camera & camera = Camera::getInstance();
	const vec3i & gridDim = macrocells->getGridDim();
	
	// initialize camera
	camera.position.x() = (float) gridDim.x() * .5;
	camera.position.y() = (float) gridDim.y() * 2.0 / 3.0;
	camera.position.z() = max(gridDim.y(), gridDim.z()) * 2;

	// look at center of dataset
	camera.lookAt.x() = (float) gridDim.x() * .5;
	camera.lookAt.y() = (float) gridDim.y() * .5;
	camera.lookAt.z() = (float) gridDim.z() * .5;
	
	// initialize projection parameters
	#ifndef DO_RIFT
		
		// projection plane
		const float L = 15.0;
		float R = (float) winHeight / (float) winWidth;
		R *= L;
		camera.left = -L * .5f;
		camera.right = L * .5f;
		camera.bottom = -R * .5f;
		camera.top = R * .5f;
		camera.nearClip = 10.5;
		camera.farClip = 500.0;
		
		camera.uvw();
	#endif
}

/* ------------------------------------------
 *              R A Y  casting
 * ------------------------------------------*/
void draw_quad( GLint vWorld, Camera * cam = NULL, const vec2 * _coordinates = NULL )
{
	if (cam == NULL) { cam = &Camera::getInstance(); }
	
	glPushAttrib(GL_TRANSFORM_BIT | GL_ENABLE_BIT);
	{
		if (!_coordinates)
		{
			glMatrixMode(GL_PROJECTION);
			glPushMatrix();
			glLoadIdentity();
			glOrtho(-1.0, 1.0, -1.0, 1.0, -1.0, 1.0);
			
			glMatrixMode(GL_MODELVIEW);
			glPushMatrix();
			glLoadIdentity();
		}
		
		// enable blending
		glEnable(GL_BLEND);
		glDisable(GL_DEPTH_TEST);
		
		static const vec2 default_coordinates[4] = 
		{
			vec2(-1.0, -1.0),
			vec2(-1.0, 1.0),
			vec2(1.0, 1.0),
			vec2(1.0, -1.0)
		};
		const vec2 * c = _coordinates == NULL ? default_coordinates : _coordinates;
		
		glBegin(GL_QUADS);
		{
			if (vWorld >= 0)
				glVertexAttrib3f(vWorld, cam->bL.x(), cam->bL.y(), cam->bL.z());
			glVertex2f(c[0][0], c[0][1]);
			
			if (vWorld >= 0)
				glVertexAttrib3f(vWorld, cam->tL.x(), cam->tL.y(), cam->tL.z());
			glVertex2f(c[1][0], c[1][1]);
			
			if (vWorld >= 0)
				glVertexAttrib3f(vWorld, cam->tR.x(), cam->tR.y(), cam->tR.z());
			glVertex2f(c[2][0], c[2][1]);
			
			if (vWorld >= 0)
				glVertexAttrib3f(vWorld, cam->bR.x(), cam->bR.y(), cam->bR.z());
			glVertex2f(c[3][0], c[3][1]);
		}
		glEnd();
		
		if (!_coordinates)
		{
			glMatrixMode(GL_PROJECTION);
			glPopMatrix();
			
			glMatrixMode(GL_MODELVIEW);
			glPopMatrix();
		}
	}
	glPopAttrib();
}


void density_ray_cast()
{
	assert(macrocells);
	const vec3i gridDim = macrocells->getGridDim();
	Camera & camera  = Camera::getInstance();

	densityShader.use();
	
	// vertex attribute
	GLint vWorld		= glGetAttribLocation( densityShader.getProgram(), "vWorld" ); 
	
	// update shader uniforms
	GLint _nearClip		= densityShader.getUniform("nearClip");
	GLint _origin		= densityShader.getUniform("origin");
	GLint _maxDomain	= densityShader.getUniform("maxDomain");

	GLint _tLevel		= densityShader.getUniform("tLevel");
	GLint _densityTexture	= densityShader.getUniform("densityTexture");
	GLint _maxDensity	= densityShader.getUniform("maxDensity");
	GLint _modifier		= densityShader.getUniform("modifier");
	
	glUniform1f(_nearClip, camera.nearClip);
	glUniform3f(_origin, camera.position.x(), camera.position.y(), camera.position.z());
	glUniform3f(_maxDomain, gridDim.x(), gridDim.y(), gridDim.z());	
	glUniform1i(_tLevel, traversalLevel);
	glUniform1i(_modifier, testModifier);
	
	
	// full screen quad
	draw_quad( vWorld );
	
	Shader::unuse();
}

void balls_ray_cast(Camera * cam = NULL, const vec2 * win_coordinates = NULL )
{
	if (cam == NULL) { 
		cam = &Camera::getInstance(); 
	}
	
	// make sure balls data is uploaded to the GPU
	if (!macrocells->hasGLSLData())
	{
		macrocells->upload_GLSL();
	}
	
	if (volume && drawVolume && !volume->hasGLSLData())
	{
		volume->upload_GLSL();
	}
	
	ballsShader.use();

	// vertex attribute
	GLint vWorld = glGetAttribLocation( ballsShader.getProgram(), "vWorld" ); 
	
	// get uniforms
	GLint _nearClip		= ballsShader.getUniform("nearClip");
	GLint _origin		= ballsShader.getUniform("origin");
	GLint _maxDomain	= ballsShader.getUniform("maxDomain");
	GLint _tLevel		= ballsShader.getUniform("tLevel");
	
	// textures
	GLint _macrocells	= ballsShader.getUniform("macrocells");
	GLint _atoms		= ballsShader.getUniform("atoms");
	GLint _indices		= ballsShader.getUniform("indices");
	GLint _volume		= ballsShader.getUniform("volume");
	GLint _tf		= ballsShader.getUniform("tf");
	GLint _dT		= ballsShader.getUniform("dT");
	GLint _colorScale	= ballsShader.getUniform("colorScale");
	
	// clipbox
	GLint _clipBoxMin	= ballsShader.getUniform("clipBoxMin");
	GLint _clipBoxMax	= ballsShader.getUniform("clipBoxMax");
	
	glUniform1f(_nearClip, cam->nearClip);
	glUniform3f(_origin, cam->position.x(), cam->position.y(), cam->position.z());
	glUniform1i(_tLevel, traversalLevel);
	
	// textures
	// ===========
	int texIndex = 0;
	
	// macrocells
	glActiveTexture(GL_TEXTURE0 + texIndex);
	glEnable(GL_TEXTURE_3D);
	glBindTexture(GL_TEXTURE_3D, macrocells->getMacrocellsTex());
	glUniform1i(_macrocells, texIndex++);

	// indices
	if (macrocells->getRefMode() == MC_INDEXED)
	{
		glActiveTexture(GL_TEXTURE0 + texIndex);
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, macrocells->getIndicesTex());
		glUniform1i(_indices, texIndex++);
		
		// atoms
		glActiveTexture(GL_TEXTURE0 + texIndex);
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, macrocells->getAtomsTex());
		glUniform1i(_atoms, texIndex++);
	}
	else if (macrocells->getRefMode() == MC_DIRECT)
	{
		// atoms
		glActiveTexture(GL_TEXTURE0 + texIndex);
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, macrocells->getAtomsTex());
		glUniform1i(_atoms, texIndex++);
	}
	
	if (volume && drawVolume)
	{
		// volume
		glActiveTexture(GL_TEXTURE0 + texIndex);
		glEnable(GL_TEXTURE_3D);
		glBindTexture(GL_TEXTURE_3D, volume->getVolumeTex());
		glUniform1i(_volume, texIndex++);
		
		// transfer function
		glActiveTexture(GL_TEXTURE0 + texIndex);
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, tf->getTFTex());
		glUniform1i(_tf, texIndex++);
		
		// delta T
		glUniform1f(_dT, dT);
		
		// color scale
		glUniform1f(_colorScale, colorScale);
	}
	
	if (clipBox)
	{
		const vec3i & mcDim = macrocells->getGridDim();

		vec3 cbMin = clipBoxMin * vec3(mcDim.x(), mcDim.y(), mcDim.z());
		vec3 cbMax = clipBoxMax * vec3(float(mcDim.x())-.1, float(mcDim.y())-.1, float(mcDim.z())-.1);
		
		glUniform3f(_clipBoxMin, cbMin.x(), cbMin.y(), cbMin.z());
		glUniform3f(_clipBoxMax, cbMax.x(), cbMax.y(), cbMax.z());
	
	}
	
	// full screen quad
	draw_quad( vWorld, cam, win_coordinates );
	
	Shader::unuse();
}

void doWiggle(double dT)
{
	Camera & camera = Camera::getInstance();
	static const double ROTATION		= .8;
	static const double ALPHA_ADVANCE	= 3.5;
	static double ALPHA_SIGN		= -1.0;
	static double alpha			= +1.0;
	
	alpha += ALPHA_SIGN * ALPHA_ADVANCE * dT;
	if (alpha < -1.0) {
		alpha = -1.0;
		ALPHA_SIGN = 1.0;
	}
	else if (alpha > 1.0) {
		alpha = 1.0;
		ALPHA_SIGN = -1.0;
	}
	
	// rotate by alpha
	camera.rotateY( alpha * ROTATION * dT );
	camera.uvw();
}


/* -------------------------------------
 * loop & event handlers
 * ------------------------------------- */
bool masterRedraw = true;
void mouseDrag(int x, int y, int dX, int dY)
{
	Camera & camera = Camera::getInstance();
	void rotateMeta(float, float);
	
	if (tf && tf->isPicked())
	{
		tf->pointerMove(x, y, dX, dY);
		masterRedraw = true;

	}
	else if (colorWheel && colorWheel->isPicked())
	{
		if (leftButton || rightButton)
		{
			colorWheel->pointerMove(x, y, dX, dY);
			masterRedraw = true;
		}
	}
	else if (!thinClient)
	{
		float _dX = +1.0f * (float) dX;
		float _dY = +1.0f * (float) dY;
		
		if (!shiftKey) {
			_dX *= 0.25;
			_dY *= 0.25;
		}
		                           
		if (controlKey || altKey)
		{
			rotateMeta(_dX, _dY);
		}
		else if (rightButton)
		{
			float __dX =  -.5f * _dX;
			float __dY =  -.5f * _dY;
			
			camera.moveCamera(vec3f(__dX, __dY, 0.0f));
			camera.updateProjection();
		}
		else if (leftButton)
		{
			rotationY += _dX;
			rotationX += _dY;
			
			camera.rotateY( _dX );
			camera.rotateU( _dY );
			camera.uvw();
		}
	}
}

void mouseWheel(int dir)
{
	Camera & camera = Camera::getInstance();
	if (altKey)
	{
		void _metaZoom(float);
		_metaZoom( dir > 0 ? 1.1f : 1.0f / 1.1f );
	}
	else if (xKey || yKey || zKey)
	{
		const float amount = 0.015;	// percent movement 
		
		// move clipbox
		vec3 & border = (shiftKey ? clipBoxMax : clipBoxMin);
		vec3 offset(xKey?1.0f:0.0f, yKey?1.0f:0.0f,zKey?1.0f:0.0f);
		offset *= float(dir) * amount;
		
		border = min(max(border + offset, vec3(0.0f)), vec3(1.0f));
		
		#ifdef DO_MPI
		
			masterNode->sendCommand("clip_box_extents", 0, 6,
				
				clipBoxMin.x(), clipBoxMin.y(), clipBoxMin.z(),
				clipBoxMax.x(), clipBoxMax.y(), clipBoxMax.z()
	
			);
		#endif
	}	
	else
	{
		// move camera back and forth
		float amount = !shiftKey ? .5f : 4.0f;
		if (dir > 0) {
			amount *= -1.0f;
		}
		camera.position += amount * camera.w;
		camera.updateProjection();	
	}			
}

void keyPress(char key)
{
	Camera & camera = Camera::getInstance();
	switch (key)
	{
	case '.':
		if (shiftKey)
		{
			static int counter = 1;
			stringstream strName; strName << tfFile << "." << counter++;
			if (tf->save( strName.str() ))
			{
				cout << "Saved transfer function to: " << strName.str() << endl;
			}
			else
			{
				cerr << "Could not save transfer function to: " << strName.str() << endl;
			}
		}
	case 'c':
		if (shiftKey)
		{
			// save camera
			camera.saveCamera( cameraFile );
		}
		else
		{
			clipBox = !clipBox;
			cout << "Clip box: " << (clipBox ? "ON" : "OFF") << endl;
			compileShaders();
			#ifdef DO_MPI
				masterNode->sendCommand("clip_box", 1, 0, clipBox ? 1 : 0);
			#endif
		}
		break;
		
	case 't':
		if (tf)
		{
			if (shiftKey) 
			{
				if (!tf->save(tfFile)) 
				{
					cerr << "Could not save transfer function to " << tfFile << endl;
				}
			}
			else
			{
				if (!tf->load(tfFile)) 
				{
					cerr << "Could not load transfer function from " << tfFile << endl;
				}
			}
		}
		break;
		
	case 'q':
		t_level = !t_level;
		cout << "T_LEVEL: " << (t_level ? "ON" : "OFF") << endl;
		compileShaders();
		break;
		
	case 'm':
		if (shiftKey)
		{
			drawMeta = !drawMeta;
		}
		else
		{
			dumpMovie = !dumpMovie;
			cout << "MOVIE recording: " << (dumpMovie ? "ON" : "OFF") << endl;
			if (videoout) {
				videoout->setRecording(dumpMovie);
			}
			
		}
		break;
	
	case 'l':
		if (shiftKey)
		{
			traversalLevel++;
		}
		else
		{
			traversalLevel--;
			if (traversalLevel < 0) {
				traversalLevel = 0;
			}
		}
		cout << "traversal level: " << traversalLevel << endl;
		break;
	
	case 'k':
		colorScale *= (shiftKey ? 2.0f : .5f);
		cout << "Color scale: " << colorScale << endl;
		#ifdef DO_MPI
			masterNode->sendCommand("color_scale", 0, 1, colorScale);
		#endif
		
		break;
	
	case 'b':
		drawBalls = !drawBalls;
		cout << "Balls rendering: " << (drawBalls ? "ON" : "OFF") << endl;
		compileShaders();
		#ifdef DO_MPI
			masterNode->sendCommand("draw_balls", 1, 0, drawBalls ? 1 : 0);
		#endif
		
		break;
		
	case 'd':
		dT *= (shiftKey ? 2.0f : .5f);
		cout << "delta T: " << dT << endl;
		#ifdef DO_MPI
			assert(masterNode);
			masterNode->sendCommand("delta_t", 0, 1, dT);
		#endif
		break;
		
	case 'r':
		if (densityRayCast) {
			densityRayCast = false;
			ballsRayCast = true;
			cout << "Ray casting balls" << endl;
		}
		else if (ballsRayCast) {
			ballsRayCast = false;
			densityRayCast = true;
			cout << "Density" << endl;
		}
		
		break;
		
	case 'w':
		wiggle = !wiggle;
		break;
		
	case 'u':
		drawTF = !drawTF;
		break;
		
	case 'g':
		if (shiftKey) {
			geometryPrecision++;
		}
		else
		{
			if (geometryPrecision > 0)
				geometryPrecision--;
		}
		cout << "Geometry precision: " << geometryPrecision << endl;
		compileShaders();
		break;
		
	case 'i':
		
		if (macrocells && macrocells->getRefMode() == MC_DIRECT)
		{
			cout << "INDEXED atom reference." << endl;
			macrocells->changeRefMode(MC_INDEXED);
		}
		else if (macrocells && macrocells->getRefMode() == MC_INDEXED)
		{
			cout << "Direct atom reference." << endl;
			macrocells->changeRefMode(MC_DIRECT);
		}
		compileShaders();
		break;
	
	case 'v':
		
		if (shiftKey && volume)
		{
			// toggle volume type
			switch (volume->getVolumeType())
			{
			case VOL_FLOAT:
				// switch to unsigned char
				cout << "Charge volume set to\t U CHAR" << endl;
				volume->setVolumeType(VOL_UCHAR);
				break;
			case VOL_UCHAR:
				cout << "Charge volume set to\t FLOAT" << endl;
				volume->setVolumeType(VOL_FLOAT);
				break;
			}
			volume->free_GLSL();
			compileShaders();
		}
		else
		{
			drawVolume = !drawVolume;
			cout << "Volume rendering\t" << (drawVolume ? "ON" : "OFF") << endl;
			compileShaders();
			#ifdef DO_MPI
				assert(masterNode);
				masterNode->sendCommand("draw_volume", 1, 0, drawVolume ? 1 : 0);
			#endif
		}
		
		break;
	
	case 's':
		skipEmpty = ! skipEmpty;
		cout << "Skip empty space: " << (skipEmpty ? "ON" : "OFF") << endl;
		compileShaders();
		break;
		
	case '1':
		if (shiftKey) {
			
			// force a recompilation of the shaders
			cout << "Reompiling shaders." << endl;
			compileShaders();
		}
		break;
	
	case '2':
		if (extraTFs.size() > 1) 
		{
			int interpTarget = currentTFInterp;
			tf->setInterpTarget(extraTFs[interpTarget]);
			currentTFInterp = (currentTFInterp + 1) % extraTFs.size();
			#ifdef DO_MPI
				masterNode->setInterpTarget(interpTarget);
			#endif
		}
		break;
		
	case '3':
		#ifdef DO_MPI
			masterNode->sendCommand("toggle_stereo", 0, 0);
			cout << "Toggle stereo." << endl;
		
		#else
			if (shiftKey) {
				enableStereoTest = !enableStereoTest;
				cout << "Stereo test pattern: " << (enableStereoTest ? "ON" : "OFF") << endl;
			}
			else
			{
				enableStereo = !enableStereo;
				cout << "Stereo: " << (enableStereo ? "ON" : "OFF") << endl;
			}
		#endif
		break;
	case ']':
		if (!shiftKey)
		{
			#ifdef DO_RIFT
				ovrRender & ovr = ovrRender::getInstance();
				float ipd = ovr.getIPD();
				ipd += 0.005 * OVR_METERS_2_GRID;	// 5 mm increments
				ovr.setIPD(ipd);
				cout << "IPD: " << ipd << endl;
			
			#else
				#ifdef DO_MPI
					float getStereoDisparity();
					void setStereoDisparity(float);
					float offset = getStereoDisparity();
					offset *= 1.1f;
					setStereoDisparity(offset);
					cout << "Stereo separation: " << offset << endl;
					masterNode->sendCommand("stereo_disparity", 0, 1, offset);
				#else
					stereoSeparation *= 1.1f;
					cout << "Stereo separation: " << stereoSeparation << endl;
				#endif
			#endif
		}
		else
		{
			resScaleX = min(MAX_RES_SCALE, resScaleX + 0.05f);
			resScaleY = min(MAX_RES_SCALE, resScaleY + 0.05f);
			#ifdef DO_MPI
				masterNode->sendCommand("res_scale", 0, 2, resScaleX, resScaleY);
			#endif
			cout << "Res scale: " << resScaleX << " x  " << resScaleY << endl;
		}
		break;
		
	case '[':
		if (!shiftKey)
		{
			#ifdef DO_RIFT
				ovrRender & ovr = ovrRender::getInstance();
				float ipd = ovr.getIPD();
				ipd -= 0.005 * OVR_METERS_2_GRID;	// 5 mm increments
				if (ipd < 0.0f) ipd = 0.0f;
				ovr.setIPD(ipd);
				cout << "IPD: " << ipd << endl;
			
			#else
				#ifdef DO_MPI
					float getStereoDisparity();
					void setStereoDisparity(float);
					float offset = getStereoDisparity();
					offset /= 1.1;
					setStereoDisparity(offset);
					cout << "Stereo separation: " << offset << endl;
					masterNode->sendCommand("stereo_disparity", 0, 1, offset);
				#else
					stereoSeparation *= 1.0f / 1.1f;
					cout << "Stereo separation: " << stereoSeparation << endl;
				#endif
			#endif
		}
		else
		{
			resScaleX = max(MIN_RES_SCALE, resScaleX - 0.05f);
			resScaleY = max(MIN_RES_SCALE, resScaleY - 0.05f);
			#ifdef DO_MPI
				masterNode->sendCommand("res_scale", 0, 2, resScaleX, resScaleY);
			#endif
			cout << "Res scale: " << resScaleX << " x  " << resScaleY << endl;
		}
		break;
	}
}

bool handleSDLEvents()
{
	SDL_Event event;
	#define IS_KEY(K) (K == event.key.keysym.sym)

	while (SDL_PollEvent(&event))
	{
		switch (event.type)
		{
		case SDL_KEYDOWN:
			if (IS_KEY(SDLK_RSHIFT) || IS_KEY(SDLK_LSHIFT))
				shiftKey = true;
			else if (IS_KEY(SDLK_RCTRL) || IS_KEY(SDLK_LCTRL))
				controlKey = true;
			else if (IS_KEY(SDLK_RALT) || IS_KEY(SDLK_LALT))
				altKey = true;
			else if IS_KEY(SDLK_x)
				xKey = true;
			else if IS_KEY(SDLK_y)
				yKey = true;
			else if IS_KEY(SDLK_z)
				zKey = true;
			else
				keyPress(event.key.keysym.sym);
			break;
		case SDL_KEYUP:
			if IS_KEY(SDLK_ESCAPE)
				return false;
			else if (IS_KEY(SDLK_RSHIFT) || IS_KEY(SDLK_LSHIFT))
				shiftKey = false;
			else if (IS_KEY(SDLK_RCTRL) || IS_KEY(SDLK_LCTRL))
				controlKey = false;
			else if (IS_KEY(SDLK_RALT) || IS_KEY(SDLK_LALT))
				altKey = false;
			else if IS_KEY(SDLK_x)
				xKey = false;
			else if IS_KEY(SDLK_y)
				yKey = false;
			else if IS_KEY(SDLK_z)
				zKey = false;
			break;
			
		case SDL_MOUSEBUTTONDOWN:
					
			// add event to queue if left/right button
			if (SDL_BUTTON_LEFT == event.button.button)
			{
				leftButton = true;
				if (tf && drawTF && tf->inWidget(event.button.x, winHeight-1 -event.button.y)) {
					tf->pointerPick(event.button.x, winHeight-1 -event.button.y);
				}
				if (colorWheel && drawColorWheel) {
					colorWheel->pointerPick(event.button.x, winHeight-1 -event.button.y);
				}


			}
			else if (SDL_BUTTON_RIGHT == event.button.button)
			{
				rightButton = true;
				
				if (tf && drawTF)
				{
					tf->rightPick(event.button.x, winHeight-1 -event.button.y);
				}
				if (colorWheel && drawColorWheel)
				{
					colorWheel->rightPick(event.button.x, winHeight-1 - event.button.y);
				}
				masterRedraw = true;
			}
			else if (SDL_BUTTON_MIDDLE == event.button.button)
			{
				middleButton = true;
			}
			else if (SDL_BUTTON_WHEELUP == event.button.button)
			{
				mouseWheel(1);	
			}
			else if (SDL_BUTTON_WHEELDOWN == event.button.button)
			{
				mouseWheel(-1);
			}
			
			break;
			
		case SDL_MOUSEBUTTONUP:
			
			if (SDL_BUTTON_LEFT == event.button.button)
			{
				leftButton = false;
				if (tf && tf->isPicked()) {
					tf->pointerUnpick();
				}
				else if (colorWheel && colorWheel->isPicked()) {
					colorWheel->pointerUnpick();
				}
			}
			else if (SDL_BUTTON_RIGHT == event.button.button)
			{
				rightButton = false;
				if (colorWheel && colorWheel->isPicked())
				{
					colorWheel->pointerUnpick();
				}
			}
			else if (SDL_BUTTON_MIDDLE == event.button.button)
			{
				middleButton = false;
			}
			break;
		
		
		case SDL_MOUSEMOTION:
			if (leftButton || rightButton)
			{
				mouseDrag( 
					(int) event.motion.x, 
					winHeight-1 -(int) event.motion.y, 
					(int) event.motion.xrel, -((int) event.motion.yrel) 
				);				
			}
			absMouse.x() = (int) event.motion.x;
			absMouse.y() = (int) event.motion.y;

			break;
		
		/*
		case SDL_VIDEORESIZE:
			winWidth = event.resize.w;
			winHeight = event.resize.h;
			SDL_SetVideoMode(winWidth, winHeight, 32, SDL_OPENGL | SDL_DOUBLEBUF | SDL_RESIZABLE | SDL_HWSURFACE);
			setupMatrix();
			
			break;
		*/
		
		case SDL_QUIT:
			return false;
		}
	}
	return true;
}

void drawCursor()
{
	static Color MOUSE_CURSOR_COLOR(0x111E8B);
	const vec2i & mouse = absMouse;
	if (true)
	{
		glPushAttrib( GL_TRANSFORM_BIT | GL_ENABLE_BIT );
		
		glMatrixMode(GL_PROJECTION);
		glPushMatrix();
		glLoadIdentity();
		glOrtho(0.0, getWinWidth(), getWinHeight(), 0.0, -1.0, 1.0);
		
		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
		glLoadIdentity();
		glTranslatef( mouse.x(), mouse.y(), 0.0 );
		glScalef(.5, .5, 1.0);
		glRotatef( -30.0, 0.0, 0.0, 1.0 );
		{
			glColor4fv( MOUSE_CURSOR_COLOR.v() );
			glBegin(GL_TRIANGLES);
			{
				glVertex2i(0, 0);
				glVertex2i(+18, 45);
				glVertex2i(-18, 45);
			}
			glEnd();
			
			glColor3f(1.0, 1.0, 1.0);
			glLineWidth(1.0);
			glBegin(GL_LINE_STRIP);
			{
				glVertex2i(0, 0);
				glVertex2i(+18, 45);
				glVertex2i(-18, 45);
				glVertex2i(0, 0);
			}
			glEnd();
		}
		glPopMatrix();
		
		glMatrixMode(GL_PROJECTION);
		glPopMatrix();
		
		glPopAttrib();
	}
}

/* -------------------------------------
 * M A I N
 * ------------------------------------- */
 
double average(vector<double> v)
{
	double total = 0.0;
	for (int i = 0; i < v.size(); i++) {
		total += v[i];
	}
	if (v.size() > 0) {
		total /= double(v.size());
	}
	return total;
}

void tfUpdate()
{
#ifdef DO_MPI
	assert(masterNode);
	masterNode->flagTFUpdated();
#endif
}

int main(int argc, char ** argv)
{
	int rank = 0, processCount = 0;
	Timer loadTimer;
	double loadTime = 0.0;
#ifdef DO_MPI
	// initialize MPI
	if (MPI_SUCCESS != MPI_Init(0, NULL))
	{
		cerr << "Could not initialize MPI." << endl;
		exit(1);
	}
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &processCount);

	#ifdef NO_SLAVE_OUTPUT
		if (rank > 0) {
			fclose(stdout);
			fclose(stderr);
		}
	#endif
#endif

	init_atom_data();
	if (argc < 2)
	{
		cerr << "USAGE: " << argv[0] << " data-file\n";
		exit(1);
	}
	parseCmdLine(argc, argv);


	
#ifdef DO_MPI
	const int RENDER_UPDATE_TAG = 1000;
	struct RenderUpdate
	{
		bool finished;
		double renderTime;
		double loadTime;
		double percentDone;
	};
	
	parseClusterConfigFile();
	
	if (rank == 0)
	{
		// initialize tracker
		cout << "Connecting to CAVE2 tracker..." << endl;
		CaveTracker::initialize(CAVE2_TRACK_HOST, CAVE2_TRACK_PORT);
		cout << "Opening master window..." << endl;
		masterNode = new MasterWindow(winWidth, winHeight);
	}
	else
	{
		// fireup render windowsi
		renderNodes = openRenderWindows(resScaleX, resScaleY, myNode);

	}
#else
	initSDL();
#endif
	initGL();
	
	// get camera
	Camera & camera = Camera::getInstance();

		
	if (thinClient && rank == 0) {
		goto skipDataLoading;
	}
	
#ifdef DEBUG	
	bool looper;
	looper = true;
	while (looper && rank != 0)
		sleep(1);
#endif

	loadTimer.start();
	if (loadSequence)
	{
		#ifdef DO_MPI
			if (offlineSequenceRender && rank > 0)
			{
				cubeSequence = new CubeSequence( dataFile, rank, processCount );
			}
			else if (offlineSequenceRender && rank == 0)
			{
				// master does not do anything in offlineSequenceRender
				cout << "\n\n\n";
				cout << "***** RENDER   JOB *****\n" << endl;
				
				int finished = 0;
				double percentDone = 0.0;
				vector<double> renderTimes;
				vector<double> loadTimes;
				
				while (finished < processCount-1)
				{
					struct RenderUpdate update;
					
					// wait for messages from render nodes
					MPI_Status status;
					MPI_Recv(&update, sizeof(RenderUpdate), MPI_UNSIGNED_CHAR, MPI_ANY_SOURCE, RENDER_UPDATE_TAG, MPI_COMM_WORLD, &status);
				
					percentDone += (1.0/double(processCount-1)) * update.percentDone;
					if (update.finished) {
						finished++;
					}
					renderTimes.push_back(update.renderTime);
					loadTimes.push_back(update.loadTime);
					
					printf("\r%.2f\%  render: %.2f sec, data: %.2f sec", 100.0*percentDone, average(renderTimes), average(loadTimes));
					fflush(stdout);
				}
				
				MPI_Barrier(MPI_COMM_WORLD);
				cout << "\n***** ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *****\n\n\n" << flush;
				MPI_Finalize();
				exit(0);
			}
			else
			{
				// not offline render mode
				cubeSequence = new CubeSequence( dataFile );
			}
		#else
			cubeSequence = new CubeSequence( dataFile );
		#endif
		
		// get the first file in the sequence
		const Timestep * t = cubeSequence->getCurrentTimestep();
		theCube = t->cube;
		macrocells = t->macrocells;
		volume = t->volume;
	}
	else
	{
		if (loadRaw)
		{
			theCube = new AtomCube;
			theCube->load_file( dataFile.c_str() );
			
			if (buildMacrocells)
			{
				macrocells = new Macrocells(
					theCube->allAtoms, 
					voxelsPerAngstrom, atomScale,
					theCube->worldMin, theCube->worldMax, macrocellsFile
				);
			}
			
			if (buildVolume)
			{
				assert(macrocells);
				volume = new ChargeDensityVolume(
					theCube->allAtoms,
					theCube->worldMin,
					
					macrocells->getVPA(),
					chargeVoxelsPerAngstrom,
					macrocells->getGridDim(),
					volumeFile
				);
			}
		}
		else
		{
			if (loadMacrocells) {
				macrocells = new Macrocells( macrocellsFile );
			}
			
			if (loadVolume) {
				assert(macrocells);
				volume = new ChargeDensityVolume( volumeFile, macrocells->getVPA() );
			}
		}
	}
	loadTime = loadTimer.getElapsedTimeInSec();

	// load and compile shaders
	compileShaders();
	cout << "Done compiling shaders..." << endl;
	
	// initialize camera
	if (rank == 0)
		initCamera();
	
	if (fileExists(cameraFile) && fixedCamera)
		if (!camera.loadCamera(cameraFile)) {
			cerr << "Could not load camera file " << cameraFile << endl;
		}

skipDataLoading:	
	// make a transfer function
	tf = new TransferFunction(tfFile, !(thinClient && rank == 0));
	extraTFs.push_back( new TransferFunction(*tf) );
	currentTFInterp = 0;
	
	// make a color wheel so that we can pick colors
	if (rank == 0) 
	{
		colorWheel = new ColorWheel(winWidth - WHEEL_WIDTH, winHeight - WHEEL_HEIGHT);
		tf->setUpdateCallback( &tfUpdate );
		#ifdef DO_MPI
			//masterNode->setCaveOffset(camera.position);
			//cout << "initial cave offset: " << camera.position << endl;
		#endif
	}
	
	// stereo framebuffers
	Framebuffer leftFB(getWinWidth(), getWinHeight()/2), rightFB(getWinWidth(), getWinHeight()/2);
	leftFB.setFloatingTexture(false);
	leftFB.setComponents(3);
	
	rightFB.setFloatingTexture(false);
	rightFB.setComponents(3);
	
	// keep track of time
	int startTick = SDL_GetTicks();
	Timer timer, sinceLastRedraw, realTime; sinceLastRedraw.start(); realTime.start(); double lastT = 0.0;
	vector<double> FPS;
	char title[200];
	double renderTime;
	
	for EVAA
	{
		bool timeForRedraw = sinceLastRedraw.getElapsedTimeInSec() > THIN_CLIENT_UPDATE_RATE;
		if (thinClient && !masterRedraw)
			timeForRedraw = false;
		
		if (!thinClient)
			timeForRedraw = true;
		
		const double newT = realTime.getElapsedTimeInSec();
		const double dT = newT - lastT;
		lastT = newT;
		
		// MASTER NODE
		// ===================
		if (rank == 0)
		{
			// read head orientation from Oculus Rift
			#ifdef DO_RIFT
				ovrRender::getInstance().update();
				camera.uvw();
			#endif

			if (timeForRedraw)
				glClear( GL_COLOR_BUFFER_BIT );	
				
			// tick transfer function
			tf->tick(dT);
				
			if (!thinClient)
			{
				// wiggle
				if (wiggle) 
					doWiggle( dT );
										
				// start measuring time and account for ray casting FPS
				timer.start();
				
				glPushAttrib( GL_ALL_ATTRIB_BITS );
				#ifdef DO_RIFT
					ovrRender::getInstance().draw();
				#else
				
					const int passes = enableStereo ? 2 : 1;
					
					for (int p = 0; p < passes; p++)
					{
						float cameraOffset = 0.0;
						if (enableStereo) 
						{
							
							Framebuffer & fb = p == 0 ? leftFB : rightFB;
							cameraOffset = (p == 0 ? -1.0f : 1.0f) * stereoSeparation * .5f;

							// bind, set viewport, and clear
							fb.bind();
							glViewport(0, 0, fb.getWidth(), fb.getHeight());
							glClear( GL_COLOR_BUFFER_BIT );
						}
												
						// offset camera
						const vec3 originalCamPos = camera.position;
						camera.position += cameraOffset * camera.u;
						
						if (densityRayCast)
							density_ray_cast();
						
						if (ballsRayCast) {
							balls_ray_cast();
						}
						
						if (enableStereo) {
							
							if (enableStereoTest) {
								glDisable(GL_TEXTURE_2D);
								glDisable(GL_TEXTURE_3D);
								glDisable(GL_BLEND);
								if (p == 0)
									// red for left
									glColor3f(1.0, 0.0, 0.0);
								else
									// cyan for right
									glColor3f(1.0, 0.0, 1.0);
								Shader::unuse();
								drawBox(0.0f, 0.0f, 1.0f, 1.0f);			

							}
							Framebuffer::unbind();
							glFinish();
						}

						// restore camera
						camera.position = originalCamPos;
					}
					
					
					if (enableStereo)
					{
						Framebuffer::unbind();
						interlaceShader.use();
						glViewport(0, 0, getWinWidth(), getWinHeight());
						
						GLint _left = interlaceShader.getUniform("left");
						GLint _right = interlaceShader.getUniform("right");
						
						glActiveTexture(GL_TEXTURE0 + 0);
						glEnable(GL_TEXTURE_2D);
						glBindTexture(GL_TEXTURE_2D, leftFB.getTexture());
						glUniform1i(_left, 0);
						
						glActiveTexture(GL_TEXTURE0 + 1);
						glEnable(GL_TEXTURE_2D);
						glBindTexture(GL_TEXTURE_2D, rightFB.getTexture());
						glUniform1i(_right, 1);
						
						// draw full screen quad
						drawTexturedBoxF(0.0f, 0.0f, 1.0f, 1.0f);			

						//draw_quad( -2 );
						Shader::unuse();
					}
					
				#endif

				glPopAttrib();
				glFinish();
			
				// calculate FPS
				FPS.push_back(1.0 / /*timer.getElapsedTimeInSec() */ dT);
				
				if (drawMeta) {
					void metaDraw();
					metaDraw();
				}
			}	// <-- draw data on master
			
			if (timeForRedraw)
			{
				// draw UI
				if (tf && drawTF) 
				{
					tf->draw();
					if (colorWheel && drawColorWheel) 
						colorWheel->draw();
				}
					
				// save movie?
				if (dumpMovie && videoout)
				{
					// draw a cursor, if we are recording video
					drawCursor();

					// make memory for new frame
					char * frameBuffer = new char[ getWinWidth() *  getWinHeight() * 3];
					
					// read frame buffer and dump it
					glReadPixels(0, 0, getWinWidth(), getWinHeight(), GL_RGB, GL_UNSIGNED_BYTE, frameBuffer);
					glFinish();
					videoout->addFrame( frameBuffer );
				}
				
				// swap master buffer
				SDL_GL_SwapBuffers();
				
				sinceLastRedraw.start();
				masterRedraw = false;				
			}			
			
			// update statistics
			// display FPS on window caption
			int deltaTicks = SDL_GetTicks() - startTick;
			if (deltaTicks >= 1000)
			{
				// average FPS
				double avgFPS = average(FPS);
				string caption;
				
			#ifdef DO_MPI
				if (thinClient)
				{
					sprintf(title, "cluster: %.2f FPS", masterNode->getClusterFPS());
				}
				else
				{
					sprintf(title, "cluster: %.2f FPS  (master: %.2f)", masterNode->getClusterFPS(), avgFPS);
				}
			#else
				sprintf(title, "%.2f FPS", avgFPS);
			#endif
				caption = string("super nanovol -- ") + title;
				SDL_WM_SetCaption(caption.c_str(), caption.c_str());
				
				// reset average
				startTick = SDL_GetTicks();
				FPS.clear();
			}
		}
	#ifdef DO_MPI
	
		// RENDER NODE
		// ===================
		else
		{
			// render windows
			glClear( GL_COLOR_BUFFER_BIT );	
			for (int i = 0; i < renderNodes.size(); i++)
				renderNodes[i]->draw();				
		
	
			if (loadSequence && offlineSequenceRender) 
			{
				RenderUpdate update;
				update.renderTime = renderTime;
				update.loadTime = loadTime;
				update.finished = false;
			
				assert(cubeSequence);
				if (!cubeSequence->next())
				{
					offlineSequenceRender = false;
					
					/* update and exit */
					update.finished = true;
					update.percentDone = 0.0;
					MPI_Send( &update, sizeof(RenderUpdate), MPI_UNSIGNED_CHAR, 0, RENDER_UPDATE_TAG, MPI_COMM_WORLD);
						
					MPI_Barrier(MPI_COMM_WORLD);
					MPI_Finalize();
					
					exit(0);
				}
				else
				{
					// send update
					update.percentDone = 1.0 / double(cubeSequence->getLength());
					MPI_Send( &update, sizeof(RenderUpdate), MPI_UNSIGNED_CHAR, 0, RENDER_UPDATE_TAG, MPI_COMM_WORLD);
					
					Timer loadT; loadT.start();
					const Timestep * t = cubeSequence->getCurrentTimestep();
					theCube = t->cube;
					macrocells = t->macrocells;
					volume = t->volume;
					loadTime = loadT.getElapsedTimeInSec();
					
					// recompile shaders
					compileShaders();
				}
			}
		}
	#endif
		
		// handle any events
		if (!handleSDLEvents())
			break;

	}	// <-- render loop
	
	return 0;
}


