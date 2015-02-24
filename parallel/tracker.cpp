/* -------------------------------------
 * Nanovol-MPI
 * Cave2 Tracker
 * tracker.cpp
 *
 * -------------------------------------
 */

#include <stdio.h>
#include <math.h>
#include <iostream>
#include "tracker.h"

using namespace std;

static const int SENSOR_COUNT			= 2;
static const int HEAD_SENSOR			= 0;
static const int WAND_SENSOR			= 1;

static const int CONTROLLER_COUNT		= 1;

// minimum joystick value to register
static const float DEADZONE			= 0.1f;

//#define PRINT_TRACKER_PROGRESS 0

// array to hold all the button values (1 - down, 0 = up)
int buttons[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

double Sensor::howOld() const
{
	CaveTracker * cave2 = CaveTracker::getInstance();
	return cave2->getCurrentTime() - timestamp;
}

CaveTracker * CaveTracker::initialize(string serverName, int serverPort)
{
	assert(!instance);
	instance = new CaveTracker(serverName, serverPort);
	
	return instance;
}

CaveTracker * CaveTracker::getInstance()
{
	assert(instance);
	return instance;
}

const Sensor * CaveTracker::getHead() const
{
	return &sensors[HEAD_SENSOR];
}

const Sensor * CaveTracker::getWand() const
{
	return &sensors[WAND_SENSOR];
}

const Controller * CaveTracker::getController() const
{
	// we have one controller only, for now
	return &controllers[0];
}


CaveTracker::CaveTracker(string serverName, int serverPort)
{
	// add sensors
	for (int i = 0; i < SENSOR_COUNT; i++)
	{
		sensors.push_back( Sensor(i) );
	}
	
	// add controllers
	for (int i = 0; i < CONTROLLER_COUNT; i++)
	{
		controllers.push_back( Controller(i) );
	}	
	
	// initialize listener and connect to OInputserver
	listener = new CaveEventListener;
	connector = new omicronConnector::OmicronConnectorClient(listener);
	//int dataPort = rand() % 5000 + 7100;
	int dataPort = 8765;
	connector->connect( serverName.c_str(), serverPort, dataPort );
	
	timer.start();
	currentTime = 0.0;
}

void CaveTracker::poll()
{
	
	currentTime = timer.getElapsedTimeInSec();
	bool gotSomeData = connector->poll();
	
#ifdef PRINT_TRACKER_PROGRESS
	
	static const int printCount = 35;
	static int curPrint = 1;
	static int printDir = 1;
	static double T = currentTime;
	
	if (currentTime -T > 0.05 && gotSomeData)
	{
		printf("\r");
		for (int i = 0; i < printCount; i++)
		{
			if (i < curPrint)
				printf(".");
			else
				printf(" ");
		}
		fflush(stdout);
		
		if (printDir == 1)
		{
			curPrint++;
			if (curPrint > printCount) {
				curPrint -=2;
				printDir = 2;
			}
		}
		else if (printDir == 2)
		{
			curPrint--;
			if (curPrint < 0)
			{
				curPrint = 1;
				printDir = 1;
			}
		}
		T = currentTime;
	}
	
#endif
}


// Based on Andy's cave2twaka code
void CaveTracker::onEvent(const omicronConnector::EventData & e)
{
	float r_roll, r_yaw, r_pitch;

	r_roll = asin(2.0*e.orx*e.ory + 2.0*e.orz*e.orw);
	r_yaw = atan2(2.0*e.ory*e.orw-2.0*e.orx*e.orz , 1.0 - 2.0*e.ory*e.ory - 2.0*e.orz*e.orz);
	r_pitch = atan2(2.0*e.orx*e.orw-2.0*e.ory*e.orz , 1.0 - 2.0*e.orx*e.orx - 2.0*e.orz*e.orz);

	if (e.type == 3)
	{ 
		if (e.serviceType == 1)
		{
			Sensor * sensor = NULL;
			if (e.sourceId == 0) // head
			{
				sensor = &sensors[HEAD_SENSOR];
			}
					
			else if (e.sourceId == 1) // wand
			{
				sensor = &sensors[WAND_SENSOR];
			}
			else // unknown source ID 
			{
				sensor = NULL;
			}
			
			// update sensor
			if (sensor != NULL)
			{
				sensor->x = e.posx * METERS_TO_FEET;
				sensor->y = e.posy * METERS_TO_FEET;
				sensor->z = e.posz * METERS_TO_FEET;
				
				sensor->elev = r_pitch;
				sensor->azim = r_yaw;
				sensor->roll = r_roll;
			        
				sensor->timestamp = currentTime;
				sensor->valid = true;
			}
			        
		}
		else if (e.serviceType == 7) 
		{
			// seems like the buttons are getting echoed here - seems odd
		}
		else // unknown service type
		{
		}

	}
	else if (e.type == 5) // button(s) DOWN
	{ 
		int counter;
		unsigned int i;
		for(counter=0; counter < 16; counter++)
		{
			i=pow(2, counter);

			if (e.flags & i) {
				buttons[counter] = 1;
				events.push_back(ControllerEvent(counter, BUTTON_DOWN));
			}
			
		}
	}
	else if (e.type == 6) // button(s) UP
	{ 
		int counter;
		unsigned int i;
		for(counter=0; counter < 16; counter++)
		{
			i=pow(2, counter);
			if (e.flags & i) 
			{
				buttons[counter] = 0;
				events.push_back(ControllerEvent(counter, BUTTON_UP));
			}
		}
	}

	else // unknown type
	{
	}


	// look for any extra data items which are the joystick values in this case
	Controller * controller = &controllers[0];
	if (e.extraDataItems)
	{
		if (e.extraDataType == 1)
		{
			//array of floats
			float *ptr = (float*)(e.extraData);
			for (int k=0; k<e.extraDataItems; k++)
			{
				//printf("%s     val %2d: [%6.3f]\n", textColor, k, ptr[k]);
				
				// set the cave joystick to be the first joystick
				if (k == 0) // left and right
				{
				    if (fabs(ptr[k]) > DEADZONE)
					controller->valuator[0] =  ptr[k];
				    else
					controller->valuator[0] = 0.0;
				}
				if (k == 1) // forward and backward (which are reversed by default)
				{
				    if (fabs(ptr[k]) > DEADZONE)
					controller->valuator[1] = - ptr[k];
				    else
				      controller->valuator[1] = 0.0;
				}
			}
		}
	}

	/*
	// show the button states
	strcpy(textColor, white);
	//		printf("%s \033[14;0H Button values:", textColor);
	printf("%s \033[14;0H 1  2  3  4  5  6  7  8  9  10 11 12 13 14 15 16\n", textColor);
	
	int i;
	for (i=0; i < 16; i++)
	{
		if (buttons[i] == 0)
			printf("%s up", white);
		else
			printf("%s dn", green);
	}
	*/

	// for cavelib use xbox controller X(3), Y(7), and B(2) for the 3 CAVE wand buttons for now
	//controller->controller.button[0] = buttons[2]; // arrays start at 0, buttons start at 1 so subtract 1 !!!
	//controller->controller.button[1] = buttons[6];

	//controller->controller.button[2] = buttons[1];

	// for cavelib use ps3 small controller use d-pad left-13 up-11 and right-14 for the 3 CAVE wand buttons for now
	controller->button[0] = buttons[2]; // arrays start at 0, buttons start at 1 so subtract 1 !!!
	controller->button[1] = buttons[10];
	controller->button[2] = buttons[13];
	controller->button[10] = buttons[9];

	// move the cursor somewhere out of the way
	// printf("\033[1;1H\n");
}	

bool CaveTracker::pollEvent(ControllerEvent & e)
{
	if (events.size() == 0) 
	{
		return false;
	}
	else
	{
		e = events.front();
		events.pop_front();
		return true;
	}
}

// initialize static variables
CaveTracker * CaveTracker::instance = NULL;
