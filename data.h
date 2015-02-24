/* -----------------------------------------------
 * Super nanovol
 * Khairi Reda, 2013
 * Electronic Visualization Laboratory
 * www.evl.uic.edu/kreda
 * 
 * GPL v2 License:
 * http://www.gnu.org/licenses/gpl-2.0.html
 * data.h
 *
 * -----------------------------------------------
 */
 
#ifndef _SUPER_NANOVOL_DATA_H__
#define _SUPER_NANOVOL_DATA_H__

#include <string>
#include <vector>
#include <list>
#include <assert.h>
#include "atoms.h"
#include "grid.h"
#include "VectorT.hxx"
#include "graphics/graphics.h"

using namespace std;

// forward class declaration
class AtomCube;
class Macrocells;
class ChargeDensityVolume;

static int MAX_INCORE_TIMESTEPS = 1;		// max number of timesteps to keep loaded in core
void setMaxIncore(int);

// a single timestep with all its associated data
struct Timestep
{
	bool hasData;
	string dataFile;
	AtomCube * cube;
	Macrocells * macrocells;
	ChargeDensityVolume * volume;
	
	void release();
	Timestep(string _cubeFile);
	~Timestep();
};

// An atoms cube: contains all atoms in single time slice
class AtomCube
{
public:
	AtomCube();
	~AtomCube();
	
	bool load_ANP_text(const char * filename);
	bool load_XYZ(const char * filename);
	bool load_DAT(const char * filename);
	bool load_file(const char * filename);
	
public:
	vec3f				worldMin, worldMax, worldMag;
	
	// atoms
	Atoms				allAtoms;
	
	// whether we have loaded atoms / density grids
	bool				hasAtoms;
};

static const float ATOM_RADII[5] = {
	1.52,				// oxygen
	2.1,				// silicon
	2.1,				// AL
	2.1,				// background AL
	1.7				// carbon
};

// CubeSequence: contains all time slices specified in a sequence
class CubeSequence
{
public:
	CubeSequence(string sequenceFile, int mpi_rank = -1, int mpi_count = -1);
	~CubeSequence();
	
	bool hasNext() const { return current < start + length - 1; }
	bool hasPrev() const { return current > start; }
	
	bool next() { if (hasNext()) { current++; return true; } else { return false; } }
	bool prev() { if (hasPrev()) { current--; return true; } else { return false; } }
	
	const Timestep * getCurrentTimestep();
	const AtomCube * getCube(string & filename);
	
	int getCurrentIndex() const { return current; }
	int getStart() const { return start; }
	int getLength() const { return length; }
 	
private:
	void load_data(Timestep * t);
	
	int			start;
	int			length;
	int			current;
	vector<Timestep*>	timesteps;
	list<Timestep*>		loadedTimesteps;
	
	// main() is our friend
	friend int main(int, char**);
};



string getAtomType(const char * t, bool ANP = false);
int getShaderIndex(int atomicNumber);
		
#endif

