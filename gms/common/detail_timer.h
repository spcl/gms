#pragma once
#ifndef GMS_COMMON_DETAIL_TIMER_H_
#define GMS_COMMON_DETAIL_TIMER_H_

#include <vector>
#include <string>
#include <iostream>

#include <gms/third_party/gapbs/timer.h>

namespace GMS {

class DetailTimer {
public:
    Timer timer_;
    std::vector<std::string> phaseName;
    std::vector<double> phaseTime;

    DetailTimer() {
        timer_.Start();
    }
    void endPhase(const std::string &name) {
        timer_.Stop();
        phaseTime.push_back(timer_.Seconds());
        phaseName.push_back(name);
        timer_.Start();
    }
    void print() const {
        std::cout << "DetailTimer:" << std::endl;
        for(size_t i = 0; i < phaseTime.size(); i++) {
            std::cout << "   Phase " << phaseName[i] << ": " << phaseTime[i] << std::endl;
        }
    }
};

} // namespace GMS

#endif // GMS_COMMON_DETAIL_TIMER_H_