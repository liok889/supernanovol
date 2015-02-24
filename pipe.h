/* -----------------------------------------------
 * Super nanovol
 * Khairi Reda, 2013
 * Electronic Visualization Laboratory
 * www.evl.uic.edu/kreda
 * 
 * GPL v2 License:
 * http://www.gnu.org/licenses/gpl-2.0.html
 * pipe.h
 *
 * -----------------------------------------------
 */
 
#ifndef _PIPE_H__
#define _PIPE_H__

#include <pthread.h>
#include <iostream>
#include <string>
#include <fstream>

using namespace std;

class PipeReader
{
public:
	PipeReader(string fileName);
	~PipeReader();
	
	void start();
	void stop();
	
private:
	static void * thread_entry(void * instance);
	void loop();

	bool			working;
	bool			opened;	
	string			filename;
	ifstream		input;
	
	pthread_t		thread;
};


#endif

