#include "NFCSScriptModule.h"

void* NFCSScripteModule::hostHandle = nullptr;
unsigned int NFCSScripteModule::domainId = -1;

char* filePath;

int main(int argc, char* argv[])
{
    filePath = argv[0];

    std::cout << "main id:" << std::this_thread::get_id() << " path:" << argv[0] << std::endl;
    NFCSScripteModule csScriptModule;
    csScriptModule.Awake();
    csScriptModule.Init();
    csScriptModule.AfterInit();
    csScriptModule.ReadyExecute();

    csScriptModule.Test("DoWork");

    //while(1)
    {
        csScriptModule.Execute();
    }

    csScriptModule.BeforeShut();
    csScriptModule.Shut();
  
    return 0;
}

bool NFCSScripteModule::Awake()
{
    // Get the current executable's directory
    // This sample assumes that both CoreCLR and the
    // managed assembly to be loaded are next to this host
    // so we need to get the current path in order to locate those.
    char runtimePath[1024];
#if WINDOWS
    GetFullPathNameA(filePath, MAX_PATH, runtimePath, NULL);
#elif LINUX
    realpath(filePath, runtimePath);
#endif
    printf("0)%s\n", runtimePath);
    char *last_slash = strrchr(runtimePath, FS_SEPARATOR[0]);
    if (last_slash != NULL)
        *last_slash = 0;

    std::string coreClrPath = "./";
    coreClrPath.append(FS_SEPARATOR);
    coreClrPath.append(CORECLR_FILE_NAME);

    std::string managedLibraryPath("./");
    managedLibraryPath.append(FS_SEPARATOR);
    managedLibraryPath.append(MANAGED_ASSEMBLY);

        // STEP 1: Load CoreCLR (coreclr.dll/libcoreclr.so)
        //
    #if WINDOWS
        // <Snippet1>
        HMODULE coreClr = LoadLibraryExA(coreClrPath.c_str(), NULL, 0);
        // </Snippet1>
    #elif LINUX
        void *coreClr = dlopen(coreClrPath.c_str(), RTLD_NOW | RTLD_LOCAL);
    #endif
        if (coreClr == NULL)
        {
            printf("ERROR: Failed to load CoreCLR from %s\n", coreClrPath.c_str());
            return -1;
        }
        else
        {
            printf("Loaded CoreCLR from %s\n", coreClrPath.c_str());
        }

        printf("STEP 2: Get CoreCLR hosting functions\n");
        //
        // STEP 2: Get CoreCLR hosting functions
        //
    #if WINDOWS
        // <Snippet2>
        coreclr_initialize_ptr initializeCoreClr = (coreclr_initialize_ptr)GetProcAddress(coreClr, "coreclr_initialize");
        createManagedDelegate = (coreclr_create_delegate_ptr)GetProcAddress(coreClr, "coreclr_create_delegate");
        shutdownCoreClr = (coreclr_shutdown_ptr)GetProcAddress(coreClr, "coreclr_shutdown");
        // </Snippet2>
    #elif LINUX
        coreclr_initialize_ptr initializeCoreClr = (coreclr_initialize_ptr)dlsym(coreClr, "coreclr_initialize");
        createManagedDelegate = (coreclr_create_delegate_ptr)dlsym(coreClr, "coreclr_create_delegate");
        shutdownCoreClr = (coreclr_shutdown_ptr)dlsym(coreClr, "coreclr_shutdown");
    #endif

        if (initializeCoreClr == NULL)
        {
            printf("coreclr_initialize not found");
            return -1;
        }

        if (createManagedDelegate == NULL)
        {
            printf("coreclr_create_delegate not found");
            return -1;
        }

        if (shutdownCoreClr == NULL)
        {
            printf("coreclr_shutdown not found");
            return -1;
        }

        // STEP 3: Construct properties used when starting the runtime
        //

        // Construct the trusted platform assemblies (TPA) list
        // This is the list of assemblies that .NET Core can load as
        // trusted system assemblies.
        // For this host (as with most), assemblies next to CoreCLR will
        // be included in the TPA list
        std::string tpaList;
        this->BuildTpaList(runtimePath, ".dll", tpaList);
        
        // <Snippet3>
        // Define CoreCLR properties
        // Other properties related to assembly loading are common here,
        // but for this simple sample, TRUSTED_PLATFORM_ASSEMBLIES is all
        // that is needed. Check hosting documentation for other common properties.
        const char* propertyKeys[] = {
            "TRUSTED_PLATFORM_ASSEMBLIES"      // Trusted assemblies
        };

        const char* propertyValues[] = {
            tpaList.c_str()
        };
        // </Snippet3>

        // STEP 4: Start the CoreCLR runtime

        // <Snippet4>
 

        // This function both starts the .NET Core runtime and creates
        // the default (and only) AppDomain
        hr = initializeCoreClr(
                        runtimePath,        // App base path
                        "SampleHost",       // AppDomain friendly name
                        sizeof(propertyKeys) / sizeof(char*),   // Property count
                        propertyKeys,       // Property names
                        propertyValues,     // Property values
                        &NFCSScripteModule::hostHandle,        // Host handle
                        &NFCSScripteModule::domainId);         // AppDomain ID
        // </Snippet4>

        if (hr >= 0)
        {
            printf("CoreCLR started\n");
        }
        else
        {
            printf("coreclr_initialize failed - status: 0x%08x\n", hr);
            return -1;
        }
        
        return true;
    }

bool NFCSScripteModule::Init()
{
    return true;
}

bool NFCSScripteModule::Shut()
{
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


 bool NFCSScripteModule::Test(const std::string& functionName)
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
            functionName.c_str(),
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


    // Invoke the managed delegate and write the returned string to the console
    char* ret = managedDelegate("Test job", 5, ReportProgressCallback);

    printf("Managed code returned: %s\n", ret);

    // Strings returned to native code must be freed by the native code
#if WINDOWS
    CoTaskMemFree(ret);
#elif LINUX
    free(ret);
#endif

     return true;
 }