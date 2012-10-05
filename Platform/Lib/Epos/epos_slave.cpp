/*
 * dsp402_slave : CANopen communication interface for epos motor controllers 
 * author : Mike Hopkins
 */

#include "co_types.h"
#include "epos_slave.h"

/* defines standard epos object dictionary entries */
static co_dictionary_entry epos_slave_dictionary[] = {
  {EPOS_NODE_ID, 0x00, CO_UNSIGNED8},
  {EPOS_CAN_BITRATE, 0x00, CO_UNSIGNED16},
  {EPOS_RS232_BAUDRATE, 0x00, CO_UNSIGNED16},
  {EPOS_RS232_FRAME_TIMEOUT, 0x00, CO_UNSIGNED16},
  {EPOS_USB_FRAME_TIMEOUT, 0x00, CO_UNSIGNED16},
  {EPOS_MISCELLANEOUS_CONFIGURATION, 0x00, CO_UNSIGNED16},
  {EPOS_CAN_BITRATE_DISPLAY, 0x00, CO_UNSIGNED16},
  {EPOS_INCREMENTAL_ENCODER_1_COUNTER, 0x00, CO_UNSIGNED32},
  {EPOS_INCREMENTAL_ENCODER_1_COUNTER_AT_INDEX_PULSE, 0x00, CO_UNSIGNED32},
  {EPOS_HALL_SENSOR_PATTERN, 0x00, CO_UNSIGNED32},
  {EPOS_CURRENT_ACTUAL_VALUE_AVERAGED, 0x00, CO_INTEGER16},
  {EPOS_VELOCITY_ACTUAL_VALUE_AVERAGED, 0x00, CO_INTEGER32},
  {EPOS_AUXILIARY_VELOCITY_ACTUAL_VALUE_AVERAGED, 0x00, CO_INTEGER32},
  {EPOS_CURRENT_MODE_SETTING_VALUE, 0x00, CO_INTEGER16},
  {EPOS_CURRENT_DEMAND_VALUE, 0x00, CO_INTEGER16},
  {EPOS_POSITION_MODE_SETTING_VALUE, 0x00, CO_INTEGER32},
  {EPOS_AUXILIARY_VELOCITY_SENSOR_ACTUAL_VALUE, 0x00, CO_INTEGER32},
  {EPOS_VELOCITY_MODE_SETTING_VALUE, 0x00, CO_INTEGER32},
  {EPOS_AUXILIARY_VELOCITY_ACTUAL_VALUE, 0x00, CO_INTEGER32},
  {EPOS_CONFIGURATION_OF_DIGITAL_INPUTS, 0x01, CO_UNSIGNED16},
  {EPOS_CONFIGURATION_OF_DIGITAL_INPUTS, 0x02, CO_UNSIGNED16},
  {EPOS_CONFIGURATION_OF_DIGITAL_INPUTS, 0x03, CO_UNSIGNED16},
  {EPOS_CONFIGURATION_OF_DIGITAL_INPUTS, 0x04, CO_UNSIGNED16},
  {EPOS_CONFIGURATION_OF_DIGITAL_INPUTS, 0x05, CO_UNSIGNED16},
  {EPOS_CONFIGURATION_OF_DIGITAL_INPUTS, 0x06, CO_UNSIGNED16},
  {EPOS_CONFIGURATION_OF_DIGITAL_INPUTS, 0x07, CO_UNSIGNED16},
  {EPOS_CONFIGURATION_OF_DIGITAL_INPUTS, 0x08, CO_UNSIGNED16},
  {EPOS_CONFIGURATION_OF_DIGITAL_INPUTS, 0x09, CO_UNSIGNED16},
  {EPOS_CONFIGURATION_OF_DIGITAL_INPUTS, 0x0A, CO_UNSIGNED16},
  {EPOS_DIGITAL_INPUT_FUNCTIONALITIES, 0x01, CO_UNSIGNED16},
  {EPOS_DIGITAL_INPUT_FUNCTIONALITIES, 0x02, CO_UNSIGNED16},
  {EPOS_DIGITAL_INPUT_FUNCTIONALITIES, 0x03, CO_UNSIGNED16},
  {EPOS_DIGITAL_INPUT_FUNCTIONALITIES, 0x04, CO_UNSIGNED16},
  {EPOS_DIGITAL_OUTPUT_FUNCTIONALITIES, 0x01, CO_UNSIGNED16},
  {EPOS_DIGITAL_OUTPUT_FUNCTIONALITIES, 0x02, CO_UNSIGNED16},
  {EPOS_DIGITAL_OUTPUT_FUNCTIONALITIES, 0x03, CO_UNSIGNED16},
  {EPOS_CONFIGURATION_OF_DIGITAL_OUTPUTS, 0x01, CO_UNSIGNED16},
  {EPOS_CONFIGURATION_OF_DIGITAL_OUTPUTS, 0x02, CO_UNSIGNED16},
  {EPOS_CONFIGURATION_OF_DIGITAL_OUTPUTS, 0x03, CO_UNSIGNED16},
  {EPOS_CONFIGURATION_OF_DIGITAL_OUTPUTS, 0x04, CO_UNSIGNED16},
  {EPOS_CONFIGURATION_OF_DIGITAL_OUTPUTS, 0x05, CO_UNSIGNED16},
  {EPOS_CONFIGURATION_OF_ANALOG_INPUTS, 0x01, CO_UNSIGNED16},
  {EPOS_CONFIGURATION_OF_ANALOG_INPUTS, 0x02, CO_UNSIGNED16},
  {EPOS_ANALOG_INPUTS, 0x01, CO_INTEGER16},
  {EPOS_ANALOG_INPUTS, 0x02, CO_INTEGER16},
  {EPOS_ANALOG_INPUT_FUNCTIONALITIES_EXECUTION_MASK, 0x00, CO_UNSIGNED16},
  {EPOS_ANALOG_OUTPUT_1, 0x00, CO_UNSIGNED16},
  {EPOS_CURRENT_THRESHOLD_FOR_HOMING_MODE, 0x00, CO_UNSIGNED16},
  {EPOS_HOME_POSITION, 0x00, CO_INTEGER32},
  {EPOS_FOLLOWING_ERROR_ACTUAL_VALUE, 0x00, CO_INTEGER16},
  {EPOS_HOLDING_BRAKE_CONFIGURATION, 0x01, CO_UNSIGNED16},
  {EPOS_HOLDING_BRAKE_CONFIGURATION, 0x02, CO_UNSIGNED16},
  {EPOS_HOLDING_BRAKE_CONFIGURATION, 0x03, CO_UNSIGNED16},
  {EPOS_STANDSTILL_WINDOW_CONFIGURATION, 0x01, CO_UNSIGNED16},
  {EPOS_STANDSTILL_WINDOW_CONFIGURATION, 0x02, CO_UNSIGNED16},
  {EPOS_STANDSTILL_WINDOW_CONFIGURATION, 0x03, CO_UNSIGNED16},
  {EPOS_SENSOR_CONFIGURATION, 0x01, CO_UNSIGNED32},
  {EPOS_SENSOR_CONFIGURATION, 0x02, CO_UNSIGNED16},
  {EPOS_SENSOR_CONFIGURATION, 0x04, CO_UNSIGNED16},
  {EPOS_SSI_ENCODER_CONFIGURATION, 0x01, CO_UNSIGNED16},
  {EPOS_SSI_ENCODER_CONFIGURATION, 0x02, CO_UNSIGNED16},
  {EPOS_SSI_ENCODER_CONFIGURATION, 0x03, CO_INTEGER32},
  {EPOS_SSI_ENCODER_CONFIGURATION, 0x04, CO_UNSIGNED16},
  {EPOS_INCREMENTAL_ENCODER_2_CONFIGURATION, 0x01, CO_UNSIGNED32},
  {EPOS_INCREMENTAL_ENCODER_2_CONFIGURATION, 0x02, CO_UNSIGNED32},
  {EPOS_INCREMENTAL_ENCODER_2_CONFIGURATION, 0x03, CO_UNSIGNED32},
  {EPOS_SINUS_INCREMENTAL_ENCODER_2_CONFIGURATION, 0x01, CO_UNSIGNED32},
  {EPOS_SINUS_INCREMENTAL_ENCODER_2_CONFIGURATION, 0x02, CO_INTEGER32},
  {EPOS_CONTROLLER_STRUCTURE, 0x00, CO_UNSIGNED16},
  {EPOS_GEAR_CONFIGURATION, 0x01, CO_UNSIGNED32},
  {EPOS_GEAR_CONFIGURATION, 0x02, CO_UNSIGNED16},
  {EPOS_GEAR_CONFIGURATION, 0x03, CO_UNSIGNED32},
  {EPOS_DIGITAL_POSITION_INPUT, 0x01, CO_INTEGER32},
  {EPOS_DIGITAL_POSITION_INPUT, 0x02, CO_UNSIGNED16},
  {EPOS_DIGITAL_POSITION_INPUT, 0x03, CO_UNSIGNED16},
  {EPOS_DIGITAL_POSITION_INPUT, 0x04, CO_UNSIGNED8},
  {EPOS_DIGITAL_POSITION_INPUT, 0x05, CO_INTEGER32},
  {EPOS_ANALOG_CURRENT_SETPOINT_CONFIGURATION, 0x01, CO_INTEGER16},
  {EPOS_ANALOG_CURRENT_SETPOINT_CONFIGURATION, 0x02, CO_INTEGER16},
  {EPOS_ANALOG_CURRENT_SETPOINT_CONFIGURATION, 0x03, CO_INTEGER8},
  {EPOS_ANALOG_CURRENT_SETPOINT_CONFIGURATION, 0x04, CO_INTEGER16},
  {EPOS_ANALOG_VELOCITY_SETPOINT_CONFIGURATION, 0x01, CO_INTEGER16},
  {EPOS_ANALOG_VELOCITY_SETPOINT_CONFIGURATION, 0x02, CO_INTEGER32},
  {EPOS_ANALOG_VELOCITY_SETPOINT_CONFIGURATION, 0x03, CO_INTEGER8},
  {EPOS_ANALOG_VELOCITY_SETPOINT_CONFIGURATION, 0x04, CO_INTEGER32},
  {EPOS_ANALOG_POSITION_SETPOINT_CONFIGURATION, 0x01, CO_INTEGER16},
  {EPOS_ANALOG_POSITION_SETPOINT_CONFIGURATION, 0x02, CO_INTEGER32},
  {EPOS_ANALOG_POSITION_SETPOINT_CONFIGURATION, 0x03, CO_INTEGER8},
  {EPOS_ANALOG_POSITION_SETPOINT_CONFIGURATION, 0x04, CO_INTEGER32},
  {EPOS_TARGET_TORQUE, 0x00, CO_INTEGER32},
  {EPOS_TORQUE_ACTUAL_VALUE, 0x00, CO_INTEGER32},
  {EPOS_TORQUE_CONTROL_PARAMETER_SET, 0x01, CO_INTEGER32},
  {EPOS_TORQUE_CONTROL_PARAMETER_SET, 0x02, CO_INTEGER32},
  {EPOS_TORQUE_CONTROL_PARAMETER_SET, 0x03, CO_INTEGER32},
  {EPOS_ANALOG_TORQUE_FEEDBACK_CONFIGURATION, 0x01, CO_INTEGER32},
  {EPOS_ANALOG_TORQUE_FEEDBACK_CONFIGURATION, 0x02, CO_INTEGER32},
  {DSP402_TORQUE_CONTROL_PARAMETER_SET, 0x01, CO_INTEGER16},
  {DSP402_TORQUE_CONTROL_PARAMETER_SET, 0x02, CO_INTEGER16},
  {DSP402_VELOCITY_CONTROL_PARAMETER_SET, 0x01, CO_INTEGER16},
  {DSP402_VELOCITY_CONTROL_PARAMETER_SET, 0x02, CO_INTEGER16},
  {DSP402_VELOCITY_CONTROL_PARAMETER_SET, 0x04, CO_INTEGER16},
  {DSP402_VELOCITY_CONTROL_PARAMETER_SET, 0x05, CO_INTEGER16},
  {DSP402_POSITION_CONTROL_PARAMETER_SET, 0x01, CO_INTEGER16},
  {DSP402_POSITION_CONTROL_PARAMETER_SET, 0x02, CO_INTEGER16},
  {DSP402_POSITION_CONTROL_PARAMETER_SET, 0x03, CO_INTEGER16},
  {DSP402_POSITION_CONTROL_PARAMETER_SET, 0x04, CO_INTEGER16},
  {DSP402_POSITION_CONTROL_PARAMETER_SET, 0x05, CO_INTEGER16},
  {DSP402_MOTOR_DATA, 0x01, CO_UNSIGNED16},
  {DSP402_MOTOR_DATA, 0x02, CO_UNSIGNED16},
  {DSP402_MOTOR_DATA, 0x03, CO_UNSIGNED8},
  {DSP402_MOTOR_DATA, 0x04, CO_UNSIGNED32},
  {DSP402_MOTOR_DATA, 0x05, CO_UNSIGNED16},
  {CO_SENTINEL}
};

epos_slave::epos_slave(uint8_t node_id) : dsp402_slave(node_id)
{
  /* initialize object dictionary */
  register_dictionary_entries(epos_slave_dictionary);
}
