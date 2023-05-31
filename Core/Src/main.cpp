#include "main.h"
#include "lwip.h"

#include "ST-LIB.hpp"
#include "Runes/Runes.hpp"
#include "OBCCU/OBCCU.hpp"


int main(void)
{
	OBCCU::inscribe();
	OBCCU::start();

	HeapOrder start_charging_order = {
		900,
		&OBCCU::Orders::start_charging
	};

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
		&battery10_packet
	};

	DatagramSocket test_socket(IPV4("192.168.1.9"), 50400, IPV4("192.168.0.9"), 50400);
	while(1) {
		// for (HeapPacket* packet : battery_packets) {
		// 	test_socket.send(*packet);
		// }

		test_socket.send(battery1_packet);

		OBCCU::update();
	}
}

void Error_Handler(void)
{
	ErrorHandler("HAL error handler triggered");
	while (1){}
}
