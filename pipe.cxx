/* -----------------------------------------------
 * Super nanovol
 * Khairi Reda, 2013
 * Electronic Visualization Laboratory
 * www.evl.uic.edu/kreda
 * 
 * GPL v2 License:
 * http://www.gnu.org/licenses/gpl-2.0.html
 * pipe.cxx
 *
 * -----------------------------------------------
 */
 
#include <assert.h>
#include "graphics/misc.h"
#include "pipe.h"

// defined in main.cxx
bool parseCmdLine(int argc, const char ** argv, bool skipFileName = false);

PipeReader::PipeReader(string _filename)
{
	filename = _filename;
	opened = false;
	working = false;
}

PipeReader::~PipeReader()
{
	if (opened) {
		input.close();
	}
}

void PipeReader::start()
{
	pthread_create(&thread, NULL, &PipeReader::thread_entry, this);
}

void PipeReader::stop()
{
	working = false;
	pthread_join(thread, NULL);
}

void PipeReader::loop()
{
	input.open( filename.c_str() );
	if (!input.is_open()) {
		cerr << "Could not open file: " << filename << endl;
		return;
	}
	
	
	while (working)
	{
		char buffer[2*1024];
		input.getline(buffer, 2*1024-1);
		
		
		vector<string> tokens = splitWhite(buffer);
		const char ** argv = new const char*[tokens.size()+1];
		argv[0] = NULL;
		for (int i = 0; i < tokens.size(); i++)
		{
			argv[i+1] = tokens[i].c_str();
		}
		
		parseCmdLine(tokens.size()+1, argv, true);
		delete [] argv;
	}
	
	input.close();
}

void * PipeReader::thread_entry(void * instance)
{
	PipeReader * pInstance = (PipeReader* ) (instance);
	assert (pInstance);

	pInstance->working = true;	
	pInstance->loop();
	return NULL;
}

