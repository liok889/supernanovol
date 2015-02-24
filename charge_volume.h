/* -----------------------------------------------
 * Super nanovol
 * Khairi Reda, 2013
 * Electronic Visualization Laboratory
 * www.evl.uic.edu/kreda
 * 
 * GPL v2 License:
 * http://www.gnu.org/licenses/gpl-2.0.html
 * charge_volume.h
 * Based of the original nanovol:
 * https://svn.ci.uchicago.edu/svn/nanovol
 * -----------------------------------------------
 */
 
#ifndef _CHARGE_VOLUME_H__
#define _CHARGE_VOLUME_H__

#include <string>
#include "VectorT.hxx"
#include "data.h"
#include "graphics/graphics.h"

using namespace std;

enum VOLUME_TYPE
{
	VOL_UCHAR,
	VOL_FLOAT,
};
static const VOLUME_TYPE defaultVolType = VOL_FLOAT;

class ChargeDensityVolume
{
public:	
	// construct approximate charge density volume
	ChargeDensityVolume(
		const Atoms & vAtoms,			// list of all atoms 
		vec3 worldMin,				// min world coordinate
		float mvpa, 				// Macrocells VPA
		float cvpa,				// Volume VPA
		vec3i mcDim,				// dimensions of MC grid
		string saveFile				// file to SAVE
	);
	
	// load charge density volume from file
	ChargeDensityVolume(string filename, float mvpa);
	
	// destructor
	~ChargeDensityVolume();
	
	// upload volume to GLSL
	void upload_GLSL();
	void free_GLSL();
	
	// public accessors
	bool hasGLSLData() const { return hasGPUData; }
	GLuint getVolumeTex() const { return volumeTex; }
	
	// dimensions
	const vec3i & getGridDim() const { return gridDim; }
	const vec3 & getVolumeWSExtent() const { return volumeWSExtent; }
	const vec3 & getVolumeGSExtent() const { return volumeGSExtent; }
	
	// VPA
	float getVPA() const { return voxelsPerAngstrom; }
	
	// max / min density
	float getMaxValue() const { return maxDensity; }
	float getMinValue() const { return minDensity; }
	float getDensityScale() const { return (volType == VOL_UCHAR) ? maxDensity - minDensity : 1.0f; }
	float getDensityOffset() const { return (volType == VOL_UCHAR) ? minDensity : 0.0f; }
	
	VOLUME_TYPE getVolumeType() const { return volType; }
	void setVolumeType(VOLUME_TYPE _volType) { volType = _volType; }
private:
	
	// private functions
	bool save(const char *);
	bool load(const char *);
	
	// size of volume
	vec3i				gridDim;
	
	// volume extents
	vec3				volumeWSExtent;
	vec3				volumeGSExtent;
	
	// information
	float				voxelsPerAngstrom;
	float				maxDensity;
	float				minDensity;
		
	// the actual volume
	Grid3<float>			volume;
	
	// volume type to upload to GLSL
	VOLUME_TYPE			volType;
	
	// GLSL data
	bool				hasGPUData;
	GLuint				volumeTex;
};

#endif

