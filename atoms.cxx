/* -----------------------------------------------
 * Super nanovol
 * Khairi Reda, 2013
 * Aaron Knoll, 2011-2013
 * Electronic Visualization Laboratory
 * www.evl.uic.edu/kreda
 * 
 * GPL v2 License:
 * http://www.gnu.org/licenses/gpl-2.0.html
 * atoms.cxx
 * Based of the original nanovol:
 
 * -----------------------------------------------
 */

#include <iostream>
#include <map>
#include "atoms.h"

// map containing atom data for a selection of atoms
static map<string, AtomData> knownElements;
static AtomData unknownElement;

int get_atomic_number(const char * str)
{
	for (int i = 0; i < 119; i++)
	{
		if (0 == strcasecmp(str, atom_table[i])) {
			return i;
		}
	}
	return -1;
}

void init_atom_data()
{
	static bool inited = false;
	if (inited) {
		return;
	}
	
	knownElements["h"]	=		AtomData(1.20, 0.37, get_atomic_number("h"	));
	knownElements["o"]	=		AtomData(1.52, 0.73, get_atomic_number("o"	));
	knownElements["o_bg"]	=		AtomData(1.52, 0.73, get_atomic_number("o"	), true);	// background O
	knownElements["al"]	=		AtomData(2.10, 1.18, get_atomic_number("al"	));
	knownElements["al_bg"]	=		AtomData(2.10, 1.18, get_atomic_number("al"	), true);	// background Al
	knownElements["c"]	= 		AtomData(1.70, 0.77, get_atomic_number("c"	));
	knownElements["si"]	=		AtomData(2.10, 1.11, get_atomic_number("si"	));
	
	inited = true;
}

const AtomData & lookUpAtom(string symbol)
{
	std::transform(symbol.begin(), symbol.end(), symbol.begin(), ::tolower);
	map<string, AtomData>::const_iterator it = knownElements.find(symbol);
	
	if (it == knownElements.end())
	{
		return unknownElement;
	}
	else
	{
		return it->second;
	}
}


