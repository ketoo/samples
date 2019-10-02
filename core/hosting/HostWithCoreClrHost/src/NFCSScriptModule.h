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

// Define OS-specific items like the CoreCLR library's name and path elements



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
#define MAX_PATH PATH_MAX
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
typedef char* (*doWork_ptr)(const char* jobName, int iterations, int dataSize, double* data, report_callback_ptr callbackFunction);

void BuildTpaList(const char* directory, const char* extension, std::string& tpaList);
int ReportProgressCallback(int progress);


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

	virtual bool Init()
    {

        // <Snippet5>
        doWork_ptr managedDelegate;

        // The assembly name passed in the third parameter is a managed assembly name
        // as described at https://docs.microsoft.com/dotnet/framework/app-domains/assembly-names
        hr = createManagedDelegate(
                hostHandle,
                domainId,
                "ManagedLibrary, Version=1.0.0.0",
                "ManagedLibrary.ManagedWorker",
                "DoWork",
                (void**)&managedDelegate);
        // </Snippet5>

        if (hr >= 0)
        {
            printf("Managed delegate created\n");
        }
        else
        {
            printf("coreclr_create_delegate failed - status: 0x%08x\n", hr);
            return -1;
        }

        // Create sample data for the double[] argument of the managed method to be called
        double data[4];
        data[0] = 0;
        data[1] = 0.25;
        data[2] = 0.5;
        data[3] = 0.75;

        // Invoke the managed delegate and write the returned string to the console
        char* ret = managedDelegate("Test job", 5, sizeof(data) / sizeof(double), data, ReportProgressCallback);

        printf("Managed code returned: %s\n", ret);

        // Strings returned to native code must be freed by the native code
    #if WINDOWS
        CoTaskMemFree(ret);
    #elif LINUX
        free(ret);
    #endif

        //
        // STEP 6: Shutdown CoreCLR
        //

        // <Snippet6>
        hr = shutdownCoreClr(hostHandle, domainId);
        // </Snippet6>

        if (hr >= 0)
        {
            printf("CoreCLR successfully shutdown\n");
        }
        else
        {
            printf("coreclr_shutdown failed - status: 0x%08x\n", hr);
        }

        return true;
    }

    virtual bool Shut()
    {
        return true;
    }

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
};

    //
    // STEP 5: Create delegate to managed code and invoke it
    //

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

// Callback function passed to managed code to facilitate calling back into native code with status
int ReportProgressCallback(int progress)
{
    std::cout << std::this_thread::get_id() << std::endl;
    // Just print the progress parameter to the console and return -progress
    printf("Received status from managed code: %d\n", progress);
    return -progress;
}
