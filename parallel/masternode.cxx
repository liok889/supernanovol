/* -----------------------------------------------
 * Super nanovol
 * Khairi Reda, 2013
 * Aaron Knoll, 2011-2013
 * Electronic Visualization Laboratory
 * www.evl.uic.edu/kreda
 * 
 * GPL v2 License:
 * http://www.gnu.org/licenses/gpl-2.0.html
 * masternode.cxx
 * 
 * -----------------------------------------------
 */

#include <mpi.h>
#include <pthread.h>
#include <string.h>
#include "cluster.h"
#include "masternode.h"
#include "tracker.h"
#include "../main.h"
#include "../Timer.h"
#include "../tf.h"

MasterWindow::MasterWindow(int winWidth, int winHeight)
{
	clusterFPS = 0.0;
	headTracking = true;
	camera_updated = false;
	tf_updated = false;
	interpTarget = -1;
	
	commandQueueFront = &commandQ1;
	commandQueueBack  = &commandQ2;
	
	// open SDL window
	int error;
	error = SDL_Init(SDL_INIT_VIDEO);
	if (error < 0)
	{
		cerr << "Could not initialize SDL." << endl;
		exit(1);
	}	

	int flags = SDL_OPENGL | SDL_DOUBLEBUF | /* SDL_RESIZABLE | */ SDL_HWSURFACE;
	if (!SDL_SetVideoMode(winWidth, winHeight, 32, flags))
	{
		cerr << "Could not open SDL window." << endl;
		exit(1);
	}
	
	// get number of processes within the MPI system
	MPI_Comm_size(MPI_COMM_WORLD, &processCount);
		
	// init navigation flags and settings
	moveVector = vec3(0.0f);
	rotateVector = vec3(0.0f);
	
	wandLockPosition = vec3(0.0f);
	wandLockOrientation = vec3(0.0f);
	
	headPosition= vec3(0.0);
	wandNavigate = false;
	zeroHead = false;
	
	caveOffset = vec3(0.0);
	caveRotation = vec3(0.0);
	caveTransform.identity();
	newTransform.identity();
	

	// initialize locks
	pthread_mutex_init(&mainLock, NULL);
	pthread_mutex_init(&commandQLock, NULL);
	
	// create new threads to perform synching and tracking works
	pthread_create(&syncThread, NULL, &MasterWindow::syncThreadEntry, this);
	pthread_create(&trackerThread, NULL, &MasterWindow::trackerThreadEntry, this);	
}

void MasterWindow::syncThreadLoop()
{
	extern bool noSync;		// defined in main.cxx
	static const double UPDATE_RATE = 1.0/15.0;
	
	unsigned int frameNumber = 0;
	unsigned char buffer[1024*2];	
	vector<double> FPS;
	Timer timer, sinceLastUpdate, realTime;
	
	sinceLastUpdate.start();
	realTime.start();
	
	double currentTime, lastTime = 0.0, deltaTime = 0.0;
	
	// sync structure
	SyncBlock sync;
	
	for (;;)
	{
		// get main lock
		pthread_mutex_lock(&mainLock);

		// copy sync information
		sync.frameNumber = frameNumber;
		sync.deltaTime = deltaTime;
		sync.currentTime = currentTime;
		sync.interpTarget = interpTarget;
		camera.marshal(sync.camera);
		memcpy(sync.caveTransform, &caveTransform, sizeof(Matrix44));

		// reset interp target
		interpTarget = -1;
				
		// release lock
		pthread_mutex_unlock(&mainLock);

		// measure time needed to render 
		timer.start();
	
		// bcast and sync
		if (!noSync)
		{			
			MPI_Bcast( &sync, sizeof(SyncBlock), MPI_UNSIGNED_CHAR, 0, MPI_COMM_WORLD);
			MPI_Barrier(MPI_COMM_WORLD);
			
			// another barrier for sync
			MPI_Barrier(MPI_COMM_WORLD);
		}
		
		// calc elapsed time
		timer.stop();
		FPS.push_back( 1.0 / timer.getElapsedTimeInSec() );
		if (FPS.size() > 30) 
		{
			double total = 0.0;
			for (int i = 0; i < FPS.size(); i++) 
			{
				total += FPS[i];
			}
			total /= (double) FPS.size();
			clusterFPS = total;
			FPS.clear();
		}
		
		if (sinceLastUpdate.getElapsedTimeInSec() > UPDATE_RATE)
		{
			if (tf_updated)
			{
				sendTransferFunction();
				tf_updated = false;
			}
			
			pthread_mutex_lock(&commandQLock);
			
			// swap
			vector<SendCommand> & commandQueue = *commandQueueFront;
			commandQueueFront = commandQueueBack;
			commandQueueBack = &commandQueue;
			
			pthread_mutex_unlock(&commandQLock);
			
			// take only the last command for each command type			
			map<string, const SendCommand *> cmdReduce;
			for (int j = 0; j < commandQueue.size(); j++) {
				cmdReduce[ commandQueue[j].command ] = &commandQueue[j];
			}
			
			map<string, const SendCommand*>::const_iterator it = cmdReduce.begin();
			for (; it != cmdReduce.end(); it++)
			{
				const SendCommand & command = *(it->second);
				
				int strLen = strlen(command.command)+1;
				int iCount = command.ints.size();
				int fCount = command.floats.size();
				unsigned char * ptr = buffer;
				
				memcpy(ptr, &strLen, sizeof(int));		ptr += sizeof(int);
				memcpy(ptr, &iCount, sizeof(int));		ptr += sizeof(int);
				memcpy(ptr, &fCount, sizeof(int));		ptr += sizeof(int);
				
				// copy the command string
				memcpy(ptr, command.command, strLen);		ptr += strLen;
				
				// copy integers
				for (int i = 0; i < iCount; i++)
				{
					int x = command.ints[i];
					memcpy(ptr, &x, sizeof(int));
					ptr += sizeof(int);
				}
				
				// copy floats
				for (int i = 0; i < fCount; i++)
				{
					float x = command.floats[i];
					memcpy(ptr, &x, sizeof(float));
					ptr += sizeof(float);
				}
				
				// send to all render nodes
				int sendSize = ptr - buffer;
				for (int i = 1; i < processCount; i++)
				{
					MPI_Send( (void*) buffer, sendSize, MPI_UNSIGNED_CHAR, i, COMMAND_TAG, MPI_COMM_WORLD);
				}
			}
			commandQueue.clear();
			sinceLastUpdate.start();
		}
		
		frameNumber++;
		
		// advance real time and measure delta time
		currentTime = realTime.getElapsedTimeInSec();
		deltaTime = currentTime - lastTime;
		lastTime = currentTime;
				
	}	// for ever
}

void MasterWindow::trackerThreadLoop()
{
	static const double TRACKER_HZ = 80.0;
	static const double TRACKER_T = 1.0 / TRACKER_HZ;
	
	
	Timer timer;
	timer.start();
	double lastT = 0.0;
	double dT = 0.0;
	
	for (;;)
	{
		bool ticked = false;
		if (dT >= TRACKER_T)
		{
			tickTracker(dT);
			ticked = true;
			
			if (this->camera_updated)
			{
				pthread_mutex_lock( &mainLock );
				
				// get latest camera position
				camera.position.x() = newCamera.x();
				camera.position.y() = newCamera.y();
				camera.position.z() = newCamera.z();
				
				
				// caveTransform should be in row major order
				// (newTransform is Matrix44, which internally is column major order)
				Matrix44::transpose( caveTransform, newTransform );
				
				camera_updated = false;
				pthread_mutex_unlock( &mainLock );
			}
			
		}
		
		double thisT = timer.getElapsedTimeInSec();
		dT = (ticked ? 0.0 : dT) + thisT - lastT;
		lastT = thisT;
	}
}

void * MasterWindow::trackerThreadEntry(void * p)
{
	MasterWindow * win = (MasterWindow *) p;
	win->trackerThreadLoop();
	return NULL;
}

void * MasterWindow::syncThreadEntry(void * p)
{
	MasterWindow * win = (MasterWindow *) p;
	win->syncThreadLoop();
	return NULL;
}

void MasterWindow::navigate(double dT, bool actuallyMove)
{
	const static float ALPHA		= 8.0f;
	const static float MOVE_SPEED		= 2.9f;
	const static float ROT_SPEED		= .8f * 15.0f * 3.14f / 180.0f;
	const static float MAX_ROT		= 2.5f * 0.76f * 30.0f * 3.14f / 180.0f;

	// interpolation factor
	double alpha = ALPHA * dT;
	
	// rotation
	// ========
	vec3 wandRotOffset = wandOrientation - wandLockOrientation;
	
	// limit the amount of rotation
	
	wandRotOffset.x() = wandRotOffset.x() > MAX_ROT ? MAX_ROT : wandRotOffset.x() < -MAX_ROT ? -MAX_ROT : wandRotOffset.x();
	wandRotOffset.y() = wandRotOffset.y() > MAX_ROT ? MAX_ROT : wandRotOffset.y() < -MAX_ROT ? -MAX_ROT : wandRotOffset.y();
	wandRotOffset.z() = wandRotOffset.z() > MAX_ROT ? MAX_ROT : wandRotOffset.z() < -MAX_ROT ? -MAX_ROT : wandRotOffset.z();
	

	rotateVector = alpha * wandRotOffset + (1.0 - alpha) * rotateVector;
	
	// rotate
	vec3 rotationalOffset = (ROT_SPEED * dT) * rotateVector;
	caveRotation += rotationalOffset;
	
	// make a rotation matrix
	Matrix44 mYaw, mPitch, mRoll;
	mPitch.	Xrotation( caveRotation.x() );
	mYaw.	Yrotation( caveRotation.y() );
	mRoll.	Zrotation( caveRotation.z() );
		
	
	Matrix44 temp, caveComposed, caveRotationMatrix;
	caveRotationMatrix.identity();

	// yaw then pitch then roll
	Matrix44::multiply( temp, mPitch, mRoll );
	Matrix44::multiply( caveRotationMatrix, temp, mYaw );
	caveComposed = caveRotationMatrix;
	
	// translation
	// ===========
	vec3 wandOffset = wandPosition - wandLockPosition;
	moveVector = alpha * wandOffset + (1.0 - alpha) * moveVector;
	//cout << "moveVector X: " << moveVector.x() << ", " << moveVector.z() << endl;
	
	// translation subject to current rotation
	vec3 translationVector;
	MatrixTransformColMajor( translationVector, caveRotationMatrix, moveVector );
	vec3 offset = (MOVE_SPEED * dT) * translationVector;	
	caveOffset += offset;

	// transpose the composed matrix to get row major Cave transform
	caveComposed.translate( caveOffset );
	newTransform = caveComposed;
}

void MasterWindow::tickTracker(double dTime)
{
	static const double SENSOR_EXPIRE_TIME = 4.0;
	bool cameraUpdated = false;
	
	CaveTracker * tracker = CaveTracker::getInstance();
	tracker->poll();
	
	// get head position and update camera with it
	const Sensor * head = tracker->getHead();
	bool hasHead = false;
	if (head->isValid()  && head->howOld() < SENSOR_EXPIRE_TIME )
	{
		headPosition.x() = head->x * FEET_2_GRID / 1.0f;
		headPosition.y() = head->y * FEET_2_GRID / 1.0f;
		headPosition.z() = head->z * FEET_2_GRID / 1.0f;
		cameraUpdated = true;
		hasHead = true;
	}

	const Sensor * wand = tracker->getWand();
	if (wand->isValid() && wand->howOld() < SENSOR_EXPIRE_TIME )
	{
		wandPosition.x() = wand->x * FEET_2_GRID;
		wandPosition.y() = wand->y * FEET_2_GRID;
		wandPosition.z() = wand->z * FEET_2_GRID;
		
		wandOrientation.x() = wand->getPitch();
		wandOrientation.y() = wand->getYaw();
		wandOrientation.z() = wand->getRoll();
	}

	const Controller * controller = tracker->getController();
	
	// --------------------
	// Navigate button
	// --------------------	
	if (controller->button[10] != 0)
	{
		if (!wandNavigate)
		{
			wandNavigate = true;
			
			// reset position / orientaiton locks
			wandLockPosition = wandPosition;
			wandLockOrientation = wandOrientation;
			
			moveVector.x() = 0.0f;
			moveVector.y() = 0.0f; 
			moveVector.z() = 0.0f;
			
			rotateVector.x() = 0.0f;
			rotateVector.y() = 0.0f;
			rotateVector.z() = 0.0f;
		}
		navigate(dTime, true);
		cameraUpdated = true;
	}
	else
	{
		wandNavigate = false;
	}
	
	// --------------------
	// View reset button
	// --------------------
	if (controller->button[0] != 0)
	{
		caveOffset[0] = caveOffset[1] = caveOffset[2] = 0.0f;
		caveRotation[0] = caveRotation[1] = caveRotation[2] = 0.0f;
		wandLockOrientation = wandOrientation;
		wandLockPosition = wandPosition;
		rotateVector = vec3(0.0f);
		moveVector = vec3(0.0f);
		
		navigate(dTime, true);
		cameraUpdated = true;
	}
	
	if (cameraUpdated)
	{	
		// assume a person 5ft tall standing in the middle of the CAVE
		static vec4 theCamera(0.0f, 5.0f * FEET_2_GRID, 0.0f, 1.0f);
		
		if (zeroHead)
		{
			// reset head position to the middle of the CAVE again
			theCamera.x() = 0.0f;
			theCamera.y() = 5.0f * FEET_2_GRID;
			theCamera.z() = 0.0f;
			zeroHead = false;
		}
		
		if (headTracking)
		{
			theCamera.x() = headPosition.x();
			theCamera.y() = headPosition.y();
			theCamera.z() = headPosition.z();
		}

		// transform the camera according to the cave transform
		Matrix44::transform(newCamera, caveTransform, theCamera );
			
		// update flags
		this->camera_updated = true;
	}
	
	// pool other events
	ControllerEvent e;
	while (tracker->pollEvent(e))
	{
		extern float colorScale;
		extern float dT;
		extern int currentTFInterp;
		extern vector<TransferFunction*> extraTFs;
		
		if (e.event != BUTTON_UP)
			continue;
		
		switch (e.button)
		{
		case 10:
			colorScale *= 1.3f;
			sendCommand("color_scale", 0, 1, colorScale);
			break;
		case 11:
			colorScale /= 1.3f;
			sendCommand("color_scale", 0, 1, colorScale);
			break;
		
		case 12:
			dT /= 1.3f;
			sendCommand("delta_t", 0, 1, dT);
			break;
		case 13:
			dT *= 1.3f;
			sendCommand("delta_t", 0, 1, dT);
			break;
		case 8:
			{
				int interpTarget = currentTFInterp;
				getCurrentTF()->setInterpTarget(extraTFs[interpTarget]);
				currentTFInterp = (currentTFInterp + 1) % extraTFs.size();
				setInterpTarget(interpTarget);
			}
			break;
		
		default:
			cout << "button: " << e.button << endl;
			break;
		}
	}
}

void MasterWindow::sendTransferFunction()
{
	static float * buffer = new float[100];
	int count = 100;
	getCurrentTF()->marshal(buffer, count);
	
	for (int dest = 1; dest < processCount; dest++)
	{
		MPI_Send( buffer, count*sizeof(float), MPI_UNSIGNED_CHAR, dest, TRANSFER_FUNCTION_UPDATE_TAG, MPI_COMM_WORLD);
	}
}

void MasterWindow::sendCommand(const char * _command, int iCount, int fCount, ... )
{
	va_list		arguments;
	va_start(arguments, iCount+fCount);

	SendCommand command;
	command.command = _command;
	
	// copy integers
	for (int i = 0; i < iCount; i++)
	{
		int x = va_arg(arguments, int);
		command.ints.push_back(x);
	}
	
	// copy floats
	for (int i = 0; i < fCount; i++)
	{
		float x = (float) va_arg(arguments, double);
		command.floats.push_back(x);
	}
	va_end(arguments);
	
	pthread_mutex_lock(&commandQLock);
	commandQueueFront->push_back(command);
	pthread_mutex_unlock(&commandQLock);
}

