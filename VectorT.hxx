
#ifndef _VECTORT_HXX_
#define _VECTORT_HXX_
#include <math.h>
#include <iostream>
#include <fstream>

using namespace std; 

template<typename T, int Dim>
class VectorT {
public:
  typedef T ComponentType;

  VectorT() {
  }
  
  VectorT(T x) {
    for(int i=0;i<Dim;i++)
      data[i] = x;
  }

  VectorT(T x, T y) {
    typedef char unnamed[ Dim == 2 ? 1 : 0 ];
    data[0] = x; data[1] = y;
  }
  VectorT(T x, T y, T z) {
    typedef char unnamed[ Dim == 3 ? 1 : 0 ];
    data[0] = x; data[1] = y; data[2] = z;
  }
  VectorT(T x, T y, T z, T w) {
    typedef char unnamed[ Dim == 4 ? 1 : 0 ];
    data[0] = x; data[1] = y; data[2] = z; data[3] = w;
  }

	VectorT(T x, const VectorT<T,3>& yzw) {
		typedef char unnamed[ Dim == 4 ? 1 : 0 ];
		data[0] = x; data[1] = yzw[0]; data[2] = yzw[1]; data[3] = yzw[2];
	}


  // Copy from another vector of the same size.
  template< typename S >
  VectorT(const VectorT<S, Dim>& copy) {
    for(int i=0;i<Dim;i++)
      data[i] = static_cast<T>(copy[i]);
  }

  // Copy from same type and size.
  VectorT(const VectorT<T, Dim>& copy) {
    for(int i=0;i<Dim;i++)
      data[i] = copy[i];
  }

  VectorT(const T* data_in) {
    for(int i=0;i<Dim;i++)
      data[i] = data_in[i];
  }
  
#ifndef SWIG
  VectorT<T, Dim>& operator=(const VectorT<T, Dim>& copy) {
    for(int i=0;i<Dim;i++)
      data[i] = (T)copy[i];
    return *this;
  }
#endif

  ~VectorT() {
  }

  const T & x() const {
    return data[0];
  }
  const T & y() const {
    typedef char unnamed[ Dim >=2 ? 1 : 0 ];
    return data[1];
  }
  const T & z() const {
    typedef char unnamed[ Dim >= 3 ? 1 : 0 ];
    return data[2];
  }
  const T & w() const {
    typedef char unnamed[ Dim >= 4 ? 1 : 0 ];
    return data[3];
  }
  
  T & x() {
    return data[0];
  }
  T & y() {
    typedef char unnamed[ Dim >=2 ? 1 : 0 ];
    return data[1];
  }
  T & z() {
    typedef char unnamed[ Dim >= 3 ? 1 : 0 ];
    return data[2];
  }
  T & w() {
    typedef char unnamed[ Dim >= 4 ? 1 : 0 ];
    return data[3];
  }
  
#ifndef SWIG
  const T& operator[](int i) const {
    return data[i];
  }
  T& operator[] ( int i ) {
    return data[i];
  }

#else
  %extend {
    T& __getitem__( int i ) {
      return self->operator[](i);
    }
  }
#endif
  // One might be tempted to add an "operator &" function, but
  // that could lead to problems when you want an address to the
  // object rather than a pointer to data.
  const T* getDataPtr() const { return data; }

  VectorT<T, Dim> operator+(const VectorT<T, Dim>& v) const {
    VectorT<T, Dim> result;
    for(int i=0;i<Dim;i++)
      result.data[i] = data[i] + v.data[i];
    return result;
  }
  VectorT<T, Dim>& operator+=(const VectorT<T, Dim>& v) {
    for(int i=0;i<Dim;i++)
      data[i] += v.data[i];
    return *this;
  }
  VectorT<T, Dim> operator-(const VectorT<T, Dim>& v) const {
    VectorT<T, Dim> result;
    for(int i=0;i<Dim;i++)
      result.data[i] = data[i] - v.data[i];
    return result;
  }
  VectorT<T, Dim>& operator-=(const VectorT<T, Dim>& v) {
    for(int i=0;i<Dim;i++)
      data[i] -= v.data[i];
    return *this;
  }
  VectorT<T, Dim> operator-() const {
    VectorT<T, Dim> result;
    for(int i=0;i<Dim;i++)
      result.data[i] = -data[i];
    return result;
  }

  VectorT<T, Dim> operator*(const VectorT<T, Dim>& v) const {
    VectorT<T, Dim> result;
    for(int i=0;i<Dim;i++)
      result.data[i] = data[i] * v.data[i];
    return result;
  }
  VectorT<T, Dim> operator*(T s) const {
    VectorT<T, Dim> result;
    for(int i=0;i<Dim;i++)
      result.data[i] = data[i] * s;
    return result;
  }
  VectorT<T, Dim>& operator*=(const VectorT<T, Dim>& v) {
    for(int i=0;i<Dim;i++)
      data[i] *= v.data[i];
    return *this;
  }
  VectorT<T, Dim>& operator*=(T s) {
    for(int i=0;i<Dim;i++)
      data[i] *= s;
    return *this;
  }
  VectorT<T, Dim> operator/(T s) const {
    T inv_s = 1/s;
    VectorT<T, Dim> result;
    for(int i=0;i<Dim;i++)
      result.data[i] = data[i] * inv_s;
    return result;
  }
  VectorT<T, Dim>& operator/=(T s) {
    T inv_s = 1/s;
    for(int i=0;i<Dim;i++)
      data[i] *= inv_s;
    return *this;
  }
  VectorT<T, Dim> operator/(const VectorT<T, Dim>& v) const {
    VectorT<T, Dim> result;
    for(int i=0;i<Dim;i++)
      result.data[i] = data[i] / v.data[i];
    return result;
  }

  bool operator==(const VectorT<T, Dim>& right) const {
    for (int i = 0; i < Dim; ++i)
      if(data[i] != right.data[i])
        return false;
    return true;
  }

  T length() const {
    T sum = 0;
    for(int i=0;i<Dim;i++)
      sum += data[i]*data[i];
    return sqrtf(sum);
  }
  T length2() const {
    T sum = 0;
    for(int i=0;i<2;i++)
      sum += data[i]*data[i];
    return sqrtf(sum);
  }
  T length3() const {
    T sum = 0;
    for(int i=0;i<3;i++)
      sum += data[i]*data[i];
    return sqrtf(sum);
  }

  // GCC and perhaps other compilers have a hard time optimizing
  // with additional levels of function calls (i.e. *this *= scale).
  // Because of this, we try to minimize the number of function
  // calls we make and do explicit inlining.
  T normalize() {
    T sum = 0;
    for(int i=0;i<Dim;i++)
      sum += data[i]*data[i];
    T l = sqrtf(sum);
    T scale = 1/l;
    for(int i=0;i<Dim;i++)
      data[i] *= scale;
    return l;
  }
  VectorT<T, Dim> normal() const {
    T sum = 0;
    for(int i=0;i<Dim;i++)
      sum += data[i]*data[i];
    T l = sqrtf(sum);
    T scale = 1/l;
    VectorT<T, Dim> result;
    for(int i=0;i<Dim;i++)
      result.data[i] = data[i] * scale;
    return result;
  }
  T minComponent() const {
    T min = data[0];
    for(int i=1;i<Dim;i++)
      if(data[i] < min)
        min = data[i];
    return min;
  }
  T maxComponent() const {
    T max = data[0];
    for(int i=1;i<Dim;i++)
      if(data[i] > max)
        max = data[i];
    return max;
  }
  int indexOfMinComponent() const {
    int idx = 0;
    for(int i=1;i<Dim;i++)
      if(data[i] < data[idx])
        idx = i;
    return idx;
  }
  int indexOfMaxComponent() const {
    int idx = 0;
    for(int i=1;i<Dim;i++)
      if(data[i] > data[idx])
        idx = i;
    return idx;
  }

  VectorT<T, Dim> inverse() const {
    VectorT<T, Dim> result;
    for(int i=0;i<Dim;i++)
      result.data[i] = 1/data[i];
    return result;
  }
  VectorT<T, Dim> absoluteValue() const {
    VectorT<T, Dim> result;
    for(int i=0;i<Dim;i++)
      result.data[i] = fabs(data[i]);
    return result;
  }

  static VectorT<T, Dim> zero() {
    VectorT<T, Dim> result;
    for(int i=0;i<Dim;i++)
      result.data[i] = 0;
    return result;
  }
  static VectorT<T, Dim> one() {
    VectorT<T, Dim> result;
    for(int i=0;i<Dim;i++)
      result.data[i] = 1;
    return result;
  }

  T data[Dim];  
};


// Two multiplication by a scalar operators.
template<typename T, int Dim, typename S >
inline VectorT<T, Dim> operator*(S s, const VectorT<T, Dim>& v)
{
  T scalar = static_cast<T>(s);
  VectorT<T, Dim> result;
  for(int i=0;i<Dim;i++)
    result[i] = scalar * v[i];
  return result;
}

template<typename T, int Dim, typename S >
inline VectorT<T, Dim> operator*(const VectorT<T, Dim>& v, S s)
{
  T scalar = static_cast<T>(s);
  VectorT<T, Dim> result;
  for(int i=0;i<Dim;i++)
    result[i] = v[i] * scalar;
  return result;
}

template<typename T, int Dim>
inline T dot(const VectorT<T, Dim>& v1, const VectorT<T, Dim>& v2)
{
  T result = 0;
  for(int i=0;i<Dim;i++)
    result += v1[i] * v2[i];
  return result;
}

template<typename T, int Dim>
inline T dot3(const VectorT<T, Dim>& v1, const VectorT<T, Dim>& v2)
{
  T result = 0;
  for(int i=0;i<3;i++)
    result += v1[i] * v2[i];
  return result;
}

template<typename T, int Dim>
inline VectorT<T, Dim> lerp(const VectorT<T, Dim>& v1, const VectorT<T, Dim>& v2, const VectorT<T, Dim>& v3)
{
  VectorT<T, Dim> result;  
  for(int i=0;i<Dim;i++)
    result[i] = v1[i] + (v2[i] - v1[i]) * v3[i];
  return result;
}

template<typename T, int Dim>
inline VectorT<T, Dim> lerp(const VectorT<T, Dim>& v1, const VectorT<T, Dim>& v2, T v3)
{
  VectorT<T, Dim> result;
  for(int i=0;i<Dim;i++)
    result[i] = v1[i] + (v2[i] - v1[i]) * v3; 
  return result;
}


template<typename T, int Dim>
inline istream &operator>>(istream &is, VectorT<T, Dim> &t) {
   for(int i=0; i<Dim; i++)
    is >> t[i];
   return is;
}

template<typename T, int Dim>
inline ostream &operator<<(ostream &os, const VectorT<T, Dim> &t) {
   for(int i=0; i<Dim; i++)
    os << t[i] << " ";
   return os;
}

template<typename T, int Dim>
inline VectorT<T, Dim> normalize(const VectorT<T, Dim>& v) {
  VectorT<T, Dim> result = v;
  result.normalize();
  return result;
}


// Cross product is only defined for R3
template<typename T>
inline VectorT<T, 3> cross(const VectorT<T, 3>& v1, const VectorT<T, 3>& v2)
{
  return VectorT<T, 3>(v1[1]*v2[2] - v1[2]*v2[1],
                       v1[2]*v2[0] - v1[0]*v2[2],
                       v1[0]*v2[1] - v1[1]*v2[0]);
}

template<typename T, int Dim>
inline VectorT<T, Dim> min(const VectorT<T, Dim>& v1,
                           const VectorT<T, Dim>& v2)
{
  VectorT<T, Dim> result;
  for(int i=0;i<Dim;i++)
    result[i] = std::min(v1[i], v2[i]);
  return result;
}

template<typename T, int Dim>
inline VectorT<T, Dim> max(const VectorT<T, Dim>& v1,
                           const VectorT<T, Dim>& v2)
{
  VectorT<T, Dim> result;
  for(int i=0;i<Dim;i++)
    result[i] = std::max(v1[i], v2[i]);
  return result;
}

template<typename T, int Dim>
inline float length(const VectorT<T, Dim>& v)
{
	return v.length();
}

typedef VectorT<float, 2> vec2;
typedef VectorT<float, 3> vec3;
typedef VectorT<float, 4> vec4;

typedef VectorT<float, 2> vec2f;
typedef VectorT<float, 3> vec3f;
typedef VectorT<float, 4> vec4f;

typedef VectorT<int, 2> vec2i;
typedef VectorT<int, 3> vec3i;
typedef VectorT<int, 4> vec4i;

typedef VectorT<unsigned int, 2> vec2ui;
typedef VectorT<unsigned int, 3> vec3ui;
typedef VectorT<unsigned int, 4> vec4ui;

typedef VectorT<unsigned char, 2> vec2uc;
typedef VectorT<unsigned char, 3> vec3uc;
typedef VectorT<unsigned char, 4> vec4uc;


#endif
