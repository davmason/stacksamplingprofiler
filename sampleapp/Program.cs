using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Threading;

// 68714 - with profiler

namespace sampleapp
{
    class Program
    {
        static void AllocateLots()
        {
            List<object> objects = new List<object>();
            int i = 0;
            while(true)
            {
                object o = new object();
                objects.Add(o);

                if (objects.Count > 200_000)
                {
                    ++i;
                    Console.WriteLine($"Hello {i}!");
                    objects.Clear();

                    if (i > 50)
                    {
                        return;
                    }
                }
            }
        }

        static void Main(string[] args)
        {
            Stopwatch sw = new Stopwatch();
            sw.Start();

            List<Thread> threads = new List<Thread>();
            for (int i = 0; i < 100; ++i)
            {
                Thread t = new Thread(AllocateLots);
                t.Start();
                threads.Add(t);
            }

            foreach (Thread t in threads)
            {
                t.Join();
            }

            sw.Stop();
            Console.WriteLine($"Total elapsed milliseconds = {sw.ElapsedMilliseconds}");
        }
    }
}
