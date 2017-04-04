/*
 * Indicator.hpp
 *
 *  Created on: 04.04.2017
 *      Author: hartung
 */

#ifndef INDICATOR_HPP_
#define INDICATOR_HPP_

class Indicator
    {
        unsigned status;
        unsigned count;
    public:
        bool isFree() const
        {
            return count == 0;
        }

        void clear()
        {
            status = 0;
            count = 0;
        }

        void setFree()
        {
            count = 0;
        }

        unsigned getCount() const
        {
            return count;
        }

        void setCount(const unsigned & in)
        {
            count = in;
        }

        bool shouldFinish() const
        {
            return status >= 2;
        }

        bool shouldAddThread() const
        {
            return status == 1;
        }

        void setAddThread()
        {
            status = 1;
        }

        // only call when shouldFinish is true
        unsigned getNumComms()
        {
            return status-2;
        }

        void setFinished(const unsigned & in)
        {
            status = in + 2;
        }

        void setOnlyElements()
        {
            status = 0;
        }
    };




#endif /* INDICATOR_HPP_ */
