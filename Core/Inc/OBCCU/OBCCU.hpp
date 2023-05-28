#include "BMS-LIB.hpp"
#include "ST-LIB.hpp"

namespace OBCCU {

    enum STATES : uint8_t{
        CONNECTING = 0,
        OPERATIONAL = 1,
        FAULT = 2
    };
    enum OPERATIONAL_STATES : uint8_t {
        CHARGING = 10,
        BALANCING = 11,
        IDLE = 12
    };
    enum CHARGING_STATES : uint8_t {
        PRECHARGE = 100,
        CONSTANT_CURRENT = 101,
        CONSTANT_VOLTAGE = 102
    };

    namespace Packets {
        struct battery_data {
            float* data[10];
        };
        battery_data serialize_battery(Battery& battery) {
            return {
                    battery.cells[0],
                    battery.cells[1],
                    battery.cells[2],
                    battery.cells[3],
                    battery.cells[4],
                    battery.cells[5],
                    battery.minimum_cell,
                    battery.maximum_cell,
                    &battery.SOC,
                    battery.temperature1,
            };
        }

        battery_data batteries_data[10];
    }
    namespace Orders {
        void start_charging() {

        };
        void stop_charging() {

        };
    };
    namespace Conditions {
        bool want_to_charge = false;
        bool ready = false;
        bool fault = false;
    }

    BMSH bms;
    HalfBridge high_voltage_charger;
    StateMachine state_machine;
    StateMachine operational_state_machine;
    StateMachine charging_state_machine;
    uint8_t charging_current_sensor;



    DatagramSocket udp_socket;


    void inscribe();
	void start();
	void update();

    void inscribe() {
        bms = BMSH(SPI::spi3);
        high_voltage_charger = HalfBridge(PE9, PE8, PE13, PE12, PD4);

        state_machine = StateMachine(STATES::CONNECTING);
        operational_state_machine = StateMachine(OPERATIONAL_STATES::IDLE);
        charging_state_machine = StateMachine(CHARGING_STATES::PRECHARGE);

        charging_current_sensor = ADC::inscribe(PA0);
    }

    void start() {
        STLIB::start();
        udp_socket = DatagramSocket( IPV4("192.168.1.8"), 50400, IPV4("192.168.0.9"), 50400);

        int i = 0;
        for (LTC6811& adc: bms.external_adcs) {
            for (Battery& battery: adc.batteries) {
                Packets::batteries_data[i] = Packets::serialize_battery(battery);
                i++;
            }
        }

        StateMachine& sm = state_machine;
        StateMachine& op_sm = operational_state_machine;
        StateMachine& ch_sm = charging_state_machine;


        sm.add_state(STATES::OPERATIONAL);
        sm.add_state(STATES::FAULT);

        sm.add_transition(STATES::CONNECTING, STATES::OPERATIONAL, [&]() {
            return Conditions::ready;
        });

        sm.add_transition(STATES::OPERATIONAL, STATES::FAULT, [&]() {
            return Conditions::fault;
        });

        sm.add_transition(STATES::CONNECTING, STATES::FAULT, [&]() {
            return Conditions::fault;
        });

        sm.add_mid_precision_cyclic_action([&]() {
            bms.update_cell_voltages();
        }, us(5000), STATES::OPERATIONAL);

        sm.add_low_precision_cyclic_action([&]() {
            bms.update_temperatures();
        }, ms(100), STATES::OPERATIONAL);

        op_sm.add_state(OPERATIONAL_STATES::CHARGING);
        op_sm.add_state(OPERATIONAL_STATES::BALANCING);

        op_sm.add_transition(OPERATIONAL_STATES::IDLE, OPERATIONAL_STATES::CHARGING, [&]() {
            return Conditions::want_to_charge;
        });

        op_sm.add_transition(OPERATIONAL_STATES::CHARGING, OPERATIONAL_STATES::IDLE, [&]() {
            return not Conditions::want_to_charge;
        });

        op_sm.add_transition(OPERATIONAL_STATES::CHARGING, OPERATIONAL_STATES::BALANCING, [&]() {
            for (LTC6811& adc: bms.external_adcs) {
                for (Battery& battery: adc.batteries) {
                    if (battery.needs_balance()) {
                        return true;
                    }
                }
            }

            return false;
        });

        sm.add_state_machine(op_sm, STATES::OPERATIONAL);

        ch_sm.add_state(CHARGING_STATES::CONSTANT_CURRENT);
        ch_sm.add_state(CHARGING_STATES::CONSTANT_VOLTAGE);
        
        ch_sm.add_enter_action( [&]() {
            high_voltage_charger.set_phase(100);
        }, CHARGING_STATES::PRECHARGE);

        ch_sm.add_low_precision_cyclic_action([&]() {
            if (ADC::get_value(charging_current_sensor) < 1) {
                Conditions::want_to_charge = false;
            }
        }, ms(100), CHARGING_STATES::CONSTANT_VOLTAGE);

        ch_sm.add_mid_precision_cyclic_action([&]() {
            high_voltage_charger.set_phase(high_voltage_charger.get_phase() - 1);
        }, us(10), CHARGING_STATES::PRECHARGE);

        ch_sm.add_exit_action( [&]() {
            high_voltage_charger.set_phase(0);
        }, CHARGING_STATES::PRECHARGE);

        ch_sm.add_transition(CHARGING_STATES::PRECHARGE, CHARGING_STATES::CONSTANT_CURRENT, [&]() {
            return high_voltage_charger.get_phase() <= 0;
        });

        ch_sm.add_transition(CHARGING_STATES::CONSTANT_CURRENT, CHARGING_STATES::CONSTANT_VOLTAGE, [&]() {
            for (LTC6811& adc: bms.external_adcs) {
                for (Battery& battery: adc.batteries) {
                    if (battery.SOC >= 0.8) {
                        return true;
                    }
                }
            }

            return false;
        });

        ch_sm.add_transition(CHARGING_STATES::CONSTANT_VOLTAGE, CHARGING_STATES::CONSTANT_CURRENT, [&]() {
            for (LTC6811& adc: bms.external_adcs) {
                for (Battery& battery: adc.batteries) {
                    if (battery.SOC <= 0.6) {
                        return true;
                    }
                }
            }

            return false;
        });

        op_sm.add_state_machine(ch_sm, OPERATIONAL_STATES::CHARGING);
        Conditions::ready = true;
        sm.check_transitions();
	}

    void update() {
        STLIB::update();
    }
};
