#include "main.h"
#include "lwip.h"

#include "ST-LIB.hpp"
#include "Runes/Runes.hpp"
#include "OBCCU/OBCCU.hpp"

int main(void) {
	static_assert(BMS::EXTERNAL_ADCS == 5, "BMS::EXTERNAL_ADCS must be 5");

	/*#ifndef BOARD
		static_assert(false, "Board code can not be run in Nucleo mode");
	#endif*/

	#ifdef HSE_VALUE
		static_assert(HSE_VALUE == 25000000, "HSE_VALUE must be 25000000");
	#endif
	
	OBCCU::inscribe();
	OBCCU::start();

	StateOrder::set_socket(OBCCU::tcp.backend);
	OBCCU::IncomingOrders incoming_orders;

	OBCCU::Actions::turn_on_IMD();

	Time::register_low_precision_alarm(15, [&](){
		OBCCU::send_to_backend();
		OBCCU::update();
	});

	while(1) {
	}
}

void Error_Handler(void)
{
	ErrorHandler("HAL error handler triggered");
	while (1){}
}
