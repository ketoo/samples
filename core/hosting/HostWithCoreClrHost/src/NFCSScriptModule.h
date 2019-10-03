#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <iostream>
#include <chrono>
#include <thread>


// https://github.com/dotnet/coreclr/blob/master/src/coreclr/hosts/inc/coreclrhost.h
#include "coreclrhost.h"

#define MANAGED_ASSEMBLY "ManagedLibrary.dll"

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
public:
    int hr;
    static void* hostHandle;
    static unsigned int domainId;
    coreclr_create_delegate_ptr createManagedDelegate;
    coreclr_shutdown_ptr shutdownCoreClr;

    NFCSScripteModule()
    {

    }

    virtual ~NFCSScripteModule()
    {

    }
    
    virtual bool Awake();
    virtual bool Init();
	
    virtual bool Shut();
    
    virtual bool Test(const std::string& functionName);

	virtual bool ReadyExecute()
    {
        return true;
    }

	virtual bool Execute()
    {
        return true;
    }

    virtual bool AfterInit()
    {
        return true;
    }

    virtual bool BeforeShut()
    {
        return true;
    }

protected:

#if WINDOWS
// Win32 directory search for .dll files
// <Snippet7>
void BuildTpaList(const char* directory, const char* extension, std::string& tpaList)
{
    // This will add all files with a .dll extension to the TPA list.
    // This will include unmanaged assemblies (coreclr.dll, for example) that don't
    // belong on the TPA list. In a real host, only managed assemblies that the host
    // expects to load should be included. Having extra unmanaged assemblies doesn't
    // cause anything to fail, though, so this function just enumerates all dll's in
    // order to keep this sample concise.
    std::string searchPath(directory);
    searchPath.append(FS_SEPARATOR);
    searchPath.append("*");
    searchPath.append(extension);

    WIN32_FIND_DATAA findData;
    HANDLE fileHandle = FindFirstFileA(searchPath.c_str(), &findData);

    if (fileHandle != INVALID_HANDLE_VALUE)
    {
        do
        {
            // Append the assembly to the list
            tpaList.append(directory);
            tpaList.append(FS_SEPARATOR);
            tpaList.append(findData.cFileName);
            tpaList.append(PATH_DELIMITER);

            // Note that the CLR does not guarantee which assembly will be loaded if an assembly
            // is in the TPA list multiple times (perhaps from different paths or perhaps with different NI/NI.dll
            // extensions. Therefore, a real host should probably add items to the list in priority order and only
            // add a file if it's not already present on the list.
            //
            // For this simple sample, though, and because we're only loading TPA assemblies from a single path,
            // and have no native images, we can ignore that complication.
        }
        while (FindNextFileA(fileHandle, &findData));
        FindClose(fileHandle);
    }
}
// </Snippet7>
#elif LINUX
// POSIX directory search for .dll files
void BuildTpaList(const char* directory, const char* extension, std::string& tpaList)
{
    DIR* dir = opendir(directory);
    struct dirent* entry;
    int extLength = strlen(extension);

    while ((entry = readdir(dir)) != NULL)
    {
        // This simple sample doesn't check for symlinks
        std::string filename(entry->d_name);

        // Check if the file has the right extension
        int extPos = filename.length() - extLength;
        if (extPos <= 0 || filename.compare(extPos, extLength, extension) != 0)
        {
            continue;
        }

        // Append the assembly to the list
        tpaList.append(directory);
        tpaList.append(FS_SEPARATOR);
        tpaList.append(filename);
        tpaList.append(PATH_DELIMITER);

        // Note that the CLR does not guarantee which assembly will be loaded if an assembly
        // is in the TPA list multiple times (perhaps from different paths or perhaps with different NI/NI.dll
        // extensions. Therefore, a real host should probably add items to the list in priority order and only
        // add a file if it's not already present on the list.
        //
        // For this simple sample, though, and because we're only loading TPA assemblies from a single path,
        // and have no native images, we can ignore that complication.
    }
}
#endif
};
 