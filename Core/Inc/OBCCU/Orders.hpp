#pragma once

#include "BMS-LIB.hpp"
#include "OBCCU/Conditions.hpp"
#include "OBCCU/Leds.hpp"
#include "OBCCU/Contactors.hpp"
#include "OBCCU/IMD.hpp"

namespace OBCCU {
    namespace Orders {
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
            imd.power.turn_on();
            Leds::full_charge.turn_on();
        };

        void turn_off_IMD() {
            imd.power.turn_off();
            Leds::full_charge.turn_off();
        };

        void reset() {
            NVIC_SystemReset();
        }
    };

    class IncomingOrders {
    public:
        HeapOrder start_charging_order;
        HeapOrder stop_charging_order;
        HeapOrder open_contactors_order;
        HeapOrder close_contactors_order;
        HeapOrder turn_on_IMD;
        HeapOrder turn_off_IMD;
        HeapOrder reset_board;

        IncomingOrders() : start_charging_order(900, Orders::start_charging),
                           stop_charging_order(901, Orders::stop_charging),
                           open_contactors_order(902, Orders::open_contactors),
                           close_contactors_order(903, Orders::close_contactors),
                           turn_on_IMD(904, Orders::turn_on_IMD),
                           turn_off_IMD(905, Orders::turn_off_IMD),
                           reset_board(906, Orders::reset) {}
    };

    class TCP {
    public:
        ServerSocket backend;

        TCP() {}

        void init() {
            backend = ServerSocket(IPV4("192.168.1.9"), 50500);
        }
    };
}