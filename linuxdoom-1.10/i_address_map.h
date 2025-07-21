/* Heaviweight F2H Bridge */
/* Memory */
#define DDR_BASE            0x00000000
#define DDR_SPAN            0x3FFFFFFF
#define A9_ONCHIP_BASE      0xFFFF0000
#define A9_ONCHIP_SPAN      0x0000FFFF

/* Heaviweight H2F Bridge */
/* Memory */
#define SDRAM_BASE          0xC0000000
#define SDRAM_SPAN          0x03FFFFFF
#define FPGA_ONCHIP_BASE    0xC8000000
#define FPGA_ONCHIP_SPAN    0x0003FFFF
#define FPGA_CHAR_BASE      0xC9000000
#define FPGA_CHAR_SPAN      0x00001FFF

/* Lightweight H2F Bridge */
#define LW_BRIDGE_BASE      0xFF200000
#define LW_BRIDGE_SPAN      0x00005000
#define BRIDGE_SETTINGS     0xFFD0501C
    /* Cyclone V FPGA devices */
    #define LEDR_BASE           0x00000000
    #define HEX3_HEX0_BASE      0x00000020
    #define HEX5_HEX4_BASE      0x00000030
    #define SW_BASE             0x00000040
    #define KEY_BASE            0x00000050
    #define JP1_BASE            0x00000060
    #define JP2_BASE            0x00000070
    #define JTAG_UART_BASE      0x00001000
    #define JTAG_UART_2_BASE    0x00001008
    #define IrDA_BASE           0x00001020
    #define AV_CONFIG_BASE      0x00003000
    #define PIXEL_BUF_CTRL_BASE 0x00003020
    #define CHAR_BUF_CTRL_BASE  0x00003030
    #define DOOM_DRIVER_BASE    0x00007000