#pragma once

#include "BMS-LIB.hpp"
#include "OBCCU/Conditions.hpp"
#include "OBCCU/Leds.hpp"
#include "OBCCU/Contactors.hpp"
#include "OBCCU/IMD.hpp"

namespace OBCCU {
    namespace Actions {
        void start_charging() {
            Conditions::want_to_charge = true;
        };

        void stop_charging() {
            Conditions::want_to_charge = false;
        };

        void open_contactors() {
            Contactors::high.turn_off();
            Contactors::low.turn_off();
            Conditions::contactors_closed = false;

            Leds::can.turn_off();
        };

        // void close_contactors() {
        //     Contactors::low.turn_on();
        //     Contactors::high.turn_on();
        //     Conditions::contactors_closed = true;

        //     Leds::can.turn_on();
        // };

        // TSD
        void close_contactors() {
            Contactors::low.turn_on();

            Time::set_timeout(1500, []() {
                Contactors::high.turn_on();
                Time::set_timeout(500, [&]() {
                    Contactors::low.turn_off();
                    Conditions::contactors_closed = true;
                    Leds::can.turn_on();
                });
            });
        };

        void turn_on_IMD() {
            OBCCU::imd.turn_on();
            Leds::full_charge.turn_on();
        };

        void turn_off_IMD() {
            OBCCU::imd.turn_off();
            Leds::full_charge.turn_off();
        };

        void reset() {
            NVIC_SystemReset();
        }
    }
}