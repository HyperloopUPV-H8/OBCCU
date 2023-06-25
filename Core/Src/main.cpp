#include "main.h"
#include "lwip.h"

#include "ST-LIB.hpp"
#include "Runes/Runes.hpp"
#include "OBCCU/OBCCU.hpp"

int main(void) {
	if (BMS::EXTERNAL_ADCS != 5) {
		ErrorHandler("BMS::EXTERNAL_ADCS must be 5");
	}

	add_protection(&BMS::EXTERNAL_ADCS, Boundary<const int, NOT_EQUALS>(5));

	OBCCU::inscribe();
	OBCCU::start();

	ServerSocket tcp_socket(IPV4("192.168.1.9"), 50500);
	DatagramSocket test_socket(IPV4("192.168.1.9"), 50400, IPV4("192.168.0.9"), 50400);

	OBCCU::Orders::turn_on_IMD();
	Time::set_timeout(5000, [&](){
		OBCCU::Conditions::first_read = true;
	});

	Time::register_low_precision_alarm(15, [&](){
		OBCCU::total_voltage = OBCCU::bms.get_total_voltage();

		if (OBCCU::imd.duty_cycle > 80) {
			OBCCU::drift = true;
		} else {
			OBCCU::drift = false;
		}

		for (HeapPacket* packet : OBCCU::packets.battery_packets) {
			test_socket.send(*packet);
		}

		test_socket.send(OBCCU::packets.total_voltage_packet);
		test_socket.send(OBCCU::packets.charging_current_packet);
		test_socket.send(OBCCU::packets.IMD_packet);
	});

	while(1) {


  		OBCCU::update();
	}
}

void Error_Handler(void)
{
	ErrorHandler("HAL error handler triggered");
	while (1){}
}
