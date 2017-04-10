#ifndef MPICOMMUNICATOR_HPP_
#define MPICOMMUNICATOR_HPP_

#include "Communicator.hpp"
#include <mpi.h>
#include <vector>
#include <limits>
#include <atomic>

template<typename T>
class MPICommunicator : public Communicator<T>
{
   typedef Communicator<T> Parent;
 public:
   MPICommunicator(const int & target, const unsigned & sizeBuffer)
         : Parent(sizeBuffer),
           _rank(-1),
           _target(target),
           _numComms(0)
   {
      MPI_Comm_rank(MPI_COMM_WORLD, &_rank);
   }

   virtual ~MPICommunicator()
   {
      for (unsigned i = 0; i < Parent::_mem.size(); ++i)
      {
         MPI_Free_mem (Parent::_mem[i]);
      }
      Parent::_mem.clear();
   }


   unsigned getNumDoneCommunications() const
   {
      return _numComms;
   }

   void addBuffer() override
   {
      char * p;
      MPI_Alloc_mem(Parent::getCharBufferSize(), MPI_INFO_NULL, &p);
      _reqs.push_back(MPI_REQUEST_NULL);
      Parent::_mem.push_back(p);
   }


   void openConnection(const unsigned & pos) override
   {
      MPI_Irecv(Parent::_mem[pos], Parent::getCharBufferSize(), MPI_CHAR, _target, pos, MPI_COMM_WORLD, &_reqs[pos]);
   }

   bool checkIfRecvCompleted(const unsigned & pos) override
   {
      int flag;
      MPI_Test(&_reqs[pos], &flag, MPI_STATUS_IGNORE);
      if (flag)
      {
         ++_numComms;
      }
      return flag;
   }

   bool checkIfFreeForSend(const unsigned & pos) override
   {

      if (_reqs[pos] != MPI_REQUEST_NULL)
      {
         MPI_Wait(&_reqs[pos], MPI_STATUS_IGNORE);
         _reqs[pos] = MPI_REQUEST_NULL;
         Parent::getIndicatorOfWin(pos).clear();
      }
      return true;

   }

   void sendData(const unsigned & pos, const int & num) override
   {
      int count = num * sizeof(T) + sizeof(Indicator);
      MPI_Isend(Parent::_mem[pos], count, MPI_CHAR, _target, pos, MPI_COMM_WORLD, &_reqs[pos]);
      ++_numComms;
   }


   void finishReceiver() override
   {
      unsigned tmp = _numComms;
      MPI_Send(&tmp, 1, MPI_UNSIGNED, _target, _reqs.size() + 1, MPI_COMM_WORLD);
      MPI_Status status[_reqs.size()];
      MPI_Waitall(_reqs.size(), _reqs.data(), status);
   }

   void finishSender() override
   {
      unsigned tmp = 0;
      MPI_Recv(&tmp, 1, MPI_UNSIGNED, _target, _reqs.size() + 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
      if (tmp != _numComms)
         throw std::runtime_error("Cleanup failed");
      for (unsigned i = 0; i < _reqs.size(); ++i)
         MPI_Isend(NULL, 0, MPI_CHAR, _target, i, MPI_COMM_WORLD, &_reqs[i]);
      MPI_Status status[_reqs.size()];
      MPI_Waitall(_reqs.size(), _reqs.data(), status);
   }

 private:
   int _rank;
   int _target;
   std::atomic<unsigned> _numComms;
   std::vector<MPI_Request> _reqs;

};

#endif

