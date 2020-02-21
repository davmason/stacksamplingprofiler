using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Runtime.CompilerServices;
using System.Threading;

namespace MultiThreadedApp
{
    delegate void FuncCall(ref int stackDepth, ref bool done);

    public class RandoThread
    {
        public static bool KeepGoing = true;

        private static readonly int ThreadDepth = 20;

        private Random _rand = new Random();
        private FuncCall[] _funcs;

        public RandoThread()
        {
            _funcs = new FuncCall[] { A, B, C, D, E, F, G };
        }

        public void Start()
        {
            while(KeepGoing)
            {
                int stackDepth = 0;
                bool done = false;
                DoRandomCall(ref stackDepth, ref done);
            }
        }
        
        [MethodImpl(MethodImplOptions.NoInlining)]
        public void A(ref int stackDepth, ref bool done)
        {
            DoRandomCall(ref stackDepth, ref done);
        }

        [MethodImpl(MethodImplOptions.NoInlining)]
        public void B(ref int stackDepth, ref bool done)
        {
            DoRandomCall(ref stackDepth, ref done);
        }

        [MethodImpl(MethodImplOptions.NoInlining)]
        public void C(ref int stackDepth, ref bool done)
        {
            DoRandomCall(ref stackDepth, ref done);
        }

        [MethodImpl(MethodImplOptions.NoInlining)]
        public void D(ref int stackDepth, ref bool done)
        {
            DoRandomCall(ref stackDepth, ref done);
        }

        [MethodImpl(MethodImplOptions.NoInlining)]
        public void E(ref int stackDepth, ref bool done)
        {
            DoRandomCall(ref stackDepth, ref done);
        }

        [MethodImpl(MethodImplOptions.NoInlining)]
        public void F(ref int stackDepth, ref bool done)
        {
            DoRandomCall(ref stackDepth, ref done);
        }

        [MethodImpl(MethodImplOptions.NoInlining)]
        public void G(ref int stackDepth, ref bool done)
        {
            DoRandomCall(ref stackDepth, ref done);
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        private bool CheckDepth(ref int stackDepth, ref bool done)
        {
            if (done || stackDepth > ThreadDepth)
            {
                done = true;
                return false;
            }

            return true;
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        private void DoRandomCall(ref int stackDepth, ref bool done)
        {
            Thread.Sleep(0);

            ++stackDepth;

            if (!CheckDepth(ref stackDepth, ref done))
            {
                return;
            }

            int methodToCall = _rand.Next(0, _funcs.Length);
            _funcs[methodToCall](ref stackDepth, ref done);
        }
    }
    class Program
    {
        static void ThreadFunc()
        {
            RandoThread rt = new RandoThread();
            rt.Start();
        }

        static void Main(string[] args)
        {
            int numThreads = 10;
            if (args.Length > 0)
            {
                numThreads = int.Parse(args[0]);
            }

            Console.WriteLine($"Process ID={Process.GetCurrentProcess().Id}");
            Console.WriteLine($"Running with {numThreads} threads");
            List<Thread> threads = new List<Thread>();
            for(int i = 0; i < 100; ++i)
            {
                Thread t = new Thread(new ThreadStart(ThreadFunc));
                t.Start();
            }

            Console.WriteLine("Waiting for input, press enter to exit.");
            Console.ReadLine();

            RandoThread.KeepGoing = false;

            foreach (Thread t in threads)
            {
                t.Join();
            }
        }
    }
}
