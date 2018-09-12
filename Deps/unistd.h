#ifndef _UNISTD_H
#define _UNISTD_H 
#include <io.h> 
#include <process.h> 

#if defined(WIN32) || defined(WIN64)

#define usleep(x) ::Sleep(x/1000)
#define getpid GetCurrentProcessId
#define close closesocket
#define access _access

/* access */
#define F_OK	0
#define X_OK	1
#define W_OK	2
#define R_OK	4

// Copied from linux libc sys/stat.h:
#define S_ISREG(m) (((m) & S_IFMT) == S_IFREG)
#define S_ISDIR(m) (((m) & S_IFMT) == S_IFDIR)

#endif

#endif /* _UNISTD_H */
