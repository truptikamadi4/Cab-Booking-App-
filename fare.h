// fare.h - Fare Calculation Module
#ifndef FARE_H
#define FARE_H

#include "models.h"
#include "utils.h"

struct FareBreakdown {
    double base = 0;
    double distanceCharge = 0;
    double nightCharge = 0;
    double waitCharge = 0;
    double total = 0;
};

class FareCalculator {
public:
    static constexpr double NIGHT_SURCHARGE = 0.25;  // +25% between 22:00 and 05:00
    static constexpr int FREE_WAIT_MINUTES = 5;      // first 5 minutes of waiting are free

    static bool isNight(const std::string& hhmm) {
        int h = 0;
        if (!util::validTime(hhmm, &h)) return false;
        return h >= 22 || h < 5;
    }

    // fare = base + km * rate, +25% at night, + waiting charge after free minutes
    static FareBreakdown compute(const CabType& cab, double km, bool night, int waitMin = 0) {
        FareBreakdown f;
        f.base = cab.baseFare;
        f.distanceCharge = km * cab.perKm;
        if (night) f.nightCharge = (f.base + f.distanceCharge) * NIGHT_SURCHARGE;
        int billable = waitMin > FREE_WAIT_MINUTES ? waitMin - FREE_WAIT_MINUTES : 0;
        f.waitCharge = billable * cab.waitPerMin;
        f.total = f.base + f.distanceCharge + f.nightCharge + f.waitCharge;
        return f;
    }
};

#endif
