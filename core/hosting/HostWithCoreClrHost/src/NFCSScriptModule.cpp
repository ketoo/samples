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

    csScriptModule.Test("ManagedLibrary", "ManagedWorker", "DoWork");

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
    InstallScriptSystem("ManagedLibrary.dll");
    BindScriptSystem();

    this->awake();

    return true;
}

bool NFCSScripteModule::Init()
{
    this->init();

    return true;
}

bool NFCSScripteModule::ReadyExecute()
{
    this->readyExecute();
    return true;
}

bool NFCSScripteModule::Execute()
{
     this->execute();
     return true;
}
bool NFCSScripteModule::AfterInit()
{
    this->afterInit();
    return true;
}

bool NFCSScripteModule::BeforeShut()
{
    this->beforeShut();
    return true;
}

bool NFCSScripteModule::Shut()
{
    this->shut();

    UnInstallScriptSystem();

    return true;
}

 bool NFCSScripteModule::InstallScriptSystem(const std::string& name)
 {
    UnInstallScriptSystem();

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
    managedLibraryPath.append(name);

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
    int hr = initializeCoreClr(
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

bool NFCSScripteModule::UnInstallScriptSystem()
{
    if (NFCSScripteModule::hostHandle != nullptr
    && NFCSScripteModule::domainId != -1)
    {
        // STEP 6: Shutdown CoreCLR
        //
        // <Snippet6>
        int hr = shutdownCoreClr(hostHandle, domainId);
        // </Snippet6>
        if (hr >= 0)
        {
            printf("CoreCLR successfully shutdown\n");
        }
        else
        {
            printf("coreclr_shutdown failed - status: 0x%08x\n", hr);
        }

        NFCSScripteModule::hostHandle = nullptr;
        NFCSScripteModule::domainId = -1;
    }

    return true;
}

void NFCSScripteModule::BindScriptSystem()
{
    std::string pluginNameSpace = "ManagedLibrary";
    std::string pluginClassName = "ManagedWorker";

    awake = (Awake_ptr)BindScriptFunction(pluginNameSpace, pluginClassName, "Awake");
    init = (Init_ptr)BindScriptFunction(pluginNameSpace, pluginClassName, "Init");
    afterInit = (AfterInit_ptr)BindScriptFunction(pluginNameSpace, pluginClassName, "AfterInit");
    readyExecute = (ReadyExecute_ptr)BindScriptFunction(pluginNameSpace, pluginClassName, "ReadyExecute");
    execute = (Execute_ptr)BindScriptFunction(pluginNameSpace, pluginClassName, "Execute");
    beforeShut = (BeforeShut_ptr)BindScriptFunction(pluginNameSpace, pluginClassName, "BeforeShut");
    shut = (Shut_ptr)BindScriptFunction(pluginNameSpace, pluginClassName, "Shut");
}

void* NFCSScripteModule::BindScriptFunction(const std::string& namSpace, const std::string& className, const std::string& functionName)
{
      // <Snippet5>
    std::string assemblyName = namSpace + "," + "Version=1.0.0.0";
    std::string typeName = namSpace + "." + className;

    const char* entryPointAssemblyName = assemblyName.c_str();
    const char* entryPointTypeName = typeName.c_str();

    void* managedDelegate = nullptr;
    // The assembly name passed in the third parameter is a managed assembly name
    // as described at https://docs.microsoft.com/dotnet/framework/app-domains/assembly-names
    int hr = createManagedDelegate(
            hostHandle,
            domainId,
            entryPointAssemblyName,
            entryPointTypeName,
            functionName.c_str(),
            (void**)&managedDelegate);
    // </Snippet5>

    if (hr >= 0)
    {
        printf("Managed delegate created\n");

        return managedDelegate;
    }
    else
    {
        printf("coreclr_create_delegate failed - status: 0x%08x\n", hr);
        
    }
        
    return nullptr;
}

void NFCSScripteModule::FreeString(char * p)
{
    // Strings returned to native code must be freed by the native code
    #if WINDOWS
        CoTaskMemFree(p);
    #elif LINUX
        free(p);
     #endif
}

bool NFCSScripteModule::Test(const std::string& namSpace, const std::string& className, const std::string& functionName)
{
    // <Snippet5>
    doWork_ptr dowork = (doWork_ptr)BindScriptFunction(namSpace, className, functionName);
    if (dowork != nullptr)
    {
        //The only part worth mention is that C# returned string needs to be freed. 
        //This is part of the contract between C# and native code that any memory ownership transfer needs to be freed using free (in Windows, 
        //it should be CoTaskMemFree). Otherwise you’ll create a leak.
        char* ret = dowork("Test job", 5, ReportProgressCallback);
        printf("Managed code returned: %s\n", ret);

        FreeString(ret);
 
    }


     return true;
 }


#if WINDOWS
// Win32 directory search for .dll files
// <Snippet7>
void NFCSScripteModule::BuildTpaList(const char* directory, const char* extension, std::string& tpaList)
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
void NFCSScripteModule::BuildTpaList(const char* directory, const char* extension, std::string& tpaList)
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