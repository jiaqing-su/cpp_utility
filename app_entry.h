
#ifndef _APP_ENTRY_H_
#define _APP_ENTRY_H_

#ifndef _VERSION_MAJOR
#define _VERSION_MAJOR 1
#endif

#ifndef _VERSION_MINOR
#define _VERSION_MINOR 0
#endif

#ifndef _REVISION
#define _REVISION      0
#endif

struct APP_ENV
{
	int argc;
	char** argv;
	int pid;
	int state;/*1:run, 0:stop*/
    const char* ver;
    const char* ussage;
    const char* appname;	
};

extern const APP_ENV* app;

#endif