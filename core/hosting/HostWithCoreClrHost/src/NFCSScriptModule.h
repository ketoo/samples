#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <iostream>
#include <chrono>
#include <thread>

// https://github.com/dotnet/coreclr/blob/master/src/coreclr/hosts/inc/coreclrhost.h
#include "coreclrhost.h"


#if WINDOWS
#include <Windows.h>
#define FS_SEPARATOR "\\"
#define PATH_DELIMITER ";"

#define CORECLR_FILE_NAME "coreclr.dll"

#elif LINUX
#include <dirent.h>
#include <dlfcn.h>
#include <limits.h>

#define FS_SEPARATOR "/"
#define PATH_DELIMITER ":"
    #if OSX
    // For OSX, use Linux defines except that the CoreCLR runtime
    // library has a different name
    #define CORECLR_FILE_NAME "libcoreclr.dylib"
    #else
    #define CORECLR_FILE_NAME "libcoreclr.so"
    #endif
#endif

// Function pointer types for the managed call and callback
typedef void (*Awake_ptr)();
typedef void (*AfterInit_ptr)();
typedef void (*Init_ptr)();
typedef void (*ReadyExecute_ptr)();
typedef void (*Execute_ptr)();
typedef void (*BeforeShut_ptr)();
typedef void (*Shut_ptr)();



typedef int (*report_callback_ptr)(int progress);
typedef char* (*doWork_ptr)(const char* jobName, int iterations, report_callback_ptr callbackFunction);

// Callback function passed to managed code to facilitate calling back into native code with status
int ReportProgressCallback(int progress)
{
    std::cout << std::this_thread::get_id()  << "  Received progress from managed code:" << progress << std::endl;
    return progress * 100;
}

class NFCSScripteModule
{
private:
    static void* hostHandle;
    static unsigned int domainId;
    coreclr_create_delegate_ptr createManagedDelegate;
    coreclr_shutdown_ptr shutdownCoreClr;


public:
    NFCSScripteModule()
    {

    }

    virtual ~NFCSScripteModule()
    {

    }
    
    virtual bool Awake();
    virtual bool Init();
	
    virtual bool Shut();
    
    virtual bool Test(const std::string& namSpace, const std::string& className, const std::string& functionName);

	virtual bool ReadyExecute();

	virtual bool Execute();

    virtual bool AfterInit();

    virtual bool BeforeShut();

protected:
    bool InstallScriptSystem(const std::string& name);
    bool UnInstallScriptSystem();

    void BindScriptSystem();
    
    void* BindScriptFunction(const std::string& namSpace, const std::string& className, const std::string& functionName);

    void FreeString(char * p);
protected:

    void BuildTpaList(const char* directory, const char* extension, std::string& tpaList);


private:
    Awake_ptr awake;
    AfterInit_ptr afterInit;
    Init_ptr init;
    ReadyExecute_ptr readyExecute;
    Execute_ptr execute;
    BeforeShut_ptr beforeShut;
    Shut_ptr shut;

};
 