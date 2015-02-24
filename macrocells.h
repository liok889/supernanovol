/* -----------------------------------------------
 * Super nanovol
 * Khairi Reda, 2013
 * Electronic Visualization Laboratory
 * www.evl.uic.edu/kreda
 * 
 * GPL v2 License:
 * http://www.gnu.org/licenses/gpl-2.0.html
 * macrocells.h
 *
 * -----------------------------------------------
 */
 
#ifndef _MACROCELLS_H___
#define _MACROCELLS_H___

#include <string>
#include <vector>
#include "data.h"

enum MC_REF_MODE
{
	MC_DIRECT,		// atoms are referenced directly
	MC_INDEXED,		// atoms are referenced by global index
};
static MC_REF_MODE MC_REF_MODE_DEFAULT = MC_DIRECT;

struct Macrocell
{
	unsigned int ballStart;
	unsigned int ballCount;
}  __attribute__((packed));


class Macrocells
{
public:
	// constructs macrocells from ground up
	Macrocells(
		const Atoms & vAtoms,
		float vpa, float atomScale,
		vec3 worldMin, vec3 worldMax, string saveFile
	);

	// loads macrocells from file
	Macrocells(string filename);
	
	// desturctor
	~Macrocells();
	
	// pload to GPU
	void upload_GLSL();

	// public accessors for texture IDs
	GLuint getMacrocellsTex() const { return macrocellsTex; }
	GLuint getIndicesTex() const { return tboIndices; }
	GLuint getAtomsTex() const { return tboAtoms; }

	// change macrocell atom referel way works
	void changeRefMode(MC_REF_MODE _mode);
	MC_REF_MODE getRefMode() const { return refMode; }
	
	// public accessors
	unsigned int getMaxMCDensity() const { return maxMCDensity; }
	int getIndicesSqrt() const { return indicesSqrt; }
	int getAtomsSqrt() const { return atomsSqrt; }
	bool hasGLSLData() const { return hasGPUData; }
	float getVPA() const { return voxelsPerAngstrom; }
	float getAtomScale() const { return atomScale; }
	const vec3i & getGridDim() const { return gridDim; }
	
private:	
	
	// private functions
	bool save(const char * );
	bool load(const char * );
	
	// reference building / upload to GLSL
	void build_indexed_ref();
	void build_direct_ref();	// builds direct reference array and upload it to GPU
	
	// referal mode
	MC_REF_MODE			refMode;
	
	// size of macrocells structure
	vec3i				gridDim;
	
	// information
	float				voxelsPerAngstrom;
	float				atomScale;
	unsigned int			maxMCDensity;
	
	unsigned int			atomsCount;
	int				atomsSqrt;
	
	unsigned int			indicesCount;
	int				indicesSqrt;
	
	// temp macrocells data
	Grid3<vector<unsigned int> >	tempMacrocells;
	
	// final macrocells structure
	gpuAtom *			allAtoms;
	unsigned int *			indices;
	Grid3<Macrocell>		macrocells;
	
	// GLSL data
	bool hasGPUData, texInited;
	GLuint macrocellsTex, tboIndices, tboAtoms;
};

#endif


