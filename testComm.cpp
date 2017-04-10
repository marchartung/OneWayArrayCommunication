#include <iostream>
#include <string>
#include <pthread.h>
#include <ctime>
#include <omp.h>

#include "RecvVector.hpp"
#include "SendVector.hpp"

struct Huge
{
   unsigned elem[4];

   operator unsigned() const
   {
      return elem[0];
   }

   Huge & operator=(const unsigned & in)
   {
      elem[0] = in;
      return *this;
   }
};

typedef Huge val_type;
typedef MPICommunicator<val_type> Comm;
typedef SendVector<Comm> SComm;
typedef RecvVector<Comm> RComm;

Comm * comm;

const unsigned numElements = 20000000;

struct TesterStruct
{
   SComm * sc;
   unsigned id;
   unsigned numThreads;
};

void * testThread(void * data)
{
   double start_t = MPI_Wtime();
   TesterStruct p = *reinterpret_cast<TesterStruct*>(data);
   SComm & c = *p.sc;
   unsigned id = p.id;
   unsigned numElementsPerThread = numElements / p.numThreads;
   for (size_t i = 0; i < numElementsPerThread; ++i)
      c.getCurThreadElement(id) = 1;
   double usedTime = ((double) (MPI_Wtime() - start_t));
   double bandwidth = (sizeof(val_type) * numElementsPerThread) / (usedTime * 1024 * 1024 * 1024);
   std::cout << "Sender " << id << " finishing with check = " << numElementsPerThread << " bandwidth: " << bandwidth << " GByte/s" << std::endl;
   return NULL;
}

void tester(int target, unsigned numBuffer, unsigned numThreads)
{
   SComm c(target, numBuffer, *comm);

   for (unsigned i = 0; i < numThreads - 1; ++i)
      c.addThread(0);
   TesterStruct structs[numThreads];

   pthread_t pids[numThreads - 1];
   for (unsigned i = 1; i < numThreads; ++i)
   {
      structs[i].sc = &c;
      structs[i].id = i;
      structs[i].numThreads = numThreads;

      pthread_create(&pids[i - 1], NULL, testThread, &structs[i]);
   }

   structs[0].sc = &c;
   structs[0].id = 0;
   structs[0].numThreads = numThreads;
   testThread(&structs[0]);

   for (unsigned i = 0; i < numThreads - 1; ++i)
      pthread_join(pids[i], NULL);

   c.finish();
}

void receiver(int target, unsigned numBuffer)
{
   double start_t = MPI_Wtime();

   RComm c(target, numBuffer, *comm);
   unsigned check = 0;
   unsigned num;
   unsigned counter = 0;
   const val_type * p;
   while (!c.isFinished())
   {
      ++counter;
      for (size_t i = 0; i < c.getNumThreads(); ++i)
      {
         if (c.hasNewDataOfThread(i))
         {
            std::tie(num, p) = c.getDataOfThread(i);
            for (unsigned j = 0; j < num; ++j)
               check += p[j];
            c.freeLastDataOfThread(i);
         }
      }
   }
   double usedTime = ((double) (MPI_Wtime() - start_t));
   double bandwidth = (sizeof(val_type) * numElements) / (usedTime * 1024 * 1024 * 1024);
   std::cout << "Receiver finishing with check = " << numElements << " bandwidth: " << bandwidth << " GByte/s" << std::endl;
}

int main(int argc, char * argv[])
{
   unsigned numBuffer = 5;
   unsigned sizeBuffer = 10000;
   unsigned numThreads = 1;

   if (argc != 4)
      throw std::runtime_error("usage: ./prog [numBuffer] [sizeBuffer] [numThreads]");
   else
   {
      numBuffer = std::stoi(argv[1]);
      sizeBuffer = std::stoi(argv[2]);
      numThreads = std::stoi(argv[3]);
   }
   if (numThreads > 1)
   {
      int provided;
      MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
      if (provided != MPI_THREAD_MULTIPLE)
         throw std::runtime_error("No threading supported with this mpi");
   } else
      MPI_Init(&argc, &argv);
   int rank;
   MPI_Comm_rank(MPI_COMM_WORLD, &rank);

   if (rank == 0)
   {
      comm = new Comm(1,sizeBuffer);
      tester(1, numBuffer, numThreads);
   }
   else
   {
      comm = new Comm(0,sizeBuffer);
      receiver(0, numBuffer);
   }

   delete comm;
   MPI_Finalize();
   return 0;
}
