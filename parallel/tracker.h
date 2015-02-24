/* -------------------------------------
 * Nanovol-MPI
 * Cave2 Tracker
 * tracker.h
 *
 * -------------------------------------
 */

#ifndef _CAVE_2__TRACKER___
#define _CAVE_2__TRACKER___

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#ifndef _WIN32
#include <inttypes.h>
#endif
#include <list>
#include <string>
#include <vector>

#include "omicron.h"
#include "../Timer.h"

// conversion constants
static const float RAD_TO_DEGREE		= 57.2957795f;
static const float METERS_TO_FEET		= 3.2808399f;

using namespace std;

class CaveTracker;

// control structures
struct Sensor
{
	int			id;
	float 			x, y, z;
	float 			azim, elev, roll;
	double			timestamp;
	bool			valid;
	
	Sensor(int _id): valid(false), timestamp(0.0), id(_id) {}
	
	float getYaw() const { return azim; }
	float getPitch() const { return elev; }
	float getRoll() const { return roll; }
	
	
	bool isValid() const { return valid; }
	double howOld() const;
};

#define CAVE_MAX_BUTTONS	32
#define CAVE_MAX_VALUATORS	32

enum BUTTON_EVENT
{
	BUTTON_UP,
	BUTTON_DOWN,
	NOT_SET,
};

struct ControllerEvent
{
	int		controller;
	int		button;
	BUTTON_EVENT	event;
	
	ControllerEvent(): controller(-1), button(-1), event(NOT_SET) {}
	ControllerEvent(int _button, BUTTON_EVENT _event): controller(0), button(_button), event(_event) {}
};

struct Controller
{
	int				id;
	uint32_t			num_buttons,num_valuators;
	int32_t				button[CAVE_MAX_BUTTONS];
	float				valuator[CAVE_MAX_VALUATORS];
	
	Controller(int _id): id(_id) 
	{
		// zero buttons and valuators
		for (int i = 0; i < CAVE_MAX_BUTTONS; i++)
			button[i] = 0;
		for (int i = 0; i < CAVE_MAX_VALUATORS; i++)
			valuator[i] = 0.0f;
	}
	
	Controller(const Controller & rhs)
	: id(rhs.id), num_buttons(rhs.num_buttons), num_valuators(rhs.num_valuators)
	{
		memcpy(button, rhs.button, sizeof(int32_t)*CAVE_MAX_BUTTONS);
		memcpy(valuator, rhs.valuator, sizeof(float)*CAVE_MAX_VALUATORS);
	}
};
	

// forward class declaration
class CaveEventListener;

/* --------------------------------------------------------------
 * class CaveTracker: wraps omnicron and provides cave 2 tracking
 * --------------------------------------------------------------
 */

class CaveTracker
{
public:

	static CaveTracker * initialize(string serverName, int serverPort);
	static CaveTracker * getInstance();
	static bool initialized() { return instance != NULL; }
	
	// get head and wand position
	const Sensor * getHead() const;
	const Sensor * getWand() const;
	
	// get controller buttons
	const Controller * getController() const;
	
	double getCurrentTime() const { return currentTime; }
	bool pollEvent(ControllerEvent &);
	
	// call this repeatidly to run the tracker and receive new data
	void poll();
	
	friend class CaveEventListener;
	
private:
	// private constructor
	CaveTracker(string serverName, int serverPort);
	
	// event function
	void onEvent(const omicronConnector::EventData & e);
	
	// this is where the tracking data goes
	vector<Sensor>			sensors;
	vector<Controller>		controllers;
	list<ControllerEvent>		events;

	
	// actual omicron stuff
	omicronConnector::OmicronConnectorClient		* connector;
	omicronConnector::IOmicronConnectorClientListener	* listener;
	
	// timer
	Timer timer;
	double currentTime;
	
	// static instance
	static CaveTracker * instance;
};


/* --------------------------------------------------------------
 * class CaveEventListener: implementation of omicron listener.
 * Passes events back to CaveTracker
 * --------------------------------------------------------------
 */
 
class CaveEventListener : public omicronConnector::IOmicronConnectorClientListener
{
public:
	virtual void onEvent(const omicronConnector::EventData & e)
	{
		assert(CaveTracker::instance);
		CaveTracker::instance->onEvent(e);
	}
};



#endif

