/* -----------------------------------------------
 * Super nanovol
 * Khairi Reda, 2013
 * Electronic Visualization Laboratory
 * www.evl.uic.edu/kreda
 * 
 * GPL v2 License:
 * http://www.gnu.org/licenses/gpl-2.0.html
 * data.cxx
 *
 * -----------------------------------------------
 */

#include <cfloat>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <math.h>
#include <stdio.h>
#include "atoms.h"
#include "data.h"
#include "macrocells.h"
#include "charge_volume.h"
#include "graphics/graphics.h"
#include "graphics/misc.h"

#define FOURTEEN_MIL		14900000.0

using namespace std;

void setMaxIncore(int t)
{
	MAX_INCORE_TIMESTEPS = t;
}

Timestep::Timestep(string _dataFile)
{
	dataFile = _dataFile;
	cube		= NULL;
	macrocells	= NULL;
	volume		= NULL;
	hasData		= false;
}

void Timestep::release()
{
	if (hasData)
	{
		delete cube;
		delete macrocells;
		delete volume;
		
		cube = NULL;
		macrocells = NULL;
		volume = NULL;
		hasData = false;
	}
}
	
Timestep::~Timestep()
{
	release();	
}

AtomCube::AtomCube()
{
	hasAtoms = false;	
}

AtomCube::~AtomCube()
{
	allAtoms.clear();
	hasAtoms = false;
}

bool AtomCube::load_file(const char * filename)
{
	string extension = fileExtension(filename);
	std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
	
	if (extension == "dat")
	{
		return load_DAT(filename);
	}
	else if (extension == "xyz")
	{
		return load_XYZ(filename);
	}
	else if (extension == "xyzw")
	{
		return load_ANP_text(filename);
	}
	else
	{
		cerr << "Unknown file extension: " << extension << endl;
		return false;
	}
}

bool AtomCube::load_XYZ(const char * filename)
{
	FILE * file = fopen(filename, "r");
	if (!file) {
		cerr << "Failed to load " << filename << endl;
		return false;
	}
	
	char buffer[1024];
	unsigned int npts;
	unsigned int lineNum = 1;
	
	fscanf(file,"%d\n",&npts);
	fscanf(file, "%s\n", buffer);
	cout << "Reading .XYZ file with " << npts << " atoms." << endl;
	lineNum++;

	// min / max
	worldMin = vec3(FLT_MAX, FLT_MAX, FLT_MAX);
	worldMax = vec3(FLT_MIN, FLT_MIN, FLT_MIN);
	
	for(unsigned int i = 0; i < npts; i++, lineNum++)
	{
		char id[100];
		Atom anAtom;
		

		fscanf(file, "%s ", id);
		
		// look up type
		int atomicNumber;
		anAtom.atomType = getAtomType(id);
		if (anAtom.atomType.length() == 0 && lineNum == 2)
		{
			// ignore second line, which could be problematic at time
			continue;
		}
		else
		{
			assert(anAtom.atomType.length() > 0);
		}

		fscanf(file,  "%f %f %f\n", &anAtom.x, &anAtom.y, &anAtom.z);
		const AtomData & atomData = lookUpAtom(anAtom.atomType);
		assert(!atomData.nonexisting);
		anAtom.radius = atomData.vdw_radius;
		
		
		if (anAtom.radius < 0)
		{
			cerr << "Warning, discarding unknown atom: " << id << endl;
			break;
		}
		
		allAtoms.push_back( anAtom );
				
		// min / max taking into account Van der Waals radius
		worldMin = min(worldMin,  anAtom.xyz_minus_rad());
		worldMax = max(worldMax, anAtom.xyz_plus_rad());
	}
	worldMag = worldMax - worldMin;

	fclose(file);
	return true;
}


bool AtomCube::load_DAT(const char * filename)
{
	FILE* file = fopen(filename, "r");
	   
	if (!file)
	{
		cerr << "Failed to load " << filename << endl;
		return false;
	}
	    
	unsigned int npts;
	float r0, r1;
	fscanf(file,"%f %f\n",&r0, &r1);
	fscanf(file,"%d\n",&npts);
	cout << "Reading .DAT file with " << npts << " atoms." << endl;
  
	// min / max
	worldMin = vec3(FLT_MAX);
	worldMax = vec3(FLT_MIN);
	
	for(unsigned int i = 0; i < npts; i++)
	{
		Atom anAtom;
		int id, pid_of_idx, blah;
		
		fscanf(file,"%d %d %f %f %f %d\n", &pid_of_idx, &id, &anAtom.x,&anAtom.y,&anAtom.z, &blah);
		if (id == 1)
		{
			// Oxygen
			anAtom.atomType = "o";

		}
		else
		{
			//silicon
			anAtom.atomType = "si";
		}

		assert(anAtom.atomType.length() > 0);
		const AtomData & atomData = lookUpAtom(anAtom.atomType);
		assert(!atomData.nonexisting);
		anAtom.radius = atomData.vdw_radius;

		allAtoms.push_back( anAtom );

		// min / max
		worldMin = min(worldMin, anAtom.xyz_minus_rad());
		worldMax = max(worldMax, anAtom.xyz_plus_rad());
		
		if (i % 100000 == 0)
		{
			cout << "\r  Loading data:\t\t" << floor(.5f + 100.0f * ((float) i / (float) npts));
			cout << "%" << flush;
		}	
	}
	cout << endl;
	worldMag = worldMax - worldMin;

	fclose(file);	
	return true;
}


bool AtomCube::load_ANP_text(const char * filename)
{
	char buffer[1024];
	
	fstream input;
	input.open(filename);
	if (!input.is_open())
	{
		cerr << "Could not open: " << filename << endl;
		return false;
	}
	
	// consume the first two lines
	input.getline(buffer, 1024);
	input.getline(buffer, 1024);
	
	unsigned long counter = 0;

	// min / max
	worldMin = vec3(FLT_MAX);
	worldMax = vec3(FLT_MIN);
	
	// load all atoms in the file
	while (!input.eof())
	{	
		char t;
		unsigned long id;
		float temp, X, Y, Z;
		Atom anAtom;
		
		input >> t;
		input >> X;
		input >> Y;
		input >> Z;
		
		anAtom.x = X;
		anAtom.y = Y;
		anAtom.z = Z;
		
		// look up type
		anAtom.atomType = getAtomType(&t, true);
		assert(anAtom.atomType.length() > 0);
		anAtom.radius = lookUpAtom(anAtom.atomType).vdw_radius;

		// min / max
		worldMin = min(worldMin, anAtom.xyz_minus_rad());
		worldMax = max(worldMax, anAtom.xyz_plus_rad());

		// read temperature values
		input >> id;
		input >> temp;
		input >> temp;
		
		// print some load status
		if (counter++ % 100000 == 0)
		{
			cout << "\rLoading data:\t\t" << floor(.5f + 100.0f * ((float) counter / FOURTEEN_MIL));
			cout << "%" << flush;
		}
		
		allAtoms.push_back( anAtom );
	}
	worldMag = worldMax - worldMin;
	
	input.close();
	return true;
}


CubeSequence::CubeSequence(string seqFile, int node_rank, int node_count)
{
	// open the sequence file and read it
	ifstream input(seqFile.c_str());
	while (!input.eof())
	{
		char buffer[1024*3];
		input.getline(buffer, 1024*3-1);
		
		string dataFile = trim(string(buffer));
		if (dataFile.length() == 0) {
			continue;
		}
		timesteps.push_back( new Timestep(dataFile) );
	}
	input.close();
	
	if (node_rank >= 0 && node_count > 1)
	{
		// distribute sequnece among participating nodes
		// ignore master
		node_rank--;
		node_count--;
		
		// how many per nodes
		int perNode = timesteps.size() / node_count;
		int remaining = timesteps.size() % node_count;
		
		if (0 == remaining)
		{
			length = perNode;
			start = perNode * node_rank;
		}
		else if (node_rank < remaining)
		{
			length = perNode + 1;
			start = (perNode+1) * node_rank;
		} 
		else
		{
			length = perNode;
			start = remaining*(perNode+1) + (node_rank-remaining)*perNode;
		}		
	}
	else
	{
		start = 0;
		length = timesteps.size();
	}
	current = start;
}

CubeSequence::~CubeSequence()
{
	for (int i = 0; i < timesteps.size(); i++) {
		delete timesteps[i];
	}
}

const Timestep * CubeSequence::getCurrentTimestep()
{
	Timestep * t = timesteps[current];
	if (!t->hasData)
	{
		cout << "Loading data for " << t->dataFile << endl;
		load_data(t);
	}
	return t;
}


const AtomCube * CubeSequence::getCube(string & filename)
{
	Timestep * t = timesteps[current];
	if (t->cube == NULL)
	{
		
		if (loadedTimesteps.size() == MAX_INCORE_TIMESTEPS)
		{
			// delete the first loaded timestep
			list<Timestep*>::iterator first = loadedTimesteps.begin();
			(*first)->release();
			loadedTimesteps.erase(first);
		}
	
		// load raw atoms
		t->cube = new AtomCube;
		t->cube->load_file( t->dataFile.c_str() );
		t->macrocells = NULL;
		t->volume = NULL;
		t->hasData = true;
		
		loadedTimesteps.push_back( t );
	}
	filename = t->dataFile;
	return t->cube;
}


void CubeSequence::load_data(Timestep * t)
{
	// globals defined in main.cxx
	extern float		voxelsPerAngstrom;
	extern float		chargeVoxelsPerAngstrom;
	extern float		atomScale;
	extern bool		loadMacrocells;
	extern bool		loadVolume;
	extern bool		buildMacrocells;
	extern bool		buildVolume;
	extern bool		loadRaw;
	extern bool		buildIfNeeded;

	bool _buildMacrocells	= buildMacrocells;
	bool _buildVolume	= buildVolume;
	bool _loadMacrocells	= loadMacrocells;
	bool _loadVolume	= loadVolume;
	bool _loadRaw		= loadRaw;
	
	static const bool SAVE_DATA		= false;
	static const bool DONT_RECREATE	= false;

	if (loadedTimesteps.size() == MAX_INCORE_TIMESTEPS)
	{
		// delete the first loaded timestep
		list<Timestep*>::iterator first = loadedTimesteps.begin();
		(*first)->release();
		loadedTimesteps.erase(first);
	}
	
	string rawFile			= fileName(t->dataFile);
	string volumeFile		= rawFile + ".volume";
	string macrocellsFile		= rawFile + ".macrocells";
	
	AtomCube * theCube		= NULL;
	Macrocells * macrocells	= NULL;
	ChargeDensityVolume * volume	= NULL;

	if (DONT_RECREATE && fileExists(macrocellsFile) && _buildMacrocells) 
	{
		_loadMacrocells = true;
		_buildMacrocells = false;
	}
	if (DONT_RECREATE && fileExists(volumeFile) && _buildVolume) 
	{
		_loadVolume = true;
		_buildVolume = false;
	}
	if (!_buildVolume && !_buildMacrocells) 
	{
		_loadRaw = false;	
	}
	
	if (_loadMacrocells)
	{
		if (fileExists(macrocellsFile)) {
			// load macrocells from disk
			macrocells = new Macrocells(macrocellsFile);
		}
		else if (buildIfNeeded)
		{
			// need to build new one
			_buildMacrocells = true;
			_loadRaw = true;
		}
	}
	
	if (_loadVolume)
	{
		if (fileExists(volumeFile) && macrocells) {
			// load volume from disk
			volume = new ChargeDensityVolume( volumeFile, macrocells->getVPA() );
		}
		else if (buildIfNeeded)
		{
			// need to build new one
			_buildVolume = true;
			_loadRaw = true;
		}
	}
	
	if (_loadRaw)
	{
		// load raw atoms
		AtomCube * theCube = new AtomCube;
		theCube->load_file( t->dataFile.c_str() );
			
		if (_buildMacrocells)
		{
			macrocells = new Macrocells(
				theCube->allAtoms, 
				voxelsPerAngstrom, atomScale,
				theCube->worldMin, theCube->worldMax, 
				(SAVE_DATA ? macrocellsFile : "")
			);
		}
			
		if (_buildVolume)
		{
			assert(macrocells);
			volume = new ChargeDensityVolume(
				theCube->allAtoms,
				theCube->worldMin,
					
				macrocells->getVPA(),
				chargeVoxelsPerAngstrom,
				macrocells->getGridDim(),
				(SAVE_DATA ? volumeFile : "")
			);
		}
	}
	
	t->cube		= theCube;
	t->macrocells	= macrocells;
	t->volume	= volume;
	t->hasData	= true;
	
	loadedTimesteps.push_back( t );
}


// Given an atomic number, this tells us the index of atom data in the const
// of the shader
int getShaderIndex(int atomicNumber)
{
	switch (atomicNumber)
	{
	case 8:		// O
		return 0;
		
	case 14:	// Si
		return 1;
		
	case 13:	// Al
		return 2;
	
	case 6:		// carbon
		return 4;
	default:
		return -1;
	}
}

string getAtomType(const char * t, bool ANP)
{
	if (ANP)
	{
		// only for ANP data from Priya
		switch (t[0])
		{
		case 'A':
		case 'C':
		case 'D':
		case 'F':
			return "al";
	
		case 'G':
		case 'I':
			return "al_bg";
		
		case 'B':
		case 'E':
			return "o";
			
		case 'H':
			return "o_bg";
		default:
			return "";
		}
	}
	else
	{
		string err;

		switch (t[0])
		{
		case 'c':
		case 'C':
			// carbon
			return "c";
		case 'o':
		case 'O':
			return "o";
		
		case 's':
		case 'S':
			return "si";
		default:
			// search atom table
			for (int i = 1; i < atom_count; i++) {
				if (0 == strcasecmp(atom_table[i], t)) {
					return atom_table[i];
				}
			}
			std::cerr << "WARNING: Can not find element " << t << endl;

			return "";
			
		}
	}
}


