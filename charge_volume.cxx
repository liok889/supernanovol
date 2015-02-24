/* -----------------------------------------------
 * Super nanovol
 * Khairi Reda, 2013
 * Electronic Visualization Laboratory
 * www.evl.uic.edu/kreda
 * 
 * GPL v2 License:
 * http://www.gnu.org/licenses/gpl-2.0.html
 * charge_volume.cxx
 *
 * Based of the original nanovol:
 * https://svn.ci.uchicago.edu/svn/nanovol
 * -----------------------------------------------
 */
 
#include <cfloat>
#include <fstream>
#include <iostream>
#include <stdlib.h>
#include <string.h>
#ifdef __linux__
#include <omp.h>
#endif
#include "rbf.h"
#include "atoms.h"
#include "charge_volume.h"

using namespace std;

ChargeDensityVolume::ChargeDensityVolume(
	const Atoms & vAtoms,
	vec3 worldMin,
	float mvpa, float cvpa, 
	vec3i mcDim, string saveFile
)
{
	// type of volume to upload to GPU
	volType = defaultVolType;
	
	// initial setting
	hasGPUData = false;
	voxelsPerAngstrom = cvpa;

	// calculate grid dimensions
	const float inv_cvpa = 1.0f / cvpa;
	
	// calculate extents of MC grid in world space
	vec3 worldDim(
		float(mcDim.x()) / mvpa,
		float(mcDim.y()) / mvpa,
		float(mcDim.z()) / mvpa
	);
	
	
	// calculate volume extends
	gridDim.x() = (int) ceil( worldDim.x() * cvpa );
	gridDim.y() = (int) ceil( worldDim.y() * cvpa );
	gridDim.z() = (int) ceil( worldDim.z() * cvpa );
	
	// figure volume extends in world and MC grid coordinates
	this->volumeWSExtent = vec3(
		
		float(gridDim.x()) * inv_cvpa,
		float(gridDim.y()) * inv_cvpa,
		float(gridDim.z()) * inv_cvpa
	);
	this->volumeGSExtent = mvpa * volumeWSExtent;
	
	
	const size_t cellCount =  gridDim.x() * gridDim.y() * gridDim.z();

	cout << "Building RBF approx. e density volume..." << endl;
	cout << "\t volume vpa: " << cvpa << endl;
	cout << "\t volume dimensions: " << gridDim.x() << " x " << gridDim.y() << " x " << gridDim.z() << endl;
	
	// allocate memory
	volume.resize( gridDim.x(), gridDim.y(), gridDim.z() );
	memset(volume.data, 0, cellCount * sizeof(float));
	
	if (volume.data)
	{
		cout << "\t mem allocated: " << ((cellCount * sizeof(float) / 1024) / 1024) << " MB" << endl;
	}
	else
	{
		cerr << "\t Could not allocate memory!" << endl;
		exit(1);
	}

	// min / max
	const vec3 gridMax(gridDim.x()-1, gridDim.y()-1, gridDim.z()-1);
	const vec3 gridMin(0, 0, 0);
	
	
	// OpenMP
#ifdef __linux__
	omp_set_dynamic(0);
	int ncores = omp_get_max_threads();  
	omp_set_num_threads( ncores );
	cout << "\t We will use " << ncores << " OpenMP cores." << endl;
#endif

	cerr << "\t Looking at all atoms... " << flush;

#ifdef __linux__
#pragma omp parallel for
#endif
	for(unsigned int i = 0; i < vAtoms.size(); i++)
	{
		const Atom & atom = vAtoms[i];
		const AtomData & atomData = lookUpAtom( atom.atomType );
		
		// subtract worldMin
		vec3 wsAtom(atom.x, atom.y, atom.z);
		wsAtom -= worldMin;
		
		// create RBF function (in angstrom (world) space)
		const RadialBasisFunction rbf(atomData.covalent_radius, atomData.vdw_radius, atomData.atomic_number );	
		vec3 kernelRadius = vec3(rbf.getClampRadius(), rbf.getClampRadius(), rbf.getClampRadius());
		
		// calculate range
		vec3 gs_emin = wsAtom - kernelRadius;		gs_emin *= cvpa; 
		vec3 gs_emax = wsAtom + kernelRadius;		gs_emax *= cvpa; 
		
		// min / max
		gs_emin = max(gs_emin, gridMin);
		gs_emax = min(gs_emax, gridMax);

		// discretize
		vec3i igs_emin(gs_emin.x(), gs_emin.y(), gs_emin.z());
		vec3i igs_emax(gs_emax.x(), gs_emax.y(), gs_emax.z());
		
		// fill
		vec3 v;
		for(unsigned int  k = igs_emin.z(); k < igs_emax.z(); k++)
		{
			v.z() = float(k) +.5f;
			for(unsigned int  j = igs_emin.y(); j < igs_emax.y(); j++)
			{
				v.y() = float(j) + .5f;
				for(unsigned int  i = igs_emin.x(); i < igs_emax.x(); i++)
				{
					v.x() = float(i) + .5f;
					
					// translate back to world space
					vec3 wsv = v * inv_cvpa;
					vec3 diff = wsv - wsAtom;
					
					const float val = rbf.evaluate(dot(diff, diff));
					float & voxel = volume.get_data(i, j, k);
					
					#ifdef __linux__
					#pragma omp atomic
					#endif
					voxel += val;	
				}
			}
		}
	}
	cout << "Done" << endl;
	
	// determine max density
	cout << "\t Figuring out min/max density: " << flush;
	maxDensity = FLT_MIN;
	minDensity = FLT_MAX;
	for (unsigned int  k = 0; k < gridDim.z(); k++)
		for (unsigned int  j = 0; j < gridDim.y(); j++)
			for (unsigned int  i = 0; i < gridDim.x(); i++)
				{
					const float & voxel = volume.get_data(i, j, k);
					if (voxel > maxDensity)
					{
						maxDensity = voxel;
					}
					else if (voxel < minDensity)
					{
						minDensity = voxel;
					}
				}
	cout << minDensity << " / " << maxDensity << endl;
	
	if (saveFile.length() > 0)
	{
		cout << "\t Saving file to: " << saveFile << endl;
		bool res = save(saveFile.c_str());
		if (!res) {
			cerr << "Could not save charge density volume to file: " << saveFile << endl;
		}
		else
		{
			cout << "OK." << endl;
		}
	}
}

ChargeDensityVolume::ChargeDensityVolume(string filename, float mvpa)
{
	volType = defaultVolType;
	hasGPUData = false;
	gridDim.x() = gridDim.y() = gridDim.z() = 0;
	voxelsPerAngstrom = 0.0;
	maxDensity = 0;

	if (!load(filename.c_str()))
	{
		cerr << "Could not read charge density volume from: " << filename << endl;
	}
	else
	{
		// figure volume extends in MC grid coordinates
		this->volumeWSExtent = vec3(
		
			float(gridDim.x()) / voxelsPerAngstrom,
			float(gridDim.y()) / voxelsPerAngstrom,
			float(gridDim.z()) / voxelsPerAngstrom
		);
		this->volumeGSExtent = mvpa * volumeWSExtent;	
	}
}

ChargeDensityVolume::~ChargeDensityVolume()
{
	if (hasGPUData) {
		glDeleteTextures(1, &volumeTex);
		hasGPUData = false;
	}
}

bool ChargeDensityVolume::save( const char * filename )
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
	output.write( (char*) &voxelsPerAngstrom, sizeof(float) );
	output.write( (char*) dimensions, sizeof(unsigned int) * 3 );
	output.write( (char*) &maxDensity, sizeof(float) );
	output.write( (char*) &minDensity, sizeof(float) );
	
	// write data
	output.write( (char*) volume.data, sizeof(float) * gridDim.x() * gridDim.y() * gridDim.z() );
	
	output << flush;
	output.close();
	
	return true;
}


bool ChargeDensityVolume::load( const char * filename )
{
	unsigned int dimensions[3];
	
	// open file for writing
	ifstream input(filename, ios::binary);
	if (!input.is_open()) {
		return false;
	}
	
	// read header
	input.read( (char*) &voxelsPerAngstrom, sizeof(float) );
	input.read( (char*) dimensions, sizeof(unsigned int) * 3 );
	input.read( (char*) &maxDensity, sizeof(float));
	input.read( (char*) &minDensity, sizeof(float));
	
	gridDim.x() = dimensions[0];
	gridDim.y() = dimensions[1];
	gridDim.z() = dimensions[2];
	size_t cellCount = gridDim.x() * gridDim.y() * gridDim.z();
	
	cout << "\t dimensions: " << gridDim.x() << " x " << gridDim.y() << " x " << gridDim.z() << endl;
	cout << "\t volume size: " << ((cellCount * sizeof(float) / 1024) / 1024) << " MB" << endl;
	cout << "\t VPA: " << voxelsPerAngstrom << endl;
	cout << "\t max density: " << maxDensity << endl;
	
	// allocate memory
	cout << "Allocating memory volume..." << endl;
	volume.resize( gridDim.x(), gridDim.y(), gridDim.z() );
	if (!volume.data) 
	{
		cerr << "Could not allocate memory!" << endl;
		exit(1);
	}
	
	cout << "Reading volume..." << flush;
	
	
	// read data
	input.read( (char*) volume.data, sizeof(float) * cellCount );
	
	cout << " OK." << endl;
	input.close();
	
	return true;
}

void ChargeDensityVolume::free_GLSL()
{
	glDeleteTextures(1, &volumeTex);
	hasGPUData = false;
}


void ChargeDensityVolume::upload_GLSL()
{
	if (hasGPUData) {
		return;
	}
	// 3D macrocells texture
	glGenTextures(1, &volumeTex);
	glBindTexture(GL_TEXTURE_3D, volumeTex);
	
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	
	if (volType == VOL_FLOAT)
	{
		glTexImage3D(GL_TEXTURE_3D, 0, GL_R32F, gridDim.x(), gridDim.y(), gridDim.z(), 0, GL_LUMINANCE, GL_FLOAT, volume.data);
		glFinish();
	}
	else if (volType == VOL_UCHAR)
	{
		// make a uchar grid
		Grid3<unsigned char> ucData(gridDim.x(), gridDim.y(), gridDim.z());
			
		// density scaling
		const float densityScale = 255.0f / (maxDensity - minDensity);
				
		// quantize float to uchar
		for (unsigned int  k = 0; k < gridDim.z(); k++)
			for (unsigned int  j = 0; j < gridDim.y(); j++)
				for (unsigned int  i = 0; i < gridDim.x(); i++)
				{
					const float voxel = volume.get_data(i, j, k);
					unsigned char & ucVoxel = ucData.get_data(i, j, k);
					
					float quant = floor(.5f + (voxel - minDensity) * densityScale);
					if (quant > 255.0f)
						quant = 255.0f;
					else if (quant < 0.0f)
						quant = 0.0f;
					
					ucVoxel = (unsigned char) quant;
				}
		glTexImage3D(GL_TEXTURE_3D, 0, GL_R8, gridDim.x(), gridDim.y(), gridDim.z(), 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, ucData.data);
		glFinish();
	}
		
	hasGPUData = true;
}


