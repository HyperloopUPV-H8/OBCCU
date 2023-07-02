#pragma once

#include "OBCCU/IMD.hpp"
#include "OBCCU/Measurements.hpp"
#include "OBCCU/Packets.hpp"
#include "OBCCU/Actions.hpp"
#include "OBCCU/Sensors.hpp"

namespace OBCCU {
    
    BMSH bms;
    HalfBridge high_voltage_charger;
    PI<IntegratorType::Trapezoidal> high_voltage_charger_pi(20, 1, 0.001);

    namespace States {
        enum General : uint8_t {
            CONNECTING = 0,
            OPERATIONAL = 1,
            FAULT = 2
        };
        enum Operational : uint8_t {
            CHARGING = 10,
            BALANCING = 11,
            IDLE = 12
        };
        enum Charging : uint8_t {
            PRECHARGE = 100,
            CONSTANT_CURRENT = 101,
            CONSTANT_VOLTAGE = 102
        };

        enum Contactors : uint8_t {
            OPEN = 0,
            CLOSED = 1
        };

        enum IMD : uint8_t {
            OFF = 0,
            ON = 1
        };
    }

    namespace StateMachines {
        StateMachine general;
        StateMachine operational;
        StateMachine charging;

        // Auxiliar State Machines
        StateMachine contactors_sm;
        StateMachine imd_sm;

        void start() {
            StateMachine& sm = general;
            StateMachine& op_sm = operational;
            StateMachine& ch_sm = charging;

            using Gen = States::General;
            using Op = States::Operational;
            using Ch = States::Charging;

            sm.add_state(Gen::OPERATIONAL);
            sm.add_state(Gen::FAULT);

            sm.add_transition(Gen::CONNECTING, Gen::OPERATIONAL, [&]() {
                return Conditions::ready;
            });

            sm.add_transition(Gen::OPERATIONAL, Gen::FAULT, [&]() {
                return Conditions::fault;
            });

            sm.add_transition(Gen::CONNECTING, Gen::FAULT, [&]() {
                return Conditions::fault;
            });

            sm.add_low_precision_cyclic_action([&]() {
                Leds::operational.toggle();
            }, ms(200), Gen::CONNECTING);

            sm.add_enter_action([&]() {
                Leds::operational.turn_on();
            }, Gen::OPERATIONAL);

            sm.add_enter_action([&]() {
                Leds::fault.turn_on();
                Conditions::fault = true;
                OBCCU::Actions::open_contactors();
            }, Gen::FAULT);

            sm.add_exit_action([&]() {
                Leds::operational.turn_off();
            }, Gen::OPERATIONAL);

            sm.add_exit_action([&]() {
                Leds::fault.turn_off();
            }, Gen::FAULT);

            sm.add_state_machine(op_sm, Gen::OPERATIONAL);

            op_sm.add_state(Op::CHARGING);
            op_sm.add_state(Op::BALANCING);

            op_sm.add_transition(Op::IDLE, Op::CHARGING, [&]() {
                return Conditions::want_to_charge;
            });

            op_sm.add_transition(Op::CHARGING, Op::IDLE, [&]() {
                return not Conditions::want_to_charge;
            });

            op_sm.add_transition(Op::CHARGING, Op::BALANCING, [&]() {
                for (LTC6811& adc: bms.external_adcs) {
                    for (Battery& battery: adc.batteries) {
                        if (battery.needs_balance()) {
                            return true;
                        }
                    }
                }

                return false;
            });

            op_sm.add_transition(Op::BALANCING, Op::CHARGING, [&]() {
                for (LTC6811& adc: bms.external_adcs) {
                    for (Battery& battery: adc.batteries) {
                        if (battery.needs_balance()) {
                            return false;
                        }
                    }
                }

                return true;
            });

            op_sm.add_transition(Op::BALANCING, Op::IDLE, [&]() {
                return not Conditions::want_to_charge;
            });

            sm.add_low_precision_cyclic_action([&]() {
                bms.wake_up();
                bms.start_adc_conversion_all_cells();
            }, us(3000), {Gen::OPERATIONAL, Gen::FAULT});

            HAL_Delay(2);

            sm.add_low_precision_cyclic_action([&]() {
                bms.wake_up();
                bms.read_cell_voltages();
            }, us(3000), {Gen::OPERATIONAL, Gen::FAULT});

            sm.add_low_precision_cyclic_action([&]() {
                imd.read();
                Measurements::total_voltage = OBCCU::bms.get_total_voltage();
            }, ms(10), {Gen::OPERATIONAL, Gen::FAULT});

            sm.add_low_precision_cyclic_action([&]() {
                bms.wake_up();
                bms.measure_internal_device_parameters();
            }, ms(10), {Gen::OPERATIONAL, Gen::FAULT});

            HAL_Delay(3);

            sm.add_low_precision_cyclic_action([&]() {
                bms.wake_up();
                bms.read_internal_temperature();
            }, ms(10), {Gen::OPERATIONAL, Gen::FAULT});

            sm.add_low_precision_cyclic_action([&]() {
                bms.wake_up();
                bms.start_adc_conversion_all_cells();
            }, us(3000), {Gen::OPERATIONAL, Gen::FAULT});

            HAL_Delay(2);

            sm.add_low_precision_cyclic_action([&]() {
                bms.wake_up();
                bms.read_temperatures();
            }, us(3000), {Gen::OPERATIONAL, Gen::FAULT});

            sm.add_low_precision_cyclic_action([&]() {
                for (LTC6811& adc: bms.external_adcs) {
                    for (Battery& battery: adc.batteries) {
                        battery.update_data();
                    }
                }

            }, ms(1000), {Gen::OPERATIONAL, Gen::FAULT});

            sm.add_low_precision_cyclic_action([&]() {
                Sensors::charging_current.read();
                Measurements::average_current += -Measurements::average_current*0.01 + Measurements::charging_current*0.01;
            }, us(3000), {Gen::OPERATIONAL, Gen::FAULT});

            op_sm.add_state_machine(ch_sm, Op::CHARGING);

            ch_sm.add_state(Ch::CONSTANT_CURRENT);
            ch_sm.add_state(Ch::CONSTANT_VOLTAGE);
            
            ch_sm.add_enter_action( [&]() {
                high_voltage_charger.set_phase(100);
            }, Ch::PRECHARGE);

            // ch_sm.add_low_precision_cyclic_action([&]() {
            //     if (Measurements::charging_current < 1) {
            //         Conditions::want_to_charge = false;
            //     }
            // }, ms(100), Ch::CONSTANT_VOLTAGE);



            // ch_sm.add_low_precision_cyclic_action([&]() {
            //     high_voltage_charger.set_phase(high_voltage_charger.get_phase() - 1);
            // }, ms(1), Ch::PRECHARGE);

            ch_sm.add_exit_action( [&]() {
                high_voltage_charger.set_phase(0);
            }, Ch::PRECHARGE);

            ch_sm.add_transition(Ch::PRECHARGE, Ch::CONSTANT_CURRENT, [&]() {
                return high_voltage_charger.get_phase() <= 0;
            });

            ch_sm.add_transition(Ch::CONSTANT_CURRENT, Ch::CONSTANT_VOLTAGE, [&]() {
                for (LTC6811& adc: bms.external_adcs) {
                    for (Battery& battery: adc.batteries) {
                        if (battery.SOC >= 0.8) {
                            return true;
                        }
                    }
                }

                return false;
            });

            ch_sm.add_transition(Ch::CONSTANT_VOLTAGE, Ch::CONSTANT_CURRENT, [&]() {
                for (LTC6811& adc: bms.external_adcs) {
                    for (Battery& battery: adc.batteries) {
                        if (battery.SOC <= 0.6) {
                            return true;
                        }
                    }
                }

                return false;
            });

            contactors_sm.add_state(States::Contactors::CLOSED);
            contactors_sm.add_transition(States::Contactors::OPEN, States::Contactors::CLOSED, [&]() {
                return Conditions::contactors_closed;
            });

            contactors_sm.add_transition(States::Contactors::CLOSED, States::Contactors::OPEN, [&]() {
                return not Conditions::contactors_closed;
            });

            imd_sm.add_state(States::IMD::ON);

            imd_sm.add_transition(States::IMD::OFF, States::IMD::ON, [&]() {
                return imd.is_on;
            });

            imd_sm.add_transition(States::IMD::ON, States::IMD::OFF, [&]() {
                return not imd.is_on;
            });
        }
    }
} 
