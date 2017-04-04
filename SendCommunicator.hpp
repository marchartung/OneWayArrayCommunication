#pragma once

#include "Communicator.hpp"

template<typename T>
class SendCommunicator : public Communicator<T>
{
    typedef Communicator<T> Parent;
 public:
    SendCommunicator(const int & target, const unsigned & numBuffer, const unsigned & sizeBuffer)
            : Parent(target,numBuffer,sizeBuffer)
    {
        addThread(0);
    }

    ~SendCommunicator()
    {
        Parent::finishSender();
    }

    unsigned getNumThreads() const
    {
        return _curPosEntry.size();
    }

    void addThread(const unsigned & tid)
    {
        if (Parent::getNumCreatedBuffer() > 0)
        {
            Parent::getIndicatorOfWin(_curPosEntry[tid].winNum).setAddThread();
            forceCommunicationOnThread(tid);
        }
        unsigned startNew = Parent::getNumCreatedBuffer();
        for (unsigned i = 0; i < Parent::getNumBuffer(); ++i)
            Parent::addBuffer();
        _curPosEntry.push_back(getWinPosOfWin(startNew));
    }

    void forceCommunicationOnThread(const unsigned & tid)
    {
        WinPos & cur = _curPosEntry[tid];
        int count = (cur.cur - Parent::getDataOfWin(cur.winNum));
        Parent::getIndicatorOfWin(cur.winNum).setCount(count);

        Parent::sendData(cur.winNum, count);

        unsigned nextWin = (cur.winNum + 1 == (tid + 1) * Parent::getNumBuffer()) ? tid * Parent::getNumBuffer() : cur.winNum + 1;
        _curPosEntry[tid] = getWinPosOfWin(nextWin);
    }

    // after calling, the class will automatically shift to the next element
    T & getCurThreadElement(const unsigned & tid)
    {
        if (_curPosEntry[tid].needsUpdate())
        {
            forceCommunicationOnThread(tid);
            Parent::checkIfFreeForSend(_curPosEntry[tid].winNum);
        }
        return *(_curPosEntry[tid].cur++);  // implies shift to next element
    }

    void finish()
    {
        for (unsigned i = 1; i < _curPosEntry.size(); ++i)
            forceCommunicationOnThread(i);  // send everything what wasn't send
        Parent::getIndicatorOfWin(_curPosEntry[0].winNum).setFinished(Parent::getNumDoneCommunications()+1);
        forceCommunicationOnThread(0);
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

    std::vector<WinPos> _curPosEntry;

    WinPos getWinPosOfWin(const unsigned & pos)
    {
        return WinPos(Parent::getDataOfWin(pos), Parent::getDataOfWin(pos) + Parent::getSizeBuffer(), pos);
    }

};
