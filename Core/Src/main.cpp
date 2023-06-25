#include "main.h"
#include "lwip.h"

#include "ST-LIB.hpp"
#include "Runes/Runes.hpp"
#include "OBCCU/OBCCU.hpp"

int main(void) {
	static_assert(BMS::EXTERNAL_ADCS == 5, "BMS::EXTERNAL_ADCS must be 5");

	OBCCU::inscribe();
	OBCCU::start();

	ServerSocket tcp_socket(IPV4("192.168.1.9"), 50500);

	OBCCU::Orders::turn_on_IMD();

	Time::register_low_precision_alarm(15, [&](){
		OBCCU::total_voltage = OBCCU::bms.get_total_voltage();

		for (HeapPacket* packet : OBCCU::packets.battery_packets) {
			OBCCU::udp.send_to_backend(*packet);
		}

		OBCCU::udp.send_to_backend(OBCCU::packets.total_voltage_packet);
		OBCCU::udp.send_to_backend(OBCCU::packets.charging_current_packet);
		OBCCU::udp.send_to_backend(OBCCU::packets.IMD_packet);
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
