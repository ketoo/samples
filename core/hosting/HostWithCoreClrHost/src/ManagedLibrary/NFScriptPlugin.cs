﻿using System;
using System.Linq;
using System.Runtime.InteropServices;
using System.Threading;

namespace ManagedLibrary
{
    // Sample managed code for the host to call
    public class ManagedWorker
    {
        // This assembly is being built as an exe as a simple way to
        // get .NET Core runtime libraries deployed (`dotnet publish` will
        // publish .NET Core libraries for exes). Therefore, this assembly
        // requires an entry point method even though it is unused.
        public static void Main()
        {
            Console.WriteLine("This assembly is not meant to be run directly.");
            Console.WriteLine("Instead, please use the SampleHost process to load this assembly.");
        }

        public delegate int ReportProgressFunction(int progress);

        // This test method doesn't actually do anything, it just takes some input parameters,
        // waits (in a loop) for a bit, invoking the callback function periodically, and
        // then returns a string version of the double[] passed in.
        [return: MarshalAs(UnmanagedType.LPStr)]
        //MarshalAs(UnmanagedType.LPStruct)]
        
        public static void Awake()
        {
            Console.WriteLine("script system Awake");
        }
        
        public static void Init()
        {
            Console.WriteLine("script system Init");
        }
        
        public static void Shut()
        {
            Console.WriteLine("script system Shut");
        }
        
        public static void ReadyExecute()
        {
            Console.WriteLine("script system ReadyExecute");
        }

        public static void Execute()
        {
            Console.WriteLine("script system Execute");
        }

        public static void AfterInit()
        {
            Console.WriteLine("script system AfterInit");
        }

        public static void BeforeShut()
        {
            Console.WriteLine("script system BeforeShut");
        }

        public static string DoWork(
            [MarshalAs(UnmanagedType.LPStr)] string jobName,
            int iterations,
            ReportProgressFunction reportProgressFunction)
        {
                Console.WriteLine($"Thread ID: {System.Threading.Thread.CurrentThread.ManagedThreadId}");
            
            for (int i = 1; i <= iterations; i++)
            {
                Console.ForegroundColor = ConsoleColor.Cyan;
                Console.WriteLine($"Beginning work iteration {i}");
                Console.ResetColor();

                // Call the native callback and write its return value to the console
                var progressResponse = reportProgressFunction(i);
                Console.WriteLine($"Received response [{progressResponse}] from progress function");
            }

            Console.ForegroundColor = ConsoleColor.Red;
            Console.WriteLine($"Work completed");

            Console.ResetColor();

            return "completed";
        }
    }
}
