/* -----------------------------------------------
 * Super nanovol
 * Khairi Reda, 2013
 * Electronic Visualization Laboratory
 * www.evl.uic.edu/kreda
 * 
 * GPL v2 License:
 * http://www.gnu.org/licenses/gpl-2.0.html
 * grid.h
 * -----------------------------------------------
 */
 
#ifndef _GRID_H____
#define _GRID_H____

#include <assert.h>

template<typename T>
class Grid3
{
public:
	Grid3(): d1(0), d2(0), d3(0), data(0) {}
	Grid3(int _d1, int _d2, int _d3): d1(0), d2(0), d3(0), data(0) 
	{
		resize(_d1, _d2, _d3);
	}
	
	~Grid3()
	{
		release();
	}
	
	void resize(int i, int j, int k)
	{
		delete [] data;
		d1 = i;
		d2 = j;
		d3 = k;
		allocate();
	}
	
	
	void allocate()
	{
		assert(d1);
		assert(d2);
		assert(d3);
	
		data = new T[ d1*d2*d3 ];
	}
	
	void release()
	{
		delete [] data;
		data = NULL;
		d1 = d2 = d3 = 0;
	}
	
	T & get_data(int i, int j, int k) const
	{
		return data[i + d1*(j + d2 * k)];
	}
	
	int d1, d2, d3;
	T * data;
};

#endif

