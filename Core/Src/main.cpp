#include "main.h"
#include "lwip.h"

#include "ST-LIB.hpp"
#include "Runes/Runes.hpp"
#include "OBCCU/OBCCU.hpp"

bool drift = false;
int main(void) {
	if (BMS::EXTERNAL_ADCS != 5) {
		ErrorHandler("BMS::EXTERNAL_ADCS must be 5");
	}

	add_protection(&BMS::EXTERNAL_ADCS, Boundary<const int, NOT_EQUALS>(5));

	int i = 0;
	for (LTC6811& adc : OBCCU::bms.external_adcs) {
		if (i == 4) {
			break;
		}
		for (Battery& battery : adc.batteries) {

			// add_protection(&battery.disbalance, Boundary<float, ProtectionType::ABOVE>(Battery::MAX_DISBALANCE));
			add_protection(&battery.total_voltage, Boundary<float, ProtectionType::BELOW>(19.8));
			add_protection(&battery.total_voltage, Boundary<float, ProtectionType::ABOVE>(26));
			// for (float* cell: battery.cells) {
			// 	add_protection(cell, Boundary<float, ProtectionType::BELOW>(3.3));
			// 	add_protection(cell, Boundary<float, ProtectionType::ABOVE>(4.2));
			// }

			// add_protection(battery.temperature1, Boundary<float, ProtectionType::ABOVE>(70));
			// add_protection(battery.temperature2, Boundary<float, ProtectionType::ABOVE>(70));
		}
		i++;
	}

	add_protection(&OBCCU::Measurements::IMD_duty_cycle, Boundary<float, ProtectionType::ABOVE>(50));

	OBCCU::inscribe();
	OBCCU::start();

	ServerSocket tcp_socket(IPV4("192.168.1.9"), 50500);
	StateOrder::set_socket(tcp_socket);

	HeapOrder start_charging_order = {
		900,
		&OBCCU::Orders::start_charging,
	};

	// HeapStateOrder start_charging_order = {
	// 	900,
	// 	&OBCCU::Orders::start_charging,
	// 	OBCCU::StateMachines::general,
	// 	OBCCU::States::OPERATIONAL
	// };

	HeapOrder stop_charging_order = {
		901,
		&OBCCU::Orders::stop_charging
	};

	HeapOrder open_contactors_order = {
		902,
		&OBCCU::Orders::open_contactors
	};

	HeapOrder close_contactors_order = {
		903,
		&OBCCU::Orders::close_contactors
	};

	HeapOrder turn_on_IMD = {
		904,
		&OBCCU::Orders::turn_on_IMD
	};

	HeapOrder turn_off_IMD = {
		905,
		&OBCCU::Orders::turn_off_IMD
	};

	HeapPacket battery1_packet = {
		910,
		&OBCCU::Packets::batteries_data[0].data
	};

	HeapPacket battery2_packet = {
		911,
		&OBCCU::Packets::batteries_data[1].data
	};

	HeapPacket battery3_packet = {
		912,
		&OBCCU::Packets::batteries_data[2].data
	};
	
	HeapPacket battery4_packet = {
		913,
		&OBCCU::Packets::batteries_data[3].data
	};

	HeapPacket battery5_packet = {
		914,
		&OBCCU::Packets::batteries_data[4].data
	};

	HeapPacket battery6_packet = {
		915,
		&OBCCU::Packets::batteries_data[5].data
	};

	HeapPacket battery7_packet = {
		916,
		&OBCCU::Packets::batteries_data[6].data
	};

	HeapPacket battery8_packet = {
		917,
		&OBCCU::Packets::batteries_data[7].data
	};

	HeapPacket battery9_packet = {
		918,
		&OBCCU::Packets::batteries_data[8].data
	};

	HeapPacket battery10_packet = {
		919,
		&OBCCU::Packets::batteries_data[9].data
	};

	HeapPacket charging_current_packet = {
		920,
		&OBCCU::Measurements::average_current,
		&OBCCU::Measurements::capacitor_temperature,
		&OBCCU::Measurements::inverter_temperature,
		&OBCCU::Measurements::rectifier_temperature,
		&OBCCU::Measurements::transformer_temperature
	};

	HeapPacket total_voltage_packet = {
		922,
		&OBCCU::total_voltage
	};

	array<HeapPacket*, 10> battery_packets = {
		&battery1_packet,
		&battery2_packet,
		&battery3_packet,
		&battery4_packet,
		&battery5_packet,
		&battery6_packet,
		&battery7_packet,
		&battery8_packet,
		&battery9_packet,
		&battery10_packet,
	};

	HeapPacket IMD_packet = {
		921,
		&OBCCU::Measurements::IMD_duty_cycle,
		&OBCCU::Measurements::IMD_frequency,
		&OBCCU::Measurements::IMD_OK,
		&drift
	};

	DatagramSocket test_socket(IPV4("192.168.1.9"), 50400, IPV4("192.168.0.9"), 50400);

	OBCCU::Orders::turn_on_IMD();
	Time::set_timeout(5000, [&](){
		OBCCU::Conditions::first_read = true;
	});

	while(1) {
		OBCCU::total_voltage = 0;
		for (LTC6811& adc: OBCCU::bms.external_adcs) {
			for (Battery& battery: adc.batteries) {
					OBCCU::total_voltage += battery.total_voltage;
			}
		}

		if (OBCCU::Measurements::IMD_duty_cycle > 80) {
			drift = true;
		} else {
			drift = false;
		}

		for (HeapPacket* packet : battery_packets) {
			test_socket.send(*packet);
		}

		test_socket.send(total_voltage_packet);
		test_socket.send(charging_current_packet);
		test_socket.send(IMD_packet);

  		OBCCU::update();

	}
}

void Error_Handler(void)
{
	ErrorHandler("HAL error handler triggered");
	while (1){}
}
