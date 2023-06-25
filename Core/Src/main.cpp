#include "main.h"
#include "lwip.h"

#include "ST-LIB.hpp"
#include "Runes/Runes.hpp"
#include "OBCCU/OBCCU.hpp"

int main(void) {
	static_assert(BMS::EXTERNAL_ADCS == 5, "BMS::EXTERNAL_ADCS must be 5");

	OBCCU::inscribe();
	OBCCU::start();
	
	OBCCU::Orders::turn_on_IMD();

	Time::register_low_precision_alarm(15, [&](){
		OBCCU::send_to_backend();
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
