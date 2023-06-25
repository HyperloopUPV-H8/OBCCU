#include "BMS-LIB.hpp"
#include "OBCCU/Measurements.hpp"

namespace OBCCU {
    namespace Sensors {
        LinearSensor<double> charging_current;
        LinearSensor<double> inverter_temperature;
        LinearSensor<double> capacitor_temperature;
        LinearSensor<double> transformer_temperature;
        LinearSensor<double> rectifier_temperature;
        SensorInterrupt pwm_sensor;
        DigitalSensor ok_sensor;
        
        void inscribe() {
            Sensors::charging_current = LinearSensor<double>(PA0, 366.2, -523, Measurements::charging_current);
            Sensors::inverter_temperature = LinearSensor<double>(PA3, 1, 0, Measurements::inverter_temperature);
            Sensors::capacitor_temperature = LinearSensor<double>(PA4, 1, 0, Measurements::capacitor_temperature);
            Sensors::transformer_temperature = LinearSensor<double>(PA5, 1, 0, Measurements::transformer_temperature);
            Sensors::rectifier_temperature = LinearSensor<double>(PA6, 1, 0, Measurements::rectifier_temperature);
        }
    };


}