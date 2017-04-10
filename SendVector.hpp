#pragma once

#include "MPICommunicator.hpp"

template<class Communicator>
class SendVector
{
 public:
    typedef typename Communicator::value_type T;
    SendVector(const int & target, const unsigned & numBuffer, Communicator & in)
            : _comm(in),
              _numBuffer(numBuffer)
    {
        addThread(0);
    }

    ~SendVector()
    {
        _comm.finishSender();
    }

    unsigned getNumThreads() const
    {
        return _curPosEntry.size();
    }

    void addThread(const unsigned & tid)
    {
        if (_comm.getNumCreatedBuffer() > 0)
        {
            _comm.getIndicatorOfWin(_curPosEntry[tid].winNum).setAddThread();
            forceCommunicationOnThread(tid);
        }
        unsigned startNew = _comm.getNumCreatedBuffer();
        for (unsigned i = 0; i < getNumBuffer(); ++i)
            _comm.addBuffer();
        _curPosEntry.push_back(getWinPosOfWin(startNew));
    }

    void forceCommunicationOnThread(const unsigned & tid)
    {
        WinPos & cur = _curPosEntry[tid];
        int count = (cur.cur - _comm.getDataOfWin(cur.winNum));
        _comm.getIndicatorOfWin(cur.winNum).setCount(count);

        _comm.sendData(cur.winNum, count);

        unsigned nextWin = (cur.winNum + 1 == (tid + 1) * getNumBuffer()) ? tid * getNumBuffer() : cur.winNum + 1;
        _curPosEntry[tid] = getWinPosOfWin(nextWin);
    }

    // after calling, the class will automatically shift to the next element
    T & getCurThreadElement(const unsigned & tid)
    {
        if (_curPosEntry[tid].needsUpdate())
        {
            forceCommunicationOnThread(tid);
            _comm.checkIfFreeForSend(_curPosEntry[tid].winNum);
        }
        return *(_curPosEntry[tid].cur++);  // implies shift to next element
    }

    void finish()
    {
        for (unsigned i = 1; i < _curPosEntry.size(); ++i)
            forceCommunicationOnThread(i);  // send everything what wasn't send
        _comm.getIndicatorOfWin(_curPosEntry[0].winNum).setFinished(_comm.getNumDoneCommunications()+1);
        forceCommunicationOnThread(0);
    }

    unsigned getNumBuffer() const
    {
       return _numBuffer;
    }

 private:
    struct WinPos
    {
        T * cur;
        const T * end;
        unsigned winNum;

        WinPos(T * start, T * end, const unsigned & winNum)
                : cur(start),
                  end(end),
                  winNum(winNum)
        {
        }

        WinPos & operator=(const WinPos & in)
        {
            cur = in.cur;
            end = in.end;
            winNum = in.winNum;
            return *this;
        }

        bool needsUpdate() const
        {
            return cur == end;
        }
    };

    Communicator & _comm;
    unsigned _numBuffer;
    std::vector<WinPos> _curPosEntry;

    WinPos getWinPosOfWin(const unsigned & pos)
    {
        return WinPos(_comm.getDataOfWin(pos), _comm.getDataOfWin(pos) + _comm.getSizeBuffer(), pos);
    }

};
