#ifndef __MATRIX44_HXX__
#define __MATRIX44_HXX__

#include "VectorT.hxx"
#include "Util.hxx"
#include <stdio.h>
#include <string.h>
#include <math.h>

using namespace std;

class Matrix44
{
public:
  float m[4][4];
  
  	//member functions
	inline void operator=(const Matrix44& rhs)
	{
		memcpy(this, &rhs, sizeof(Matrix44));
	}
  
  inline Matrix44()
  {
  	  memset(this, 0, sizeof(Matrix44));
  }
  
  inline Matrix44(const float * mm)
  {
  	  memcpy(this, mm, sizeof(Matrix44));
  }
  
  inline void operator+=(const Matrix44& rhs)
  {
    for(int j=0; j<4; j++)
      for(int i=0; i<4; i++)
        m[i][j] += rhs.m[i][j];
  }

  inline void operator-=(const Matrix44& rhs)
  {
    for(int j=0; j<4; j++)
      for(int i=0; i<4; i++)
        m[i][j] -= rhs.m[i][j];
  }
  
  inline void operator*=(const Matrix44& rhs)
  {
    Matrix44 tmp;
    multiply(tmp, *this, rhs);
    *this = tmp;
  }

	inline void set(const Matrix44& b)
	{
		memcpy(this, &b, sizeof(Matrix44));
	}
  
  inline void zero()
  {
    memset(this, 0, sizeof(Matrix44));  
  }
  
  inline void scrub()
  {
    for(int j=0; j<4; j++)
      for(int i=0; i<4; i++)
        if (fabs(m[i][j]) < 1e-7)
          m[i][j] = 0.f;
  }
  
  inline bool is_cartesian()
  {
    return !(m[0][1] || m[0][2] || m[1][0] || m[1][2] || m[2][0] || m[2][1] || m[3][0] || m[3][1] || m[3][2]); 
  }
  
  inline vec3 diag3()
  {
    return vec3(m[0][0], m[1][1], m[2][2]);
  }
  
  inline vec3 get_translate_vec3()
  {
    return vec3(m[0][3], m[1][3], m[2][3]);
  }
  
  inline void identity()
  {
    memset(this, 0, sizeof(Matrix44));
    m[0][0] = m[1][1] = m[2][2] = m[3][3] = 1.f;  
  }
  
  inline void Zrotation(float angle)
  {
  	float sine = sinf(angle);
  	float cosine = cosf(angle);
  	
  	m[0][0] = cosine;
  	m[0][1] = sine;
  	m[0][2] = 0.0f;
  	m[0][3] = 0.0f;
  	
  	m[1][0] = -sine;
  	m[1][1] = cosine;
  	m[1][2] = 0.0f;
  	m[1][3] = 0.0f;
  	
  	m[2][0] = 0.0f;
  	m[2][1] = 0.0f;
  	m[2][2] = 1.0f;
  	m[2][3] = 0.0f;
  	
  	m[3][0] = 0.0f;
  	m[3][1] = 0.0f;
  	m[3][2] = 0.0f;
  	m[3][3] = 1.0f;
  }

  inline void Yrotation(float angle)
  {
  	float sine = sinf(angle);
  	float cosine = cosf(angle);
  	
  	m[0][0] = cosine;
  	m[0][1] = 0.0f;
  	m[0][2] = -sine;
  	m[0][3] = 0.0f;
  	
  	m[1][0] = 0.0f;
  	m[1][1] = 1.0f;
  	m[1][2] = 0.0f;
  	m[1][3] = 0.0f;
  	
  	m[2][0] = sine;
  	m[2][1] = 0.0f;
  	m[2][2] = cosine;
  	m[2][3] = 0.0f;
  	
  	m[3][0] = 0.0f;
  	m[3][1] = 0.0f;
  	m[3][2] = 0.0f;
  	m[3][3] = 1.0f;
  }
  
  inline void Xrotation(float angle)
  {
  	float sine = sinf(angle);
  	float cosine = cosf(angle);
  	
  	m[0][0] = 1.0f;
  	m[0][1] = 0.0f;
  	m[0][2] = 0.0f;
  	m[0][3] = 0.0f;
  	
  	m[1][0] = 0.0f;
  	m[1][1] = cosine;
  	m[1][2] = sine;
  	m[1][3] = 0.0f;
  	
  	m[2][0] = 0.0f;
  	m[2][1] = -sine;
  	m[2][2] = cosine;
  	m[2][3] = 0.0f;
  	
  	m[3][0] = 0.0f;
  	m[3][1] = 0.0f;
  	m[3][2] = 0.0f;
  	m[3][3] = 1.0f;
  }
  
  inline void rotation(const vec3& axis, float angle)
  {
    struct {
      float x,y,z,w;
    } q;
    
    float ha = 0.5*angle;
    float sine = sinf(ha);
    q.w = cosf(ha);
    q.x = sine*axis[0];
    q.y = sine*axis[1];
    q.z = sine*axis[2];

    float tx  = 2.0*q.x;
    float ty  = 2.0*q.y;
    float tz  = 2.0*q.z;
    float twx = tx*q.w;
    float twy = ty*q.w;
    float twz = tz*q.w;
    float txx = tx*q.x;
    float txy = ty*q.x;
    float txz = tz*q.x;
    float tyy = ty*q.y;
    float tyz = tz*q.y;
    float tzz = tz*q.z;

    m[0][0] = 1.0-(tyy+tzz);
    m[0][1] = txy-twz;
    m[0][2] = txz+twy;
    m[1][0] = txy+twz;
    m[1][1] = 1.0-(txx+tzz);
    m[1][2] = tyz-twx;
    m[2][0] = txz-twy;
    m[2][1] = tyz+twx;
    m[2][2] = 1.0-(txx+tyy);

    m[3][0] = 0.0f;
    m[3][1] = 0.0f;
    m[3][2] = 0.0f;

    m[0][3] = 0.0f;
    m[1][3] = 0.0f;
    m[2][3] = 0.0f;

    m[3][3] = 1.0f;  
  }
  
  inline void multiplyRotation(const vec3& axis, float angle)
  {
    Matrix44 rot;
    rot.rotation(axis, angle);
    *this *= rot;
  }

	inline void translate(const vec3& trans)
	{
		m[3][0] += trans[0];
		m[3][1] += trans[1];
		m[3][2] += trans[2];
	}
  
	inline void scale(const vec3& scale)
	{
		m[0][0] *= scale[0];
		m[0][1] *= scale[1];
		m[0][2] *= scale[2];

		m[1][0] *= scale[0];
		m[1][1] *= scale[1];
		m[1][2] *= scale[2];

		m[2][0] *= scale[0];
		m[2][1] *= scale[1];
		m[2][2] *= scale[2];

		m[3][0] *= scale[0];
		m[3][1] *= scale[1];
		m[3][2] *= scale[2];
	}

  inline void scale(const vec4& scale)
	{
		m[0][0] *= scale[0];
		m[0][1] *= scale[1];
		m[0][2] *= scale[2];
		m[0][3] *= scale[3];
    
		m[1][0] *= scale[0];
		m[1][1] *= scale[1];
		m[1][2] *= scale[2];
		m[1][3] *= scale[3];
    
		m[2][0] *= scale[0];
		m[2][1] *= scale[1];
		m[2][2] *= scale[2];
		m[2][3] *= scale[3];
    
		m[3][0] *= scale[0];
		m[3][1] *= scale[1];
		m[3][2] *= scale[2];
    m[3][3] *= scale[3];
	}
  
  
	static inline void copy(Matrix44& dst, const Matrix44& src)
	{
		memcpy(&dst, &src, sizeof(Matrix44));
	}
  
  static inline void add(Matrix44& dst, const Matrix44& src1, const Matrix44& src2)
  {
    for(int j=0; j<4; j++)
      for(int i=0; i<4; i++)
        dst.m[i][j] = src1.m[i][j] + src2.m[i][j];
  }

  static inline void sub(Matrix44& dst, const Matrix44& src1, const Matrix44& src2)
  {
    for(int j=0; j<4; j++)
      for(int i=0; i<4; i++)
        dst.m[i][j] = src1.m[i][j] - src2.m[i][j];
  }
  
	static inline void multiply(Matrix44& dst, const Matrix44& src1, const Matrix44& src2)
	{
		dst.m[0][0] = src1.m[0][0]*src2.m[0][0] + src1.m[0][1]*src2.m[1][0] + src1.m[0][2]*src2.m[2][0] + src1.m[0][3]*src2.m[3][0];
		dst.m[0][1] = src1.m[0][0]*src2.m[0][1] + src1.m[0][1]*src2.m[1][1] + src1.m[0][2]*src2.m[2][1] + src1.m[0][3]*src2.m[3][1];
		dst.m[0][2] = src1.m[0][0]*src2.m[0][2] + src1.m[0][1]*src2.m[1][2] + src1.m[0][2]*src2.m[2][2] + src1.m[0][3]*src2.m[3][2];
		dst.m[0][3] = src1.m[0][0]*src2.m[0][3] + src1.m[0][1]*src2.m[1][3] + src1.m[0][2]*src2.m[2][3] + src1.m[0][3]*src2.m[3][3];

		dst.m[1][0] = src1.m[1][0]*src2.m[0][0] + src1.m[1][1]*src2.m[1][0] + src1.m[1][2]*src2.m[2][0] + src1.m[1][3]*src2.m[3][0];
		dst.m[1][1] = src1.m[1][0]*src2.m[0][1] + src1.m[1][1]*src2.m[1][1] + src1.m[1][2]*src2.m[2][1] + src1.m[1][3]*src2.m[3][1];
		dst.m[1][2] = src1.m[1][0]*src2.m[0][2] + src1.m[1][1]*src2.m[1][2] + src1.m[1][2]*src2.m[2][2] + src1.m[1][3]*src2.m[3][2];
		dst.m[1][3] = src1.m[1][0]*src2.m[0][3] + src1.m[1][1]*src2.m[1][3] + src1.m[1][2]*src2.m[2][3] + src1.m[1][3]*src2.m[3][3];

		dst.m[2][0] = src1.m[2][0]*src2.m[0][0] + src1.m[2][1]*src2.m[1][0] + src1.m[2][2]*src2.m[2][0] + src1.m[2][3]*src2.m[3][0];
		dst.m[2][1] = src1.m[2][0]*src2.m[0][1] + src1.m[2][1]*src2.m[1][1] + src1.m[2][2]*src2.m[2][1] + src1.m[2][3]*src2.m[3][1];
		dst.m[2][2] = src1.m[2][0]*src2.m[0][2] + src1.m[2][1]*src2.m[1][2] + src1.m[2][2]*src2.m[2][2] + src1.m[2][3]*src2.m[3][2];
		dst.m[2][3] = src1.m[2][0]*src2.m[0][3] + src1.m[2][1]*src2.m[1][3] + src1.m[2][2]*src2.m[2][3] + src1.m[2][3]*src2.m[3][3];

		dst.m[3][0] = src1.m[3][0]*src2.m[0][0] + src1.m[3][1]*src2.m[1][0] + src1.m[3][2]*src2.m[2][0] + src1.m[3][3]*src2.m[3][0];
		dst.m[3][1] = src1.m[3][0]*src2.m[0][1] + src1.m[3][1]*src2.m[1][1] + src1.m[3][2]*src2.m[2][1] + src1.m[3][3]*src2.m[3][1];
		dst.m[3][2] = src1.m[3][0]*src2.m[0][2] + src1.m[3][1]*src2.m[1][2] + src1.m[3][2]*src2.m[2][2] + src1.m[3][3]*src2.m[3][2];
		dst.m[3][3] = src1.m[3][0]*src2.m[0][3] + src1.m[3][1]*src2.m[1][3] + src1.m[3][2]*src2.m[2][3] + src1.m[3][3]*src2.m[3][3];
	}

	static inline void transpose(Matrix44& dst, const Matrix44& src)
	{
		dst.m[0][0] = src.m[0][0];
		dst.m[0][1] = src.m[1][0];
		dst.m[0][2] = src.m[2][0];
		dst.m[0][3] = src.m[3][0];

		dst.m[1][0] = src.m[0][1];
		dst.m[1][1] = src.m[1][1];
		dst.m[1][2] = src.m[2][1];
		dst.m[1][3] = src.m[3][1];

		dst.m[2][0] = src.m[0][2];
		dst.m[2][1] = src.m[1][2];
		dst.m[2][2] = src.m[2][2];
		dst.m[2][3] = src.m[3][2];

		dst.m[3][0] = src.m[0][3];
		dst.m[3][1] = src.m[1][3];
		dst.m[3][2] = src.m[2][3];
		dst.m[3][3] = src.m[3][3];
	}

	static void inverse(Matrix44& dst, const Matrix44& src)
	{
		//ASSERT( &src != &dst, "src and dst were the same! Make a temp variable!");

		int indxc[4], indxr[4], ipiv[4];
		int i,j,k,l,ll, lx;
		int icol = 0;
		int irow = 0;
		float temp, pivinv, dum, big;

		Matrix44::copy(dst, src);

		ipiv[0]=ipiv[1]=ipiv[2]=ipiv[3]=0;

		for(i=0; i<4; i++)
		{
			big = 0.0f;

			for (j=0; j<4; j++)
			{
				if (ipiv[j] != 1)
				{
					for (k=0; k<4; k++)
					{
						if (ipiv[k] == 0)
						{
							if (fabs(dst.m[j][k]) >= big)
							{
								big = fabs(dst.m[j][k]);
								irow = j;
								icol = k;
							}
						}
						else
						{
							if (ipiv[k] > 1)
							{
                cerr << "Matrix44 invert failed" << endl;
                exit(0);
								//FAIL("Failed to invert matrix!");
							}
						}
					}
				}
			}

			++(ipiv[icol]);
			if (irow != icol)
			{
				for (l=0; l<4; l++)
				{
					SWAP(dst.m[irow][l], dst.m[icol][l], temp);
				}
			}

			indxr[i]=irow;
			indxc[i]=icol;
			if (dst.m[icol][icol] == 0.0f)
			{
        cerr << "Matrix44 invert failed" << endl;
        exit(0);
			}

			pivinv = 1.0f / dst.m[icol][icol];
			dst.m[icol][icol] = 1.0f;
			for (l=0; l<4; l++)
			{
				dst.m[icol][l] *= pivinv;
			}
			for (ll=0; ll<4; ll++)
			{
				if (ll != icol)
				{
					dum = dst.m[ll][icol];
					dst.m[ll][icol] = 0.0f;
					for (l=0; l<4; l++)
					{
						dst.m[ll][l] -= dst.m[icol][l]*dum;
					}
				}
			}
		}

		for (lx=4; lx>0; lx--)
		{
			if (indxr[lx-1] != indxc[lx-1])
			{
				for (k=0; k<4; k++)
				{
					SWAP(dst.m[k][indxr[lx-1]], dst.m[k][indxc[lx-1]], temp);
				}
			}
		}
		
		dst.scrub();
	}
  
  static inline void inverseTranspose(Matrix44& dst, Matrix44& src)
  {
    Matrix44 tmp;
    Matrix44::inverse(tmp, src);
    Matrix44::transpose(dst, tmp);
  }

	static inline void inverseAffine(Matrix44& dst, const Matrix44& src)
	{
		//ASSERT( &src != &dst, "src and dst were the same! Make a temp variable!");

		float t1, t2, t3;
    
    dst.identity();

		dst.m[0][0] = src.m[0][0];
		dst.m[1][1] = src.m[1][1];
		dst.m[2][2] = src.m[2][2];

		dst.m[0][1] = src.m[1][0];
		dst.m[1][0] = src.m[0][1];

		dst.m[0][2] = src.m[2][0];
		dst.m[2][0] = src.m[0][2];

		dst.m[1][2] = src.m[2][1];
		dst.m[2][1] = src.m[1][2];

		t1 = dst.m[0][0] * (src.m[3][0]) + dst.m[0][1] * (src.m[3][1]) + dst.m[0][2] * (src.m[3][2]);
		t2 = dst.m[1][0] * (src.m[3][0]) + dst.m[1][1] * (src.m[3][1]) + dst.m[1][2] * (src.m[3][2]);
		t3 = dst.m[2][0] * (src.m[3][0]) + dst.m[2][1] * (src.m[3][1]) + dst.m[2][2] * (src.m[3][2]);

		dst.m[3][0] = -t1;
		dst.m[3][1] = -t2;
		dst.m[3][2] = -t3;
	}

  static inline void transform(vec3& dst, const Matrix44& a, const vec3& b)
  {
    for(int i=0; i<3; i++)
    {      
      dst[i] = 0.f;
      for(int j=0; j<3; j++)
        dst[i] += a.m[i][j] * b[j];
    }
  }  
  
  static inline void transform(vec4& dst, const Matrix44& a, const vec4& b)
  {

    for(int i=0; i<4; i++)
    {      
      dst[i] = 0.f;
      for(int j=0; j<4; j++)
        dst[i] += a.m[i][j] * b[j];
    }
  }
  
  static inline void transform3(vec4& dst, const Matrix44& a, const vec4& b)
  {
    vec4 btmp = b;
    btmp[3] = 1.f;
    
    for(int i=0; i<4; i++)
    {      
      dst[i] = 0.f;
      for(int j=0; j<4; j++)
        dst[i] += a.m[i][j] * btmp[j];
    }
  }  
  
  
};

inline vec4 operator*(const Matrix44& l, const vec4& r)
{
  vec4 dst;
  Matrix44::transform(dst, l, r);
  return dst;
}

inline vec4 operator*(const Matrix44& l, const vec3& r)
{
  vec4 rr = vec4(r[0], r[1], r[2], 0.f);
  vec4 dst;
  Matrix44::transform(dst, l, rr);
  return dst;
}


inline ostream & operator<<(ostream& o, const Matrix44& m)
{
	//o.unsetf(ios::floatfield);
	o.setf(ios::fixed,ios::floatfield);   // floatfield set to fixed
	o.precision(4);
  o << endl << " [ " << m.m[0][0] << "\t" << m.m[0][1] << "\t" << m.m[0][2] << "\t" << m.m[0][3] << endl 
            << "   " << m.m[1][0] << "\t" << m.m[1][1] << "\t" << m.m[1][2] << "\t" << m.m[1][3] << endl 
            << "   " << m.m[2][0] << "\t" << m.m[2][1] << "\t" << m.m[2][2] << "\t" << m.m[2][3] << endl 
            << "   " << m.m[3][0] << "\t" << m.m[3][1] << "\t" << m.m[3][2] << "\t" << m.m[3][3] << " ]" << endl; 
  return o;
}

inline ostream & operator>>(ostream& o, const Matrix44& m)
{
	//o.unsetf(ios::floatfield);
	o.setf(ios::fixed,ios::floatfield);   // floatfield set to fixed
	o.precision(4);
  o << endl << " { " << m.m[0][0] << "\t" << m.m[1][0] << "\t" << m.m[2][0] << "\t" << m.m[3][0] << endl 
            << "   " << m.m[0][1] << "\t" << m.m[1][1] << "\t" << m.m[2][1] << "\t" << m.m[3][1] << endl 
            << "   " << m.m[0][2] << "\t" << m.m[1][2] << "\t" << m.m[2][2] << "\t" << m.m[3][2] << endl 
            << "   " << m.m[0][3] << "\t" << m.m[1][3] << "\t" << m.m[2][3] << "\t" << m.m[3][3] << "  }" << endl; 
  return o;
}

#endif
