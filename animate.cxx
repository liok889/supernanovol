
#include <fstream>
#include <string>
#include <iostream>
#ifdef DO_MPI
#include <mpi.h>
#endif
#include <omp.h>
#include <unistd.h>
#include "data.h"
#include "atoms.h"
#include "graphics/misc.h"

using namespace std;
int expandCount = 2;
string seqFile;


// not really used but needed by data.o
float		voxelsPerAngstrom;
float		chargeVoxelsPerAngstrom;
float		atomScale;
bool		loadMacrocells;
bool		loadVolume;
bool		buildMacrocells;
bool		buildVolume;
bool		loadRaw;
bool		buildIfNeeded;
	

void parseCmdLine(int argc, char ** argv)
{
	for (int i = 1; i < argc; i++)
	{
		if (0 == strcasecmp("-expand", argv[i]) && i < argc-1)
		{
			expandCount = atoi(argv[++i]);
		}
		else if (argv[i][0] == '-')
		{
			cerr << "Unrecognized option " << argv[i] << endl;
			exit(1);
		}
		else
		{
			seqFile = argv[i];
		}
	}
	
	if (seqFile.length() == 0) {
		cerr << "Need sequence file." << endl;
		exit(1);
	}
}

float catmull_rom(float t, const float * P)
{
	static const float C[4][4] = {
		{-1.0, 3.0, -3.0, 1.0},
		{2.0, -5.0, 4.0, -1.0},
		{-1.0, 0.0, 1.0, 0.0},
		{0.0, 2.0, 0.0, 0.0}
	};
	
	float t2 = t*t;
	float t3 = t2 * t;
	float U[4] = {t3, t2, t, 1.0f};
	float O[4] = {0.0f, 0.0f, 0.0f, 0.0f};
	
	for (int i = 0; i < 4; i++)
	{
		for (int j = 0; j < 4; j++)
		{
			O[i] += U[j] * .5f * C[j][i];
		}
	}

	return O[0]*P[0] + O[1]*P[1] + O[2]*P[2] + O[3]*P[3];
}

inline float catmull_rom(float t, float x1, float x2, float x3, float x4)
{
	const float P[4] = {x1, x2, x3, x4};
	return catmull_rom( t, P );
}

bool writeXYZ(const char * filename, const Atoms & interpolated)
{
	ofstream output(filename);
	if (!output.is_open()) {
		return false;
	}
	else
	{
		// header
		output << interpolated.size() << "\n";
		output << filename << "\n";
		
		// actual atoms
		Atoms::const_iterator it = interpolated.begin();
		for (unsigned int id = 1; it != interpolated.end(); it++, id++)
		{
			const Atom & atom = *it;
			output << atom.atomType << '\t' << atom.x << ' ' << atom.y << ' ' << atom.z << '\n';  
		}
		
		output << flush;
		output.close();
		return true;
	}
}



void animate(
	const Atoms & atoms1,
	const Atoms & atoms2,
	const Atoms & atoms3,
	const Atoms & atoms4,
	Atoms & interpolated,
	float t
)
{
	omp_set_dynamic(0);
	int ncores = omp_get_max_threads();  
	omp_set_num_threads( ncores );
		
	unsigned int len = atoms1.size();
	
#pragma omp parallel for	
	for (unsigned i = 0; i < len; i++)
	{
		const Atom & a1 = atoms1[i];
		const Atom & a2 = atoms2[i];
		const Atom & a3 = atoms3[i];
		const Atom & a4 = atoms4[i];
		Atom & aM = interpolated[i];
				
		aM.x = catmull_rom(t, a1.x, a2.x, a3.x, a4.x);
		aM.y = catmull_rom(t, a1.y, a2.y, a3.y, a4.y);
		aM.z = catmull_rom(t, a1.z, a2.z, a3.z, a4.z);
	}
}

				
// data sequence
CubeSequence * sequence = NULL;


int main(int argc, char ** argv)
{
	init_atom_data();
	parseCmdLine(argc, argv);
	cout << "ANIMATE" << endl;
	cout << "\tWill expand: " << seqFile << " by " << expandCount << endl;

	int rank, processCount;
	
	// we need at least 4 in core
	setMaxIncore(4);
		
	// initialize MPI
#ifdef DO_MPI
	if (MPI_SUCCESS != MPI_Init(0, NULL))
	{
		cerr << "Could not initialize MPI." << endl;
		exit(1);
	}
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &processCount);
#endif
	
	
#ifdef DO_MPI
	if (rank == 0) 
	{
		cout << "Waiting for nodes to finish..." << endl;
		MPI_Barrier(MPI_COMM_WORLD);
		MPI_Finalize();
		exit(0);
	}
	else
#endif
	{
#ifdef DO_MPI
		sequence = new CubeSequence(seqFile, rank, processCount);
#else
		sequence = new CubeSequence(seqFile);
		rank = 0;
#endif
		
		if (sequence->getLength() < 4) 
		{
			cerr << "Need at least 4 timesteps per node to perform catmull-rum interpolation." << endl;
			exit(1);
		}
			
		if (rank > 1) 
		{
			sequence->start = sequence->start-1;
			sequence->current = sequence->start;
		}
		
		// loop through sequence
		string n1, n2, n3, n4;
		
		const AtomCube * cube1 = sequence->getCube(n1); sequence->next();
		const AtomCube * cube2 = sequence->getCube(n2); sequence->next();
		const AtomCube * cube3 = sequence->getCube(n3);
		
		// duplicate the list
		Atoms interpolated(cube1->allAtoms);
			
		while (sequence->next())
		{
			// get second cube
			const AtomCube * cube4 = sequence->getCube(n4);
			
			
			// reach out to atoms
			const Atoms & atoms1 = cube1->allAtoms;
			const Atoms & atoms2 = cube2->allAtoms;
			const Atoms & atoms3 = cube3->allAtoms;
			const Atoms & atoms4 = cube4->allAtoms;
			
			for (int i = 0; i < expandCount; i++)
			{
				const float t = float(i+1) / float(expandCount + 1);
				
				animate(atoms1, atoms2, atoms3, atoms4, interpolated, t);
								
				// write XYZ file
				char buffer[1024];
				string name = fileName(n2);
				sprintf(buffer, "%s.%d.%d.%s", n2.c_str(), expandCount, i+1, "xyz");
				writeXYZ(buffer, interpolated);
				cout << "Written: " << buffer << endl;
			}
			
			// marsh forward
			cube1 = cube2;		n1 = n2;
			cube2 = cube3;		n2 = n3;
			cube3 = cube4;		n3 = n4;
		}
	}
	
	return 0;
}

