/* -----------------------------------------------
 * Super nanovol
 * Khairi Reda, 2013
 * Aaron Knoll, 2011-2013
 * Electronic Visualization Laboratory
 * www.evl.uic.edu/kreda
 * 
 * GPL v2 License:
 * http://www.gnu.org/licenses/gpl-2.0.html
 * rbf.h
 * Based of the original nanovol:
 * https://svn.ci.uchicago.edu/svn/nanovol
 * -----------------------------------------------
 */
 
#ifndef RBF_DENSITY_H__
#define RBF_DENSITY_H__

#include <math.h>
#include "VectorT.hxx"

class RadialBasisFunction
{
public:
	// units are in world space (angstroms)
	RadialBasisFunction(
		float _covalent_radius,
		float _vdw_radius,
		int atomic_number
	):
	clamp_radius(_vdw_radius), sigma(_covalent_radius), inv_sigma_sq(0.0f), Z(float(atomic_number))
	{
		inv_sigma_sq = 1.0f / (sigma * sigma);
	}
	
	float getClampRadius() const { return clamp_radius; }	
	float evaluate(float distance_sq) const
	{
		float r2 = distance_sq * inv_sigma_sq;
		return (Z * expf(-r2));	
	}
	
private:
	
	float clamp_radius;		// usually Van der Waals radius
	float sigma;			// usually covalent radius
	float inv_sigma_sq;		// 1/sigma
	float Z;			// atomic number
};

#endif

