// Author: Manuel Meßmer
// Email: manuel.messmer@tum.de

#ifndef TIMER_INCLUDE_H
#define TIMER_INCLUDE_H

//// STL includes
#include <chrono>

namespace queso {

///@name QuESo Classes
///@{

///
/**
 * @class  Timer
 * @author Manuel Messmer
 * @brief  Provides functions to measure the system's time.
*/
class Timer {
public:
    ///@name Life cycle
    ///@{

    /// Constructor
    Timer(){
        mTimeBegin = std::chrono::high_resolution_clock::now();
    }

    ///@}
    ///@name Operations
    ///@{

    ///@brief Reset clock to current time.
    void Reset() {
        mTimeBegin = std::chrono::high_resolution_clock::now();
    }

    ///@brief Returns duration since instantiation or last reset.
    double Measure(){
        const auto time_now = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = time_now - mTimeBegin;
        return duration.count();
    }
    ///@}

private:
    ///@name Private Member Variables
    ///@{
    std::chrono::time_point<std::chrono::high_resolution_clock> mTimeBegin;
    ///@}

}; // End class Timer
///@} // End QuESo classes

} // End namespace queso

#endif // TIMER_INCLUDE_H