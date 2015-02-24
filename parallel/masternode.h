/* -----------------------------------------------
 * Super nanovol
 * Khairi Reda, 2013
 * Aaron Knoll, 2011-2013
 * Electronic Visualization Laboratory
 * www.evl.uic.edu/kreda
 * 
 * GPL v2 License:
 * http://www.gnu.org/licenses/gpl-2.0.html
 * masternode.h
 * 
 * -----------------------------------------------
 */
 
#ifndef _SUPER_NANOVOL_MASTER__
#define _SUPER_NANOVOL_MASTER__

#include <vector>
#include <cstdarg>
#include <pthread.h>
#include "../graphics/graphics.h"
#include "../camera.h"
#include "../Matrix44.hxx"

using namespace std;

struct SendCommand
{
	const char * command;
	vector<float> floats;
	vector<int> ints;
};

class MasterWindow
{
public:
	MasterWindow(int w, int h);
	~MasterWindow() {}
	
	// send transfer function / commands
	void sendTransferFunction();
	void sendCommand();
	
	// get and set
	double getClusterFPS() const { return clusterFPS; }
	void setHeadTracking(bool h) { headTracking = h; }
	void setCaveOffset(vec3 & offset) { caveOffset = offset; }
	void flagTFUpdated() { tf_updated = true; }
	
	// queues command for sending to render nodes
	void sendCommand(const char * command, int iCount, int fCount, ... );
	void setInterpTarget(int targetNum)	{ interpTarget = targetNum; }
private:
	int		processCount;			// number of processes in MPI world
	
	// sync thread
	static void * syncThreadEntry(void *);
	void syncThreadLoop();
	
	// tracker thread
	static void * trackerThreadEntry(void *);
	void trackerThreadLoop();
	
	// navigation
	void navigate(double dT, bool actuallyMove);
	void tickTracker(double dT);

	// settings
	bool		headTracking;			// is head tracking on?
	
	// update flags
	bool		camera_updated;	
	bool		tf_updated;

	// transfer function interpolation flags
	int		interpTarget;
	
	// navigation data
	vec4		newCamera;
	vec3		headPosition;			// current head position	
	vec3		caveOffset;			// current cave offset in space
	vec3		caveRotation;			// current Euler angles
	// ------- protected by mainLock
	Camera		camera;				// camera
	Matrix44	caveTransform;			// aggregate cave transform
							// (which will be sent to render nodes)
	Matrix44	newTransform;
	// -------
	
	// update flags
	
	// orientation
	vec3		wandOrientation;
	vec3		rotateVector;
	vec3		wandLockOrientation;
	
	// translation
	vec3		wandPosition;			// current position of wand
	vec3		moveVector;			// current movement vector
	vec3		wandLockPosition;		// position where wand was locked (ie button pressed)

	bool		wandNavigate;			// whether wand button is pressed
	bool		zeroHead;			// whether to set head position in center of cave

	// accounting
	double		clusterFPS;
	
	// pthread stuff
	pthread_t		syncThread, trackerThread;
	pthread_mutex_t		mainLock, commandQLock;
	
	// commands queue
	vector<SendCommand>	* commandQueueFront, * commandQueueBack;
	vector<SendCommand>	commandQ1, commandQ2;
};

#endif

