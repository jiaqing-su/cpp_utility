
extern void dll_init(const char* fname);
extern void dll_finit(void);

#ifdef WIN32
#include <windows.h>
BOOL WINAPI DllMain(
  _In_  HINSTANCE hinstDLL,
  _In_  DWORD fdwReason,
  _In_  LPVOID lpvReserved
)
{
    switch(fdwReason){
    case DLL_PROCESS_ATTACH:
        {
            const char* fname = 0;
            char module_name[MAX_PATH] = {0};
            DWORD len = GetModuleFileNameA(hinstDLL, module_name, MAX_PATH);
            if(len > 0){
                while(len >= 0 && module_name[len] != '/' && module_name[len] != '\\'){
                    len--;
                }
                fname = module_name + len + 1;
            }
            dll_init(fname);
        }
        break;
    case DLL_PROCESS_DETACH:
        dll_finit();
        break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
        break;
    }
    return TRUE;
}
#else
void __attribute__ ((constructor)) my_init(void){
    const char* fname = 0;
#ifdef _GNU_SOURCE
#include <dlfcn.h>
//#include <string.h>
    Dl_info dllinfo;
    if (0 != dladdr((const void*)my_init, &dllinfo)) {
        fname = dllinfo.dli_fname;  
    }
#endif
    dll_init(fname);
}

void __attribute__ ((destructor)) my_fini(void){
    dll_finit();
}
#endif
