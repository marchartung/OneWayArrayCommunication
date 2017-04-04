#include <iostream>
#include <string>
#include "SendCommunicator.hpp"
#include "RecvCommunicator.hpp"
#include <pthread.h>
#include <ctime>
#include <omp.h>

typedef unsigned val_type;
typedef SendCommunicator<val_type> SComm;
typedef RecvCommunicator<val_type> RComm;

const unsigned numElements = 80000000;

struct TesterStruct
{
    SComm * sc;
    unsigned id;
    unsigned numThreads;
};

void * testThread(void * data)
{
    TesterStruct p = *reinterpret_cast<TesterStruct*>(data);
    SComm & c = *p.sc;
    unsigned id = p.id;
    unsigned numElementsPerThread = numElements / p.numThreads;
    for (size_t i = 0; i < numElementsPerThread; ++i)
        c.getCurThreadElement(id) = 1;
    return NULL;
}

void omptester(int target, unsigned numBuffer, unsigned sizeBuffer, unsigned numThreads)
{
    long long unsigned start_t = clock();
    SComm c(target, numBuffer, sizeBuffer);

    TesterStruct structs[numThreads];
    for (unsigned i = 0; i < numThreads; ++i)
    {
        structs[i].sc = &c;
        structs[i].id = i;
        structs[i].numThreads = numThreads;
    }

#pragma omp parallel num_threads(numThreads)
    {
        testThread(&structs[omp_get_thread_num()]);
    }
    c.finish();
    double usedTime = ((double) (clock() - start_t)) / CLOCKS_PER_SEC;
    double bandwidth = (sizeof(val_type) * numElements) / (usedTime * 1024 * 1024 * 1024);
    std::cout << "Sender finishing with check = " << numElements << " bandwidth: " << bandwidth << " GByte/s" << std::endl;
}

void tester(int target, unsigned numBuffer, unsigned sizeBuffer, unsigned numThreads)
{
    long long unsigned start_t = clock();
    SComm c(target, numBuffer, sizeBuffer);

    for (unsigned i = 0; i < numThreads; ++i)
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
    double usedTime = ((double) (clock() - start_t)) / CLOCKS_PER_SEC;
    double bandwidth = (sizeof(val_type) * numElements) / (usedTime * 1024 * 1024 * 1024);
    std::cout << "Sender finishing with check = " << numElements << " bandwidth: " << bandwidth << " GByte/s" << std::endl;
}

void receiver(int target, unsigned numBuffer, unsigned sizeBuffer)
{
    long long unsigned start_t = clock();

    RComm c(target, numBuffer, sizeBuffer);
    unsigned check = 0;
    unsigned num;
    unsigned counter = 0;
    const unsigned * p;
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
    //std::cout << "Recv finishing with check = " << check << " time: " << ((double) (clock() - start_t)) / CLOCKS_PER_SEC << "s" << std::endl;
}

int main(int argc, char * argv[])
{
    unsigned numBuffer = 5;
    unsigned sizeBuffer = 10000;
    unsigned numThreads = 1;

    MPI_Init(&argc, &argv);
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if (argc != 4)
        throw std::runtime_error("usage: ./prog [numBuffer] [sizeBuffer] [numThreads]");
    else
    {
        numBuffer = std::stoi(argv[1]);
        sizeBuffer = std::stoi(argv[2]);
        numThreads = std::stoi(argv[3]);
    }

    if (rank == 0)
        omptester(1, numBuffer, sizeBuffer, numThreads);
    else
        receiver(0, numBuffer, sizeBuffer);

    MPI_Finalize();
    return 0;
}
