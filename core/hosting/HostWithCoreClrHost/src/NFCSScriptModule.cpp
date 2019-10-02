#include "NFCSScriptModule.h"

void* NFCSScripteModule::hostHandle = nullptr;
unsigned int NFCSScripteModule::domainId = -1;

char* arg0;

int main(int argc, char* argv[])
{
    arg0 = argv[0];

    std::cout << "main id:" << std::this_thread::get_id() << " path:" << argv[0] << std::endl;
    NFCSScripteModule csScriptModule;
    csScriptModule.Awake();
    csScriptModule.Init();
    csScriptModule.AfterInit();
    csScriptModule.ReadyExecute();

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
    char runtimePath[MAX_PATH];
#if WINDOWS
    GetFullPathNameA(arg0, MAX_PATH, runtimePath, NULL);
#elif LINUX
    realpath(arg0, runtimePath);
#endif
    printf("0)%s\n", runtimePath);
    char *last_slash = strrchr(runtimePath, FS_SEPARATOR[0]);
    if (last_slash != NULL)
        *last_slash = 0;



    // Construct the CoreCLR path
    // For this sample, we know CoreCLR's path. For other hosts,
    // it may be necessary to probe for coreclr.dll/libcoreclr.so
    //std::string coreClrPath(runtimePath);
    std::string coreClrPath = "./";

    printf("1)%s\n", coreClrPath.c_str());

    coreClrPath.append(FS_SEPARATOR);
    printf("2)%s\n", coreClrPath.c_str());
    coreClrPath.append(CORECLR_FILE_NAME);
    printf("3)%s\n", coreClrPath.c_str());

    // Construct the managed library path
    std::string managedLibraryPath(runtimePath);
    //std::string managedLibraryPath(runtimePath);
    managedLibraryPath.append(FS_SEPARATOR);
    managedLibraryPath.append(MANAGED_ASSEMBLY);

    printf("4)%s\n", managedLibraryPath.c_str());
        //
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

        printf("STEP 3: Construct properties used when starting the runtime\n");
        //
        // STEP 3: Construct properties used when starting the runtime
        //

        // Construct the trusted platform assemblies (TPA) list
        // This is the list of assemblies that .NET Core can load as
        // trusted system assemblies.
        // For this host (as with most), assemblies next to CoreCLR will
        // be included in the TPA list
        std::string tpaList;
        BuildTpaList(runtimePath, ".dll", tpaList);

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

        printf("STEP 4: Start the CoreCLR runtime %s %s\n", coreClrPath.c_str(), managedLibraryPath.c_str());
        //printf("ARGS: Start the CoreCLR runtime %s  %s  ==> %s\n", runtimePath, propertyKeys[0], propertyValues[0]);
        //
        // STEP 4: Start the CoreCLR runtime
        //

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