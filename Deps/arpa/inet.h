
#ifndef _I_NET_H__
#define _I_NET_H__

#define __STDC__ 1

#include <Winsock2.h>


// Try to determine endianness:
#if ( \
	defined(__i386__) || defined(__alpha__) || \
	defined(__ia64) || defined(__ia64__) || \
	defined(_M_IX86) || defined(_M_IA64) || \
	defined(_M_ALPHA) || defined(__amd64) || \
	defined(__amd64__) || defined(_M_AMD64) || \
	defined(__x86_64) || defined(__x86_64__) || \
	defined(_M_X64) || defined(__bfin__) || \
	defined(__ARMEL__) || \
	(defined(_WIN32) && defined(__ARM__) && defined(_MSC_VER)) \
)
#define MS_LITTLE_ENDIAN
#elif ( \
	defined (__ARMEB__) || defined(__sparc) || defined(__powerpc__) || defined(__POWERPC__) \
)
#define MS_BIG_ENDIAN
#else
#error Cannot determine endianness of this platform
#endif


#endif //_I_NET_H__
