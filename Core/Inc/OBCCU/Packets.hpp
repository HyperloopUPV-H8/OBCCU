#pragma once

#include "BMS-LIB.hpp"
#include "OBCCU/Measurements.hpp"

namespace OBCCU {
    float total_voltage;

    struct battery_data {
        float* data[13];
    };
    
    battery_data serialize_battery(Battery& battery) {
        return {
                &battery.SOC,
                battery.cells[0],
                battery.cells[1],
                battery.cells[2],
                battery.cells[3],
                battery.cells[4],
                battery.cells[5],
                &battery.minimum_cell,
                &battery.maximum_cell,
                battery.temperature1,
                battery.temperature2,
                (float*)&battery.is_balancing,
                &battery.total_voltage
        };
    }

    battery_data batteries_data[10];

    class Packets {
    public:
        HeapPacket battery1_packet;
        HeapPacket battery2_packet;
        HeapPacket battery3_packet;
        HeapPacket battery4_packet;
        HeapPacket battery5_packet;
        HeapPacket battery6_packet;
        HeapPacket battery7_packet;
        HeapPacket battery8_packet;
        HeapPacket battery9_packet;
        HeapPacket battery10_packet;
        HeapPacket charging_current_packet;
        HeapPacket total_voltage_packet;
        array<HeapPacket*, 10> battery_packets;
        HeapPacket IMD_packet;

        Packets() :
        battery1_packet(910, &batteries_data[0].data),
        battery2_packet(911, &batteries_data[1].data),
        battery3_packet(912, &batteries_data[2].data),
        battery4_packet(913, &batteries_data[3].data),
        battery5_packet(914, &batteries_data[4].data),
        battery6_packet(915, &batteries_data[5].data),
        battery7_packet(916, &batteries_data[6].data),
        battery8_packet(917, &batteries_data[7].data),
        battery9_packet(918, &batteries_data[8].data),
        battery10_packet(919, &batteries_data[9].data),
        charging_current_packet(920, &Measurements::charging_current),
        total_voltage_packet(921, &OBCCU::total_voltage),
        IMD_packet(922, &OBCCU::imd.duty_cycle, &OBCCU::imd.frequency, &OBCCU::imd.drift)
        {
            battery_packets = {&battery1_packet, &battery2_packet, &battery3_packet, &battery4_packet, &battery5_packet, &battery6_packet, &battery7_packet, &battery8_packet, &battery9_packet, &battery10_packet};
        }
    };

    class UDP {
    public:
        DatagramSocket backend;
        UDP() {}

        void init() {
            backend = DatagramSocket(IPV4("192.168.1.9"), 50400, IPV4("192.168.0.9"), 50400);
            backend.reconnect();
        }

        void send_to_backend(Packet& packet) {
            backend.send(packet);
        }
    };
}