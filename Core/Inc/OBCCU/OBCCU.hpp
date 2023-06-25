#include "BMS-LIB.hpp"
#include "ST-LIB.hpp"
#include "OBCCU/Orders.hpp"
#include "OBCCU/IMD.hpp"
#include "OBCCU/Sensors.hpp"
#include "OBCCU/Packets.hpp"
#include "OBCCU/StateMachine.hpp"
#include "OBCCU/Protections.hpp"

namespace OBCCU {
    IncomingOrders incoming_orders;
    UDP udp;
    TCP tcp;
    Packets packets;

    void inscribe();
	void start();
	void update();

    void inscribe() {
        bms = BMSH(SPI::spi3);
        high_voltage_charger = HalfBridge(PE9, PE8, PE13, PE12, PD4);

        imd = IMD(PE2, PF0, PF4);

        StateMachines::general = StateMachine(States::General::CONNECTING);
        StateMachines::operational = StateMachine(States::Operational::IDLE);
        StateMachines::charging = StateMachine(States::Charging::PRECHARGE);

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
        
        bms.initialize();
        StateMachines::start();

        ProtectionManager::set_id(Boards::ID::OBCCU);
        ProtectionManager::link_state_machine(StateMachines::general, States::General::FAULT);

        int i = 0;
        for (LTC6811& adc : bms.external_adcs) {
            for (Battery& battery: adc.batteries) {
                batteries_data[i] = serialize_battery(battery);
                i++;
            }
        }

        Time::set_timeout(1000, [&](){
            Conditions::ready = true;
        });
        StateMachines::general.check_transitions();
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

        if (Conditions::ready) {
            ProtectionManager::check_protections();
        }
    }
};
