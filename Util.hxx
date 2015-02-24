#ifndef __UTIL_HXX__
#define __UTIL_HXX__

#include <iostream>
#include <fstream>
#include <sstream>
#include <stdlib.h>
#include <cstring>

//AARONBAD -- put this in BallAndStick.hxx
//#define BAS_TEXTURE_BUFFER

//#define MINMAX_MACROCELLS
//#define OTF_MACROCELLS
//#define BPG_MACROCELLS
//#define CS_MACROCELLS
#define PREINTEGRATED
//#define PREPF
#define ISOMAP
//#define ISOMAP4
//#define ISO_MCELLS

#ifndef MIN
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif
#ifndef MAX
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#endif

#define LERP(a, b, c) ((a) + ((b-a)*c))

#define SWAP(a,b,temp)   ((temp)=(a),(a)=(b),(b)=(temp))

#ifdef WIN32
#define M_PI 3.14159263589793
#endif


#ifdef WIN32
#define FATAL(a)     { std::cout << "FATAL error: " << a << std::endl; exit(0); }
#else
#define FATAL(a)     { std::cout << "FATAL error in " << __PRETTY_FUNCTION__ << " : " << a << std::endl; exit(0); }
#endif

#define FAIL FATAL

#ifndef NULL
#define NULL 0
#endif

size_t fread_big(void *ptr, size_t size, size_t nitems, FILE *stream);

#endif
