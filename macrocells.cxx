/* -----------------------------------------------
 * Super nanovol
 * Khairi Reda, 2013
 * Electronic Visualization Laboratory
 * www.evl.uic.edu/kreda
 * 
 * GPL v2 License:
 * http://www.gnu.org/licenses/gpl-2.0.html
 * macrocells.cxx
 *
 * -----------------------------------------------
 */

#include <assert.h>
#include <math.h>
#include "atoms.h"
#include "data.h"
#include "macrocells.h"

gpuAtom & gpuAtom::operator=(const Atom & anAtom)
{
	this->x = anAtom.x;
	this->y = anAtom.y;
	this->z = anAtom.z;
	
	int atomicNumber;
	
	assert( anAtom.atomType.length() > 0);
	atomicNumber = lookUpAtom( anAtom.atomType ).atomic_number;
	assert(atomicNumber > 0);
	this->index = getShaderIndex(atomicNumber);
	assert(this->index >= 0);
	
	return *this;
}

Macrocells::Macrocells(const Atoms & vAtoms, float vpa, float _atomScale, vec3f worldMin, vec3f worldMax, string saveFile)
{
	refMode = MC_REF_MODE_DEFAULT;
	hasGPUData = false;
	voxelsPerAngstrom = vpa;
	atomScale = _atomScale;
	
	cout << "Building macrocells..." << endl;

	// calculate integer square root
	atomsCount = vAtoms.size();
	atomsSqrt = ceil(sqrt(atomsCount));
	cout << "\t atomsCount: " << atomsCount << ", sqrt: " << atomsSqrt << ", atom scale: " << atomScale << endl; 
	
	// allocate memory for all atoms
	const float fAtomsCount = (float) atomsCount;
	allAtoms = new gpuAtom[ atomsSqrt * atomsSqrt ];
	
	vec3 worldMag = worldMax - worldMin;
	gridDim.x() = (int) ceil( worldMag.x() * vpa );
	gridDim.y() = (int) ceil( worldMag.y() * vpa );
	gridDim.z() = (int) ceil( worldMag.z() * vpa );
	vec3f gridDimMinusOne( gridDim.x() - 1, gridDim.y() - 1, gridDim.z() - 1);
	
	// allocate memory for all temp macrocells
	tempMacrocells.resize(gridDim.x(), gridDim.y(), gridDim.z());
	macrocells.resize(gridDim.x(), gridDim.y(), gridDim.z());
	
	if (tempMacrocells.data && macrocells.data)
	{
		cout << "\t mem allocated." << endl;
	}
	else
	{
		cerr << "\t Could not allocate memory!" << endl;
		exit(1);
	}
	
	cout << "\t looking at all atoms..." << flush;
	
	// loop through all atoms
	size_t counter = 0;
	size_t indicesSize = 0;
	size_t maxMC = 0;
	
	for (size_t i = 0; i < atomsCount; i++, counter++)
	{
		// copy atom
		allAtoms[i] = vAtoms[i];
		gpuAtom & a = allAtoms[i];

		// radius == Van der Waals radius		
		float radius = vAtoms[i].radius;
		
		// start from worldMin
		a.x -= worldMin.x();
		a.y -= worldMin.y();
		a.z -= worldMin.z();
		vec3f aLoc(a.x, a.y, a.z);

		vec3 ball_min = aLoc - vec3(radius*atomScale, radius*atomScale, radius*atomScale);
		vec3 ball_max = aLoc + vec3(radius*atomScale, radius*atomScale, radius*atomScale);
		
		ball_min *= vpa;
		ball_max *= vpa;
	
		ball_min = max(ball_min, vec3(0.0, 0.0, 0.0));
		ball_max = min(ball_max, gridDimMinusOne);

		// add atom to cells it crosses
		for (int m = int(ball_min.z()); m <= int(ball_max.z()); m++)
			for (int l = int(ball_min.y()); l <= int(ball_max.y()); l++)
				for (int k = int(ball_min.x()); k <= int(ball_max.x()); k++)
				{
					vector<unsigned int> & mc = tempMacrocells.get_data(k, l, m);
					mc.push_back(i);
					indicesSize++;
					
					maxMC = max(maxMC, mc.size());
				}
		
		// scale atom position
		a.x *= vpa;
		a.y *= vpa;
		a.z *= vpa;
		
		if (counter > 1000)
		{
			cout << "\r\t looking at all atoms..." << "\t\t" << 
				floor( .5 + 100.0f * (float) i / (float) fAtomsCount) << "%" << flush;
			counter = 0;
		}
		
	}
	
	this->indicesCount = indicesSize;
	this->maxMCDensity = maxMC;
	
	cout << endl;
	cout << "\t max mc density: " << maxMC << endl;
	cout << "\t constructing indices..." << flush;
	

	/* ---------------------------------------------
	 * Make indices
	 * ---------------------------------------------
	 */
	
	// make memory for indices
	indicesSqrt = ceil(sqrt(indicesCount));
 	indices = new unsigned int[ indicesSqrt * indicesSqrt ];
	counter = 0;
	unsigned int curIndex = 0;
	unsigned int totalCells = gridDim.x() * gridDim.y() * gridDim.z();
	
	for (size_t m = 0; m < gridDim.z(); m++)
		for (size_t l = 0; l < gridDim.y(); l++)
			for (size_t k = 0; k < gridDim.x(); k++)
			{
				// get location in temp macrocell struct
				const vector<unsigned int> & tempMc = tempMacrocells.get_data(k, l, m);
				Macrocell & mc = macrocells.get_data(k, l, m);
				
				// update macrocell structure
				mc.ballStart = curIndex;
				mc.ballCount = tempMc.size();				
				
				// add to indices
				for (size_t i = 0; i < mc.ballCount; i++)
				{
					indices[ curIndex++ ] = tempMc[i];
				}

				counter++;
				if (counter > 1000)
				{
					cout << "\r\t constructing indices..." << "\t\t" << 
						floor( .5 + 100.0f * (float) curIndex / (float) indicesCount) << "%" << flush;
					counter = 0;
				}								
			}
	
	cout << "\nDone!" << endl;
	
	// freeup temp memory
	tempMacrocells.release();
	
	/* ----------------------------------
	 * Save file if wanted
	 * ----------------------------------
	 */
	if (saveFile.length() != 0)
	{
		cout << "Saving Macrocells to file: " << saveFile << endl;
		bool res = save(saveFile.c_str());
		if (!res) {
			cerr << "Could not built Macrocells structure to file: " << saveFile << endl;
		}
		else
		{
			#ifndef DO_MPI
			cout << "OK." << endl;
			#endif
		}
	}
}

Macrocells::~Macrocells()
{
	delete [] indices;
	delete [] allAtoms;
	
	if (hasGPUData) {
		GLuint TEX[2] = {tboIndices, tboAtoms};
		glDeleteTextures(2, TEX);
	}
}

Macrocells::Macrocells(string filename)
{
	refMode = MC_REF_MODE_DEFAULT;
	hasGPUData = false;
	gridDim.x() = gridDim.y() = gridDim.z() = 0;
	voxelsPerAngstrom = 0.0;
	maxMCDensity = 0;
	atomsCount = 0;
	indicesCount = 0;
	
	
	allAtoms = NULL;
	indices = NULL;

	if (!load(filename.c_str()))
	{
		cerr << "Could not read macrocells file: " << filename << endl;
	}
}

bool Macrocells::save( const char * filename )
{
	unsigned int dimensions[3];
	dimensions[0] = gridDim.x();
	dimensions[1] = gridDim.y();
	dimensions[2] = gridDim.z();
	
	// open file for writing
	ofstream output(filename, ios::binary);
	if (!output.is_open()) {
		return false;
	}
	
	// write header
	output.write( (char*) &atomScale, sizeof(float) );
	output.write( (char*) &voxelsPerAngstrom, sizeof(float) );
	output.write( (char*) dimensions, sizeof(unsigned int) * 3 );
	output.write( (char*) &maxMCDensity, sizeof(unsigned int) );
	output.write( (char*) &indicesCount, sizeof(unsigned int) );
	output.write( (char*) &atomsCount, sizeof(unsigned int) );
	
	// write data
	output.write( (char*) macrocells.data, sizeof(Macrocell) * gridDim.x() * gridDim.y() * gridDim.z() );
	output.write( (char*) indices, sizeof(unsigned int) * indicesCount );
	output.write( (char*) allAtoms, sizeof(gpuAtom) * atomsCount ); 
	
	output << flush;
	output.close();
	
	return true;
}

bool Macrocells::load( const char * filename )
{
	unsigned int dimensions[3];
	
	// open file for writing
	ifstream input(filename, ios::binary);
	if (!input.is_open()) {
		return false;
	}
	
	
	// read header
	input.read( (char*) &atomScale, sizeof(float) );
	input.read( (char*) &voxelsPerAngstrom, sizeof(float) );
	input.read( (char*) dimensions, sizeof(unsigned int) * 3 );
	input.read( (char*) &maxMCDensity, sizeof(unsigned int) );
	input.read( (char*) &indicesCount, sizeof(unsigned int) );
	input.read( (char*) &atomsCount, sizeof(unsigned int) );
	
	gridDim.x() = dimensions[0];
	gridDim.y() = dimensions[1];
	gridDim.z() = dimensions[2];

	cout << "\t dimensions: " << gridDim.x() << " x " << gridDim.y() << " x " << gridDim.z() << endl;
	cout << "\t atoms: " << atomsCount << endl;
	cout << "\t indices: " << indicesCount << endl;
	cout << "\t macrocells: " << (gridDim.x() * gridDim.y() * gridDim.z()) << endl;
	cout << "\t VPA: " << voxelsPerAngstrom << endl;
	cout << "\t atom scale: " << atomScale << endl;
	cout << "\t max MC density: " << maxMCDensity << endl;
	
	// calculate integer square root
	atomsSqrt = ceil(sqrt(atomsCount));
	indicesSqrt = ceil(sqrt(indicesCount));
	
	// allocate memory
	cout << "Allocating memory for macrocells..." << endl;
	allAtoms = new gpuAtom[ atomsSqrt * atomsSqrt ];
	indices = new unsigned int[ indicesSqrt * indicesSqrt ];
	macrocells.resize( gridDim.x(), gridDim.y(), gridDim.z() );
	
	cout << "Reading macrocells..." << flush;
	
	
	// read data
	input.read( (char*) macrocells.data, sizeof(Macrocell) * gridDim.x() * gridDim.y() * gridDim.z() );
	input.read( (char*) indices, sizeof(unsigned int) * indicesCount );
	input.read( (char*) allAtoms, sizeof(gpuAtom) * atomsCount ); 	
	
	cout << " OK." << endl;
	input.close();
	
	return true;
}


void Macrocells::upload_GLSL()
{
	// make a 3D float3 texture
	float * textureMem = new float[3 * gridDim.x() * gridDim.y() * gridDim.z()];
	float  * pTexMem = textureMem;
	
	for (int m = 0; m < gridDim.z(); m++)
		for (int l = 0; l < gridDim.y(); l++)
			for (int k = 0; k < gridDim.x(); k++)
			{				
				const Macrocell & mc = macrocells.get_data(k, l, m);
				pTexMem[0] = mc.ballStart;
				pTexMem[1] = mc.ballCount;
				pTexMem[2] = m;
				
				pTexMem += 3;
			}
	
	// 3D macrocells texture
	glGenTextures(1, &macrocellsTex);
	glBindTexture(GL_TEXTURE_3D, macrocellsTex);
	
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	//glTexImage3D(GL_TEXTURE_3D, 0, GL_RGB32UI, gridDim.x(), gridDim.y(), gridDim.z(), 0, GL_RGB_INTEGER GL_UNSIGNED_INT, textureMem);
	glTexImage3D(GL_TEXTURE_3D, 0, GL_RGB32F, gridDim.x(), gridDim.y(), gridDim.z(), 0, GL_RGB, GL_FLOAT, textureMem);
	glFinish();
	
	delete [] textureMem;
	
	// upload indices
	changeRefMode( refMode );
	hasGPUData = true;
}

void Macrocells::changeRefMode(MC_REF_MODE mode)
{
	if (refMode == mode && hasGPUData) {
		return;
	}
	
	refMode = mode;
	
	if (hasGPUData) {
		GLuint TEX[2] = {tboIndices, tboAtoms};
		glDeleteTextures(2, TEX);
		hasGPUData = false;
	}
	
	switch (refMode)
	{
	case MC_DIRECT:
		build_direct_ref();
		break;
		
	case MC_INDEXED:
		build_indexed_ref();
		break;
	}
	hasGPUData = true;
}

void Macrocells::build_indexed_ref()
{
	cout << " * Uploading Macrocells with indexed reference..." << endl;
	cout << " * \t indices sqrt: " << indicesSqrt << endl;
	cout << " * \t atoms sqrt: " << atomsSqrt << endl;
	
	// make buffer objects
	if (false && glewGetExtension("GL_ARB_texture_buffer_object"))
	{
		GLuint TBO[2];
		glGenBuffers(2, TBO);
		tboIndices = TBO[0];
		tboAtoms = TBO[1];
		
		// indices
		glBindBuffer(GL_TEXTURE_BUFFER, tboIndices);
		glTexBuffer(GL_TEXTURE_BUFFER, GL_R32UI, tboIndices);
		glBufferData(GL_TEXTURE_BUFFER, indicesCount * sizeof(unsigned int), indices, GL_STATIC_DRAW);
		
		// atoms
		glBindBuffer(GL_TEXTURE_BUFFER, tboAtoms);
		glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, tboAtoms);
		glBufferData(GL_TEXTURE_BUFFER, atomsCount * sizeof(float) * 4, indices, GL_STATIC_DRAW);
	}
	else
	{
		GLuint TEX[2];
		glGenTextures(2, TEX);
		tboIndices = TEX[0];
		tboAtoms = TEX[1];
				
		float * fIndices = new float[ indicesSqrt * indicesSqrt ];
		for (int i = 0; i < indicesCount; i++)
			fIndices[i] = indices[i];
		
		// indices
		glBindTexture(GL_TEXTURE_2D, tboIndices);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, indicesSqrt, indicesSqrt, 0, GL_LUMINANCE, GL_FLOAT, fIndices);
		
		delete [] fIndices;

		// atoms
		glBindTexture(GL_TEXTURE_2D, tboAtoms);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, atomsSqrt, atomsSqrt, 0, GL_RGBA, GL_FLOAT, allAtoms);
	}
	
	// wait for upload
	glFinish();
}

void Macrocells::build_direct_ref()
{

	cout << " * Uploading Macrocells with direct atom reference..." << endl;
	cout << " \t * atoms sqrt: " << indicesSqrt << endl;
	
	// make a new texture for direct atom reference
	gpuAtom * atoms = new gpuAtom[indicesSqrt * indicesSqrt];
	gpuAtom * pAtoms = atoms;
	const unsigned int * pIndices = indices;

	// loop through all indices and copy the atoms referenced to the new texture	
	for (unsigned int i = 0; i < indicesCount; i++, pIndices++, pAtoms++)
	{	
		memcpy(pAtoms, allAtoms + *pIndices, sizeof(gpuAtom));
	}
	
	
	GLuint TEX[2];
	glGenTextures(2, TEX);
	tboIndices = TEX[0];
	tboAtoms = TEX[1];
	
	// atoms instead of indices	
	glBindTexture(GL_TEXTURE_2D, tboAtoms);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, indicesSqrt, indicesSqrt, 0, GL_RGBA, GL_FLOAT, atoms);
	
	// wait for upload
	glFinish();
		
	// cleanup
	delete [] atoms;
}


