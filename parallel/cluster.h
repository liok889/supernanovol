/* -----------------------------------------------
 * Super nanovol
 * Khairi Reda, 2013
 * Aaron Knoll, 2011-2013
 * Electronic Visualization Laboratory
 * www.evl.uic.edu/kreda
 * 
 * GPL v2 License:
 * http://www.gnu.org/licenses/gpl-2.0.html
 * cluster.h
 * 
 * -----------------------------------------------
 */
#ifndef _SUPER_NANOVOL_CLUSTER_H___
#define _SUPER_NANOVOL_CLUSTER_H___

#include <vector>
#include <map>
#include <list>
#include "../graphics/misc.h"
#include "../Matrix44.hxx"
#include "../VectorT.hxx"
#include "../camera.h"

using namespace std;

// MPI message tags
static const int DATA_TRANSFORM_TAG			= 2;
static const int TEST_TAG				= 3;
static const int TRANSFER_FUNCTION_UPDATE_TAG		= 4;
static const int PARAMETERS_TAG			= 5;
static const int CAMERA_UPDATE_TAG			= 6;
static const int META_TAG				= 7;
static const int STATS_UPDATE_TAG			= 8;
static const int COMMAND_TAG				= 9;

const float FEET_2_GRID				= 2.0;		// feet to grid units


// a function to transform V according to Matrix (assumes a row major order)
void MatrixTransform(vec4 & out, const Matrix44 & matrix, const vec4 & v);
void MatrixTransform(vec3 & out, const Matrix44 & matrix, const vec3 & v);
void MatrixTransformColMajor(vec3 & out, const Matrix44 & matrix, const vec3 & v);

class GLWindow;

struct RenderSurface
{
	vec3			w_bottomLeft;
	vec2i			p_bottomLeft;
	vec2			n_bottomLeft,		n_bottomRight;
	
	vec3			w_topRight;
	vec2i			p_topRight;
	vec2			n_topRight,		n_topLeft;
	
	// we derive those in calculate UVW
	bool brSet, tlSet;
	vec3			w_bottomRight;
	vec3			w_topLeft;
	
	// camera U V W for this surface
	vec3			U, V, W;
	
	const vec3 & bL() const { return w_bottomLeft; }
	const vec3 & bR() const { return w_bottomRight; }
	const vec3 & tL() const { return w_topLeft; }
	const vec3 & tR() const { return w_topRight; }

	vec3 & bL()  { return w_bottomLeft; }
	vec3 & bR()  { return w_bottomRight; }
	vec3 & tL()  { return w_topLeft; }
	vec3 & tR()  { return w_topRight; }
	
	RenderSurface(): brSet(false), tlSet(false) {}
	
	void calculateOposingCorners()
	{
		if (!brSet)
		{
			w_bottomRight = vec3(
				w_topRight.x(), 
				w_bottomLeft.y(), 
				w_topRight.z()
			);
			brSet = true;
		}
		
		if (!tlSet)
		{
			w_topLeft = vec3(
				w_bottomLeft.x(),
				w_topRight.y(),
				w_bottomLeft.z()
			);
			tlSet = true;
		}
	}
	
	void calculateUVW()
	{	
		calculateOposingCorners();
		
		U = w_bottomRight - w_bottomLeft;
		V = w_topLeft - w_bottomLeft;
		
		// this is a left-handed coordinate system (W from camera looking towards screen)
		W = cross(V,U);
		
		U.normalize();
		V.normalize();
		W.normalize();
	}
	
	void scaleWorldCorners(double scale)
	{
		w_topRight *= scale;
		w_bottomLeft *= scale;
	}
	
	RenderSurface transform(const Matrix44 & caveTransform) const;	
/*
private:
	vec3 & bL() { return w_bottomLeft; }
	vec3 & bR() { return w_bottomRight; }
	vec3 & tL() { return w_topLeft; }
	vec3 & tR() { return w_topRight; }
*/
	
};

struct RenderWindow
{
	// window dimensions
	int			width, height;
	
	// window offset (pixels) relative to top left? corner of desktop
	int 			x, y;
	
	// full screen?
	bool			fullscreen;

	// which X server to launch this window on
	string			Xserver;
	
	// actual window object
	GLWindow *		theWindow;
	
	// surfaces
	vector<RenderSurface>	surfaces;
	
	RenderWindow(): fullscreen(false), theWindow(NULL), width(0), height(0) {}
};

// Cluster configuration
// ======================
struct Node
{
	string			hostname;
	vector<RenderWindow>	windows;
};


typedef vector<Node> NodeVector;
struct ClusterConfig
{
	map<string, NodeVector>		nodeMap;
	NodeVector				nodeList;
};


// Sync structure
// ==============
struct SyncBlock
{
	double		deltaTime;
	double		currentTime;
	
	int		interpTarget;
	
	unsigned int	frameNumber;
	unsigned char	camera[ Camera::MARSHAL_SIZE ];
	unsigned char	caveTransform[ sizeof(Matrix44) ];
	
	SyncBlock() 
	{
		interpTarget = -1;
	}
};

const ClusterConfig & getClusterConfig();
vector<GLWindow*> openRenderWindows(float resScaleX, float resScaleY, Node *& myNode);
void parseClusterConfigFile();
void printClusterConfig();
void setConfFile(string _conf);
#endif

