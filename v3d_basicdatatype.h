#pragma once


// add this back 1/31/2020, by PHC


// be compatible with LP64(unix64) and LLP64(win64)
typedef unsigned char v3d_uint8;
typedef unsigned short v3d_uint16;
typedef unsigned int v3d_uint32;
typedef unsigned long long v3d_uint64;
typedef char v3d_sint8;
typedef short v3d_sint16;
typedef int v3d_sint32;
typedef long long v3d_sint64;
typedef float v3d_float32;
typedef double v3d_float64;

typedef void* v3dhandle;

//2010-05-19: by Hanchuan Peng. add the MSVC specific version # (vc 2008 has a _MSC_VER=1500) and win64 macro. 
//Note that _WIN32 seems always defined for any windows application.
//For more info see page for example: http://msdn.microsoft.com/en-us/library/b0084kay%28VS.80%29.aspx

#if defined(_MSC_VER) && (_WIN64)
//#if defined(_MSC_VER) && defined(_WIN64) //correct?

#define V3DLONG long long

#else

#define V3DLONG long

#endif

#if defined (_MSC_VER)

#define strcasecmp strcmpi

#endif

enum ImagePixelType { V3D_UNKNOWN, V3D_UINT8, V3D_UINT16, V3D_THREEBYTE, V3D_FLOAT32 };

enum TimePackType { TIME_PACK_NONE, TIME_PACK_Z, TIME_PACK_C };
