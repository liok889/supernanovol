/* -----------------------------------------------
 * Super nanovol
 * Khairi Reda, 2013
 * Electronic Visualization Laboratory
 * www.evl.uic.edu/kreda
 * 
 * GPL v2 License:
 * http://www.gnu.org/licenses/gpl-2.0.html
 * video_out.cxx
 *
 * -----------------------------------------------
 */

#include <unistd.h>
#include <assert.h>
#include "Timer.h"
#include "video_out.h"
#include "graphics/image.h"

VideoOut::VideoOut(int _w, int _h, string _dumpDir, int _frameRate)
{
	w = _w;
	h = _h;
	dumpDir = _dumpDir;
	frameRate = _frameRate;
	
	running = false;
	recording = false;
	lastFrame = NULL;
	
	sinceLastFrame.start();
	pthread_mutex_init(&lock, NULL);
	assert(0 == pthread_create(&outputThread, NULL, &VideoOut::outputThreadEntry, this));
}

VideoOut::~VideoOut()
{
	//stop();
}

void VideoOut::outputThreadLoop()
{
	Timer t;
	t.start();
	
	const double frameTimeUSec = 1000000.0 / double(frameRate);
	unsigned int frameNum = 1;
	char filename[100];	
	
	Timer internal;
	double timeAdjust = 0.0;
	
	while (running)
	{
		usleep((useconds_t) max(0.0, frameTimeUSec - timeAdjust));
		timeAdjust = 0.0;
		
		// skip recroding
		if (!recording)
			continue;
		
		// keep track of time spent working / saving
		internal.start();
		
		// get a frame and save it
		bool newFrame = false;
		pthread_mutex_lock(&lock);
		
		if (frames.size() != 0) 
		{
			newFrame = true;
			delete [] lastFrame;
			lastFrame = frames.front();
			frames.pop_front();
		}
		char * frame = lastFrame;
		
		pthread_mutex_unlock(&lock);
		
		if (newFrame)
			image_flip(w, h, 3, 1, lastFrame);
		
		if (frame)
		{
			// save it to disk		
			sprintf(filename, "frame-%05d.jpg", frameNum);
			string path = dumpDir + "/" + filename;
			
			// write the file here
			image_write_jpg( path.c_str(), w, h, 3, 1, frame);
			
			frameNum++;
		}
		
		timeAdjust = internal.getElapsedTimeInMicroSec();
	}
}

void VideoOut::addFrame(char * frame)
{
	static double frameTime = 1.0 / double(frameRate); 
	if (sinceLastFrame.getElapsedTimeInSec() < frameTime)
	{
		// discard
		delete [] frame;
	}
	else
	{
		sinceLastFrame.lap();
		
		pthread_mutex_lock(&lock);
		frames.push_back(frame);
		pthread_mutex_unlock(&lock);
	}
}

void VideoOut::stop()
{
	if (running) 
	{
		running = false;
		pthread_join(outputThread, NULL);
	}
	
}

void * VideoOut::outputThreadEntry(void * p) 
{
	VideoOut * me = (VideoOut*) p;
	me->running = true;
	me->recording = true;
	me->outputThreadLoop();
	
	return NULL; 
}

