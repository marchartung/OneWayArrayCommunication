#pragma once

#include "Indicator.hpp"
#include <mpi.h>
#include <vector>
#include <limits>

template<typename T>
class Communicator
{
 public:
    Communicator(const int & target, const unsigned & numBuffer, const unsigned & sizeBuffer)
            : _numBuffer(numBuffer),
              _sizeBuffer(sizeBuffer),
              _charBufferSize(_sizeBuffer * sizeof(T) + sizeof(Indicator)),
              _rank(-1),
              _target(target),
              _numComms(0)
    {
        MPI_Comm_rank(MPI_COMM_WORLD, &_rank);
    }

    virtual ~Communicator()
    {
        for (unsigned i = 0; i < _mem.size(); ++i)
        {
            MPI_Free_mem(_mem[i]);
        }
        _mem.clear();
    }

 protected:

    void addBuffer()
    {
        char * p;
        MPI_Alloc_mem(_charBufferSize, MPI_INFO_NULL, &p);
        _reqs.push_back(MPI_REQUEST_NULL);
        _mem.push_back(p);
    }

    Indicator & getIndicatorOfWin(const unsigned & pos)
    {
        return *reinterpret_cast<Indicator*>(_mem[pos]);
    }

    const Indicator & getIndicatorOfWin(const unsigned & pos) const
    {
        return *reinterpret_cast<Indicator*>(_mem[pos]);
    }

    T * getDataOfWin(const unsigned & pos)
    {
        return reinterpret_cast<T*>(_mem[pos] + sizeof(Indicator));
    }

    const T * getDataOfWin(const unsigned & pos) const
    {
        return reinterpret_cast<T*>(_mem[pos] + sizeof(Indicator));
    }

    void openConnection(const unsigned & pos)
    {
        MPI_Irecv(_mem[pos], _charBufferSize, MPI_CHAR, _target, pos, MPI_COMM_WORLD, &_reqs[pos]);
    }

    bool checkIfRecvCompleted(const unsigned & pos)
    {
        int flag;
        MPI_Test(&_reqs[pos], &flag, MPI_STATUS_IGNORE);
        if (flag)
        {
            ++_numComms;
        }
        return flag;
    }

    bool checkIfFreeForSend(const unsigned & pos)
    {

        if (_reqs[pos] != MPI_REQUEST_NULL)
        {
            MPI_Wait(&_reqs[pos], MPI_STATUS_IGNORE);
            _reqs[pos] = MPI_REQUEST_NULL;
            this->getIndicatorOfWin(pos).clear();
        }
        return true;

    }

    void sendData(const unsigned & pos, const int & num)
    {
        int count = num * sizeof(T) + sizeof(Indicator);
        MPI_Isend(_mem[pos], count, MPI_CHAR, _target, pos, MPI_COMM_WORLD, &_reqs[pos]);
        ++_numComms;
    }

    unsigned getNumCreatedBuffer() const
    {
        return _mem.size();
    }

    unsigned getNumDoneCommunications() const
    {
        return _numComms;
    }

    unsigned getCharBufferSize() const
    {
        return _charBufferSize;
    }

    void finishReceiver()
    {
        unsigned tmp = _numComms;
        MPI_Send(&tmp, 1, MPI_UNSIGNED, _target, std::numeric_limits<int>::max(), MPI_COMM_WORLD);
        MPI_Status status[_reqs.size()];
        MPI_Waitall(_reqs.size(), _reqs.data(), status);
    }

    void finishSender()
    {
        unsigned tmp = 0;
        MPI_Recv(&tmp, 1, MPI_UNSIGNED, _target, std::numeric_limits<int>::max(), MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        if (tmp != _numComms)
            throw std::runtime_error("Cleanup failed");
        for (unsigned i = 0; i < _reqs.size(); ++i)
            MPI_Isend(NULL, 0, MPI_CHAR, _target, i, MPI_COMM_WORLD, &_reqs[i]);
        MPI_Status status[_reqs.size()];
        MPI_Waitall(_reqs.size(), _reqs.data(), status);
    }

    const unsigned & getNumBuffer() const
    {
        return _numBuffer;
    }

    const unsigned & getSizeBuffer() const
    {
        return _sizeBuffer;
    }

 private:

    const unsigned _numBuffer;
    const unsigned _sizeBuffer;
    const unsigned _charBufferSize;
    int _rank;
    int _target;
    unsigned _numComms;
    std::vector<MPI_Request> _reqs;
    std::vector<char*> _mem;

};

