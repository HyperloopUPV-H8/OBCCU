#pragma once

#include "BMS-LIB.hpp"

namespace OBCCU {
    
    class IMD {
    public:
        DigitalOutput power;
        PWMSensor<double> pwm_sensor;
        DigitalSensor ok_sensor;
        
        double frequency;
        double duty_cycle;
        PinState OK;


        IMD() = default;
        IMD(Pin& power_pin, Pin& pwm_pin, Pin& ok_pin) {
            this->power = DigitalOutput(power_pin);
            this->pwm_sensor = PWMSensor(pwm_pin, &this->frequency, &this->duty_cycle);
            this->ok_sensor = DigitalSensor(ok_pin, this->OK);
            this->frequency = 0;
            this->duty_cycle = 0;
            this->OK = PinState::OFF;
        }

        void read() {
            pwm_sensor.read();
            ok_sensor.read();
        }
    };

    IMD imd;
}