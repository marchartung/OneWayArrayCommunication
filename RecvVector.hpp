#pragma once
#include <tuple>
#include "MPICommunicator.hpp"

template<class Communicator>
class RecvVector
{

 public:
   typedef typename Communicator::value_type T;

    RecvVector(const int & target, const unsigned & numBuffer, Communicator & comm)
            : _comm(comm),
              _isFinishedWithNumComms(std::numeric_limits<unsigned>::max()),
              _numBuffer(numBuffer)
    {
        addThread();
    }

    ~RecvVector()
    {
        _comm.finishReceiver();
    }

    unsigned getNumThreads() const
    {
        return _curPosEntry.size();
    }

    std::tuple<unsigned, const T*> getDataOfThread(const unsigned & tid) const
    {
        std::tuple<unsigned, const T*> res;
        std::get<0>(res) = _comm.getIndicatorOfWin(_curPosEntry[tid]).getCount();
        std::get<1>(res) = _comm.getDataOfWin(_curPosEntry[tid]);
        return res;
    }

    void freeLastDataOfThread(const unsigned & tid)
    {
        _comm.openConnection(_curPosEntry[tid]);
        _curPosEntry[tid] = (_curPosEntry[tid] + 1 == (tid + 1) * getNumBuffer()) ? tid * getNumBuffer() : _curPosEntry[tid] + 1;
    }

    bool hasNewDataOfThread(const unsigned & tid)
    {
        if (_comm.checkIfRecvCompleted(_curPosEntry[tid]))
        {
            Indicator indicator = _comm.getIndicatorOfWin(_curPosEntry[tid]);
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
        return _isFinishedWithNumComms != std::numeric_limits<unsigned>::max() && _comm.getNumDoneCommunications() == _isFinishedWithNumComms;
    }

    unsigned getNumBuffer() const
    {
       return _numBuffer;
    }

 private:
    Communicator & _comm;
    unsigned _isFinishedWithNumComms;
    unsigned _numBuffer;
    std::vector<unsigned> _curPosEntry;

    void addThread()
    {
        unsigned startNew = _comm.getNumCreatedBuffer();
        for (unsigned i = 0; i < getNumBuffer(); ++i)
        {
            _comm.addBuffer();
            _comm.openConnection(_comm.getNumCreatedBuffer() - 1);
        }
        _curPosEntry.push_back(startNew);

    }
};
