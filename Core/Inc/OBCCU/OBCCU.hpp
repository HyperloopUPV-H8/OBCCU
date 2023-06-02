#include "BMS-LIB.hpp"
#include "ST-LIB.hpp"

namespace OBCCU {
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
    }
    namespace Conditions {
        bool ready = false;
        bool want_to_charge = false;
        bool fault = false;
        bool contactors_closed = false;
    }
    namespace Packets {
        struct battery_data {
            float* data[11];
        };
        
        battery_data serialize_battery(Battery& battery) {
            return {
                    &battery.SOC,
                    battery.cells[0],
                    battery.cells[1],
                    battery.cells[2],
                    battery.cells[3],
                    battery.cells[4],
                    battery.cells[5],
                    &battery.minimum_cell,
                    &battery.maximum_cell,
                    battery.temperature1,
                    (float*)&battery.is_balancing
            };
        }

        battery_data batteries_data[10];
    }

    namespace Contactors {
        DigitalOutput high;
        DigitalOutput low;
    }

    namespace Orders {
        void start_charging() {
            Conditions::want_to_charge = true;
        };

        void stop_charging() {
            Conditions::want_to_charge = false;
        };

        void open_contactors() {
            Contactors::high.turn_off();
            Contactors::low.turn_off();
            Conditions::contactors_closed = false;
        };

        void close_contactors() {
            Contactors::low.turn_on();
            Contactors::high.turn_on();
            Conditions::contactors_closed = true;
        };
    };

    namespace Sensors {
        LinearSensor charging_current;
        uint8_t inverter_temperature;
        uint8_t capacitor_temperature;
        uint8_t transformer_temperature;
        uint8_t rectifier_temperature;
    };
    namespace Leds {
        DigitalOutput low_charge;
        DigitalOutput full_charge;
        DigitalOutput sleep;
        DigitalOutput flash;
        DigitalOutput can;
        DigitalOutput fault;
        DigitalOutput operational;
    };
    namespace Communications {
        DatagramSocket udp_socket;
        uint8_t i2c;
    };
    namespace Measurements {
        double charging_current;
    };

    BMSH bms;
    HalfBridge high_voltage_charger;
    PI<IntegratorType::Trapezoidal> high_voltage_charger_pi(20, 1, 0.001);

    namespace StateMachines {
        StateMachine general;
        StateMachine operational;
        StateMachine charging;

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

            // sm.add_low_precision_cyclic_action([&]() {
            //     bms.wake_up();
            //     bms.start_adc_conversion_temperatures();
            // }, ms(200), Gen::OPERATIONAL);

            // HAL_Delay(10);

            // sm.add_low_precision_cyclic_action([&]() {
            //     bms.wake_up();
            //     bms.read_temperatures();
            // }, ms(200), Gen::OPERATIONAL);

            sm.add_enter_action([&]() {
                Leds::fault.turn_on();
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

            op_sm.add_mid_precision_cyclic_action([&]() {
                bms.wake_up();
                bms.start_adc_conversion_all_cells();
            }, us(3000), Op::IDLE);

            HAL_Delay(2);

            op_sm.add_mid_precision_cyclic_action([&]() {
                bms.wake_up();
                bms.read_cell_voltages();
            }, us(3000), Op::IDLE);

            op_sm.add_low_precision_cyclic_action([&]() {
                bms.wake_up();
                bms.measure_internal_device_parameters();
            }, ms(1000), Op::IDLE);

            HAL_Delay(10);

            op_sm.add_low_precision_cyclic_action([&]() {
                bms.wake_up();
                bms.read_internal_temperature();
            }, ms(1000), Op::IDLE);

            op_sm.add_low_precision_cyclic_action([&]() {
                for (LTC6811& adc: bms.external_adcs) {
                    for (Battery& battery: adc.batteries) {
                        battery.update_data();
                    }
                }
            }, ms(1000), Op::IDLE);

            op_sm.add_state_machine(ch_sm, Op::CHARGING);

            ch_sm.add_state(Ch::CONSTANT_CURRENT);
            ch_sm.add_state(Ch::CONSTANT_VOLTAGE);
            
            ch_sm.add_enter_action( [&]() {
                high_voltage_charger.set_phase(100);
            }, Ch::PRECHARGE);

            ch_sm.add_low_precision_cyclic_action([&]() {
                if (Measurements::charging_current < 1) {
                    Conditions::want_to_charge = false;
                }
            }, ms(100), Ch::CONSTANT_VOLTAGE);



            ch_sm.add_mid_precision_cyclic_action([&]() {
                high_voltage_charger.set_phase(high_voltage_charger.get_phase() - 1);
            }, us(50), Ch::PRECHARGE);

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
        }
    }



    DigitalOutput IMD_Power;


    void inscribe();
	void start();
	void update();

    void inscribe() {
        bms = BMSH(SPI::spi3);
        high_voltage_charger = HalfBridge(PE9, PE8, PE13, PE12, PD4);

        IMD_Power = DigitalOutput(PE2);

        StateMachines::general = StateMachine(States::General::CONNECTING);
        StateMachines::operational = StateMachine(States::Operational::IDLE);
        StateMachines::charging = StateMachine(States::Charging::PRECHARGE);

        Sensors::charging_current = LinearSensor(PA0, 1, 1, Measurements::charging_current);
        Sensors::inverter_temperature = ADC::inscribe(PA3);
        Sensors::capacitor_temperature = ADC::inscribe(PA4);
        Sensors::transformer_temperature = ADC::inscribe(PA5);
        Sensors::rectifier_temperature = ADC::inscribe(PA6);

        Leds::low_charge = DigitalOutput(PG2);
        Leds::full_charge = DigitalOutput(PG3);
        Leds::sleep = DigitalOutput(PG4);
        Leds::flash = DigitalOutput(PG5);
        Leds::can = DigitalOutput(PG6);
        Leds::fault = DigitalOutput(PG7);
        Leds::operational = DigitalOutput(PG8);

        Contactors::high = DigitalOutput(PG12);
        Contactors::low = DigitalOutput(PG14);
    }

    void start() {
        STLIB::start();
        Communications::udp_socket = DatagramSocket( IPV4("192.168.1.9"), 50400, IPV4("192.168.0.9"), 50400);

        bms.initialize();
        StateMachines::start();

        int i = 0;
        for (LTC6811& adc: bms.external_adcs) {
            for (Battery& battery: adc.batteries) {
                Packets::batteries_data[i] = Packets::serialize_battery(battery);
                i++;
            }
        }

        Conditions::ready = true;
        StateMachines::general.check_transitions();
	}

    void update() {
        STLIB::update();
        StateMachines::general.check_transitions();
    }
};
