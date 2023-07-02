#pragma once

#include "BMS-LIB.hpp"

namespace OBCCU {
    namespace Conditions {
        bool ready = false;
        bool want_to_charge = false;
        bool fault = false;
        bool contactors_closed = false;
        bool first_read = false;
    };
};