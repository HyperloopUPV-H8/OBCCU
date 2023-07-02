#pragma once

#include "BMS-LIB.hpp"
#include "OBCCU/Orders.hpp"
#include "OBCCU/IMD.hpp"
#include "OBCCU/Sensors.hpp"
#include "OBCCU/Packets.hpp"
#include "OBCCU/StateMachine.hpp"
#include "OBCCU/Protections.hpp"

namespace OBCCU {
    UDP udp;
    TCP tcp;
    Packets packets;

    void inscribe();
	void start();
	void update();

    void inscribe() {
        imd = IMD(PE2, PF0, PF4);
        bms = BMSH(SPI::spi3);
        high_voltage_charger = HalfBridge(PE9, PE8, PE13, PE12, PD4);

        StateMachines::general = StateMachine(States::General::CONNECTING);
        StateMachines::operational = StateMachine(States::Operational::IDLE);
        StateMachines::charging = StateMachine(States::Charging::PRECHARGE);
        StateMachines::contactors_sm = StateMachine(States::Contactors::OPEN);
        StateMachines::imd_sm = StateMachine(States::IMD::OFF);

        Sensors::inscribe();

        Leds::low_charge = DigitalOutput(PG2);
        Leds::full_charge = DigitalOutput(PG3);
        Leds::sleep = DigitalOutput(PG4);
        Leds::flash = DigitalOutput(PG5);
        Leds::can = DigitalOutput(PG6);
        Leds::fault = DigitalOutput(PG7);
        Leds::operational = DigitalOutput(PG8);

        Contactors::high = DigitalOutput(PG12);
        Contactors::low = DigitalOutput(PG14);  
    }

    void start() {
        STLIB::start("192.168.1.9");
        udp.init();
        tcp.init();

        // StateOrder::set_socket(tcp.backend);
        // StateOrder::set_socket(udp.backend);
        
        bms.initialize();
        StateMachines::start();

        int i = 0;
        for (LTC6811& adc : bms.external_adcs) {
            for (Battery& battery: adc.batteries) {
                batteries_data[i] = serialize_battery(battery);
                i++;
            }
        }

        Time::set_timeout(5000, [&](){
            Conditions::ready = true;

            StateMachines::general.refresh_state_orders();
            StateMachines::contactors_sm.refresh_state_orders();
            StateMachines::imd_sm.refresh_state_orders();
        });

        Protections::inscribe();
        ProtectionManager::set_id(Boards::ID::OBCCU);
        ProtectionManager::link_state_machine(StateMachines::general, States::General::FAULT);

        StateMachines::general.check_transitions();
        StateMachines::contactors_sm.check_transitions();
        StateMachines::imd_sm.check_transitions();
	}

    void send_to_backend() {
        for (HeapPacket* packet : packets.battery_packets) {
            udp.send_to_backend(*packet);
        }

        udp.send_to_backend(packets.total_voltage_packet);
        udp.send_to_backend(packets.charging_current_packet);
        udp.send_to_backend(packets.IMD_packet);
    }

    void update() {
        STLIB::update();
        StateMachines::general.check_transitions();
        StateMachines::imd_sm.check_transitions();
        StateMachines::contactors_sm.check_transitions();

        if (Conditions::ready) {
            ProtectionManager::check_protections();
        }
    }
};
