// SITech counts/servero loop -> speed value
#define SPEED_MULTI                     65536

// Crystal frequency
#define CRYSTAL_FREQ                    96000000

// Bits in error state
#define ERROR_GATE_VOLTS_LOW            0x001
#define ERROR_OVERCURRENT_HARDWARE      0x002
#define ERROR_OVERCURRENT_FIRMWARE	0x004
#define ERROR_MOTOR_VOLTS_LOW		0x008
#define ERROR_POWER_BOARD_OVER_TEMP     0x010
#define ERROR_NEEDS_RESET               0x020
#define ERROR_LIMIT_MINUS               0x040
#define ERROR_LIMIT_PLUS                0x080
#define ERROR_TIMED_OVER_CURRENT        0x100
#define ERROR_POSITION_ERROR            0x200
#define ERROR_BISS_ENCODER_ERROR        0x400
#define ERROR_CHECKSUM                  0x800

