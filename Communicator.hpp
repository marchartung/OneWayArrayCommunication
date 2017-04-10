/*
 * Communicator.hpp
 *
 *  Created on: 10.04.2017
 *      Author: hartung
 */

#ifndef COMMUNICATOR_HPP_
#define COMMUNICATOR_HPP_

#include <vector>
#include "Indicator.hpp"

template<typename T>
class Communicator
{

 public:
   typedef T value_type;

   Communicator( const unsigned & sizeBuffer)
         : _sizeBuffer(sizeBuffer),
           _charBufferSize(_sizeBuffer * sizeof(T) + sizeof(Indicator))
   {
   }

   virtual ~Communicator()
   {
      for(unsigned i=0;i<_mem.size();++i)
         delete[] _mem[i];
   }

   virtual void addBuffer()
   {
      _mem.push_back( new char[_charBufferSize]);
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

   unsigned getNumCreatedBuffer() const
   {
      return _mem.size();
   }

   unsigned getCharBufferSize() const
   {
      return _charBufferSize;
   }

   const unsigned & getSizeBuffer() const
   {
      return _sizeBuffer;
   }

   virtual void openConnection(const unsigned & pos) = 0;

   virtual bool checkIfRecvCompleted(const unsigned & pos) = 0;

   virtual bool checkIfFreeForSend(const unsigned & pos) = 0;

   virtual void sendData(const unsigned & pos, const int & num) = 0;

   virtual void finishReceiver() = 0;

   virtual void finishSender() = 0;

 private:

   const unsigned _sizeBuffer;
   const unsigned _charBufferSize;
 protected:
   std::vector<char*> _mem;
};

#endif /* COMMUNICATOR_HPP_ */
