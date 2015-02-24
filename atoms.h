/* -----------------------------------------------
 * Super nanovol
 * Khairi Reda, 2013
 * Aaron Knoll, 2011-2013
 * Electronic Visualization Laboratory
 * www.evl.uic.edu/kreda
 * 
 * GPL v2 License:
 * http://www.gnu.org/licenses/gpl-2.0.html
 * atoms.h
 * Based of the original nanovol:
 
 * -----------------------------------------------
 */

#ifndef _ATOMS_H__
#define _ATOMS_H__

#include <algorithm>
#include <string>
#include <map>
#include <vector>
#include <string.h>
#include "VectorT.hxx"

static const int atom_count = 119;
static const char atom_table[119][4] = {"", "H", "He", "Li", "Be", "B", "C", "N", "O", "F", "Ne", "Na", "Mg", "Al", "Si", 
  "P", "S", "Cl", "Ar", "K", "Ca", "Sc", "Ti", "V", "Cr", "Mn", "Fe", "Co", "Ni", "Cu", "Zn", "Ga",
  "Ge", "As", "Se", "Br", "Kr", "Rb", "Sr", "Y", "Zr", "Nb", "Mo", "Tc", "Ru", "Rh", "Pd", "Ag", "Cd",
  "In", "Sn", "Sb", "Te", "I", "Xe", "Cs", "Ba", "La", "Ce", "Pr", "Nd", "Pm", "Sm", "Eu", "Gd", "Tb",
  "Dy", "Ho", "Er", "Tm", "Yb", "Lu", "Hf", "Ta", "W", "Re", "Os", "Ir", "Pt", "Au", "Hg", "Tl", "Pb",
  "Bi", "Po", "At", "Rn", "Fr", "Ra", "Ac", "Th", "Pa", "U", "Np", "Pu", "Am", "Cm", "Bk", "Cf", "Es",
  "Fm", "Md", "No", "Lr", "Rf", "Db", "Sg", "Bh", "Hs", "Mt", "Ds", "Rg", "Cn", "Uut", "Fl", "Uup", "Lv", 
  "Uus", "Uuo", };
  
using namespace std;

// A single atom
struct Atom
{
	float x;
	float y;
	float z;
	float radius;
	
	// element type (one of the symbols in atom_table)
	string atomType;
	
	Atom()
	{
		radius = 1.5;		// angstroms
	}
	
	vec3 xyz_minus_rad() const	{ return vec3(x - radius, y - radius, z - radius); }
	vec3 xyz_plus_rad() const	{ return vec3(x + radius, y + radius, z + radius); }
	vec3 xyz() const		{ return vec3(x, y, z); }
};

// A single atom packed in GPU memory
struct gpuAtom
{
	float x, y, z, index;
	gpuAtom & operator=(const Atom & anAtom);
} __attribute__((packed));

// vector of atoms
typedef vector<Atom>		Atoms;

struct AtomData
{
	float vdw_radius;
	float covalent_radius;
	
	int atomic_number;
	
	bool background;			// background atoms (used in some datasets)
	bool nonexisting;			// indicates that this is a non-valid entry (i.e. no such atom)
	
	AtomData()
	{
		nonexisting = true;
		atomic_number = -1;
	}
	
	AtomData(float _vdw, float _covalent, int _atomic_number, bool bg = false)
	{
		vdw_radius = _vdw;
		covalent_radius = _covalent;
		atomic_number = _atomic_number;
		
		nonexisting = false;
		background = bg;
	}
};

void init_atom_data();
static const char * lookUpAtomType(int atomic_number) { return atom_table[atomic_number]; }
const AtomData & lookUpAtom(string symbol);

#endif

