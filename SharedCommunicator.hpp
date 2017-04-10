#ifndef SHAREDCOMMUNICATOR_HPP_
#define SHAREDCOMMUNICATOR_HPP_

#include "Communicator.hpp"
#include <mpi.h>
#include <vector>
#include <limits>
#include <atomic>

template<typename T>
class SharedCommunicator : public Communicator<T>
{
   typedef Communicator<T> Parent;
 public:
   SharedCommunicator(const unsigned & sizeBuffer)
         : Parent(sizeBuffer),
           _numComms(0)
   {
   }

   virtual ~SharedCommunicator()
   {
   }

   unsigned getNumDoneCommunications() const
   {
      return _numComms;
   }

   void addBuffer() override
   {
      _reqs.push_back(false);
      Parent::addBuffer();
   }

   void openConnection(const unsigned & pos) override
   {
      _reqs[pos] = false;
   }

   bool checkIfRecvCompleted(const unsigned & pos) override
   {
      if (_reqs[pos])
      {
         ++_numComms;
         return true;
      }
      else
         return false;
   }

   bool checkIfFreeForSend(const unsigned & pos) override
   {

      if (!_reqs[pos])
      {
         Parent::getIndicatorOfWin(pos).clear();
         return true;
      }
      else
         return false;

   }

   void sendData(const unsigned & pos, const int & num) override
   {
      _reqs[pos] = true;
      ++_numComms;
   }

   void finishReceiver() override
   {
   }

   void finishSender() override
   {
   }

 private:
   std::atomic<unsigned> _numComms;
   std::vector<bool> _reqs;

};

#endif

