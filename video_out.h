/* -----------------------------------------------
 * Super nanovol
 * Khairi Reda, 2013
 * Electronic Visualization Laboratory
 * www.evl.uic.edu/kreda
 * 
 * GPL v2 License:
 * http://www.gnu.org/licenses/gpl-2.0.html
 * video_out.h
 *
 * -----------------------------------------------
 */

#ifndef _VIDEO_OUT_H___
#define _VIDEO_OUT_H___

#include <list>
#include <string>
#include <pthread.h>
#include "Timer.h"

using namespace std;

class VideoOut
{
public:
	VideoOut(int _w, int _h, string _dumpDir, int _frameRate);
	~VideoOut();
	
	
	void addFrame(char * frame);
	void stop();
	void setRecording(bool b) { recording = b; }
private:
	
	static void * outputThreadEntry(void * p);
	void outputThreadLoop();
	
	int			w, h;
	int			frameRate;
	bool			running;
	bool			recording;
	string			dumpDir;
	pthread_t		outputThread;
	pthread_mutex_t		lock;
	Timer			sinceLastFrame;
	
	list<char *>		frames;
	char *			lastFrame;
};

#endif

