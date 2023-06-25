#include "BMS-LIB.hpp"

namespace OBCCU {
    namespace Protections {
        void inscribe() {
            // int nbat = 0;
            // for (LTC6811& adc : OBCCU::bms.external_adcs) {
            // 	for (Battery& battery : adc.batteries) {
            // 		if (nbat == 3) {
            // 			break;
            // 		}

            // 		add_protection(&battery.disbalance, Boundary<float, ProtectionType::ABOVE>(Battery::MAX_DISBALANCE));
            // 		add_protection(&battery.total_voltage, Boundary<float, ProtectionType::BELOW>(19.8));
            // 		add_protection(&battery.total_voltage, Boundary<float, ProtectionType::ABOVE>(26));
            // 		for (float* cell: battery.cells) {
            // 			add_protection(cell, Boundary<float, ProtectionType::BELOW>(3.3));
            // 			add_protection(cell, Boundary<float, ProtectionType::ABOVE>(4.2));
            // 		}

            // 		add_protection(battery.temperature1, Boundary<float, ProtectionType::ABOVE>(70));
            // 		add_protection(battery.temperature2, Boundary<float, ProtectionType::ABOVE>(70));
            // 		nbat++;
            // 	}
            // }

            // add_protection(&OBCCU::imd.duty_cycle, Boundary<double, ProtectionType::ABOVE>(50));

        }
    }
}