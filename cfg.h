
#ifndef _CFG_OP_H_
#define _CFG_OP_H_

typedef void* CFG_HANDLE;

#ifdef __cplusplus
extern "C"{
#endif /*__cplusplus*/

CFG_HANDLE cfg_create(const char*);
const char* cfg_get_str(const char*, const char*, CFG_HANDLE);
int  cfg_get_int(const char*, const char*, int, CFG_HANDLE);
long cfg_get_long(const char*, const char*, long, CFG_HANDLE);
long long cfg_get_longlong(const char*, const char*, long long, CFG_HANDLE);
float cfg_get_float(const char*, const char*, float, CFG_HANDLE);
double cfg_get_double(const char*, const char*, double, CFG_HANDLE);

void cfg_release(CFG_HANDLE);

/*
like
GetPrivateProfileString
*/
unsigned int get_cfg_str_file(const char*, const char*, const char*, char*, unsigned int, const char*);

#ifdef __cplusplus
}
#endif /*__cplusplus*/

#endif/*_CFG_OP_H_*/