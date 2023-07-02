#pragma once

#include "BMS-LIB.hpp"
#include "OBCCU/Conditions.hpp"
#include "OBCCU/Leds.hpp"
#include "OBCCU/Contactors.hpp"
#include "OBCCU/IMD.hpp"
#include "OBCCU/Actions.hpp"
#include "OBCCU/StateMachine.hpp"

namespace OBCCU {
    class IncomingOrders {
    public:
        HeapStateOrder start_charging_order;
        HeapStateOrder stop_charging_order;
        HeapStateOrder open_contactors_order;
        HeapStateOrder close_contactors_order;
        HeapStateOrder turn_on_IMD;
        HeapStateOrder turn_off_IMD;
        HeapOrder reset_board;

        IncomingOrders() : start_charging_order(900, Actions::start_charging, StateMachines::operational, States::Operational::IDLE),
                           stop_charging_order(901, Actions::stop_charging, StateMachines::operational, States::Operational::CHARGING),
                           open_contactors_order(902, Actions::open_contactors, StateMachines::contactors_sm, States::Contactors::CLOSED),
                           close_contactors_order(903, Actions::close_contactors, StateMachines::contactors_sm, States::Contactors::OPEN),
                           turn_on_IMD(904, Actions::turn_on_IMD, StateMachines::imd_sm, States::IMD::OFF),
                           turn_off_IMD(905, Actions::turn_off_IMD, StateMachines::imd_sm, States::IMD::ON),
                           reset_board(906, Actions::reset) {}
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