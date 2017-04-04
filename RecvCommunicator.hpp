#pragma once
#include "Communicator.hpp"
#include <tuple>

template<typename T>
class RecvCommunicator : public Communicator<T>
{

    typedef Communicator<T> Parent;
 public:
    RecvCommunicator(const int & target, const unsigned & numBuffer, const unsigned & sizeBuffer)
            : Parent(target,numBuffer,sizeBuffer),
              _isFinishedWithNumComms(std::numeric_limits<unsigned>::max())
    {
        addThread();
    }

    ~RecvCommunicator()
    {
        Parent::finishReceiver();
    }

    unsigned getNumThreads() const
    {
        return _curPosEntry.size();
    }

    std::tuple<unsigned, const T*> getDataOfThread(const unsigned & tid) const
    {
        std::tuple<unsigned, const T*> res;
        std::get<0>(res) = Parent::getIndicatorOfWin(_curPosEntry[tid]).getCount();
        std::get<1>(res) = Parent::getDataOfWin(_curPosEntry[tid]);
        return res;
    }

    void freeLastDataOfThread(const unsigned & tid)
    {
        Parent::openConnection(_curPosEntry[tid]);
        _curPosEntry[tid] = (_curPosEntry[tid] + 1 == (tid + 1) * Parent::getNumBuffer()) ? tid * Parent::getNumBuffer() : _curPosEntry[tid] + 1;
    }

    bool hasNewDataOfThread(const unsigned & tid)
    {
        if (Parent::checkIfRecvCompleted(_curPosEntry[tid]))
        {
            Indicator indicator = Parent::getIndicatorOfWin(_curPosEntry[tid]);
            if (indicator.shouldFinish())
                _isFinishedWithNumComms = indicator.getNumComms();
            else if (indicator.shouldAddThread())
                addThread();
            return true;
        }
        else
            return false;
    }

    bool isFinished()
    {
        return _isFinishedWithNumComms != std::numeric_limits<unsigned>::max() && Parent::getNumDoneCommunications() == _isFinishedWithNumComms;
    }

 private:
    unsigned _isFinishedWithNumComms;
    std::vector<unsigned> _curPosEntry;

    void addThread()
    {
        unsigned startNew = Parent::getNumCreatedBuffer();
        for (unsigned i = 0; i < Parent::getNumBuffer(); ++i)
        {
            Parent::addBuffer();
            Parent::openConnection(Parent::getNumCreatedBuffer() - 1);
        }
        _curPosEntry.push_back(startNew);

    }
};
