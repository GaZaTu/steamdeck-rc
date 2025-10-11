#include "RX5808.hpp"

#define C_Bit0 0x0001
#define C_Bit1 0x0002
#define C_Bit2 0x0004
#define C_Bit3 0x0008
#define C_Bit4 0x0010
#define C_Bit5 0x0020
#define C_Bit6 0x0040
#define C_Bit7 0x0080

#define C_Bit00 0x00000001
#define C_Bit01 0x00000002
#define C_Bit02 0x00000004
#define C_Bit03 0x00000008
#define C_Bit04 0x00000010
#define C_Bit05 0x00000020
#define C_Bit06 0x00000040
#define C_Bit07 0x00000080
#define C_Bit08 0x00000100
#define C_Bit09 0x00000200
#define C_Bit10 0x00000400
#define C_Bit11 0x00000800
#define C_Bit12 0x00001000
#define C_Bit13 0x00002000
#define C_Bit14 0x00004000
#define C_Bit15 0x00008000
#define C_Bit16 0x00010000
#define C_Bit17 0x00020000
#define C_Bit18 0x00040000
#define C_Bit19 0x00080000
#define C_Bit20 0x00100000
#define C_Bit21 0x00200000
#define C_Bit22 0x00400000
#define C_Bit23 0x00800000
#define C_Bit24 0x01000000
#define C_Bit25 0x02000000
#define C_Bit26 0x04000000
#define C_Bit27 0x08000000
#define C_Bit28 0x10000000
#define C_Bit29 0x20000000
#define C_Bit30 0x40000000
#define C_Bit31 0x80000000

#define D_Fosc 0x08
#define C_Freq_TabelOffset_Max 15

#define D_RTC6705_SYN_RF_R_REG 0x190 // 0x190=400
#define D_RTC6705_SYN_Define_N (0x000361 >> 2)
#define D_RTC6705_Address02_Value (0x04C2B2 >> 2)

#define D_RTC6715_SYN_RF_R_REG 0x190 // 0x190=400
#define D_RTC6715_SYN_Define_N (0x000361 >> 2)
#define D_RTC6715_Address02_Value (0x04C2B2 >> 2)

static const uint16_t Tab_RTC6705Freq[] =		// ( 5325-5945 )
{// 1			2			3			4			5			6			7			8
	5865, 5845, 5825, 5805, 5785, 5765, 5745, 5725,		// A
	5733, 5752, 5771, 5790, 5809, 5828, 5847, 5866,		// B
	5705, 5685, 5665, 5645, 5885, 5905, 5925, 5945,		// C
	5740, 5760, 5780, 5800, 5820, 5840, 5860, 5880,		// D
	5658, 5695, 5732, 5769, 5806, 5843, 5880, 5917,		// E
	5362, 5399, 5436, 5473, 5510, 5547, 5584, 5621,		// F
	4990, 5020, 5050, 5080, 5110, 5140, 5170, 5200,
};

// void RX5808::Spi_Nop(void) {
//   uint8_t u8_Temp = 10;
//   while (u8_Temp) {
//     u8_Temp--;
//   }
// }

// void RX5808::SpiWrite(uint32_t u32_SPI_Data) {
//   uint8_t u8_SPI_Bitx = 25;
//   uint32_t u32_Data = u32_SPI_Data;

//   Spi_Nop();
//   digitalWrite(_sck, LOW);
//   Spi_Nop();
//   digitalWrite(_ss, LOW);
//   Spi_Nop();

//   while (u8_SPI_Bitx != 0) {
//     Spi_Nop();
//     if ((u32_Data & C_Bit00) == 0) {
//       digitalWrite(_data, LOW);
//     } else {
//       digitalWrite(_data, HIGH);
//     }
//     Spi_Nop();

//     digitalWrite(_sck, HIGH);
//     Spi_Nop();
//     Spi_Nop();
//     digitalWrite(_sck, LOW);
//     u32_Data = u32_Data >> 1;
//     u8_SPI_Bitx--;
//   }

//   Spi_Nop();
//   digitalWrite(_ss, HIGH);
//   Spi_Nop();
// }

// void RX5808::Rtc6705_Write(uint8_t u8_Addr, uint32_t u32_Value) {
//   uint8_t u8_Loop = 8;
//   uint32_t u32_Data = (uint32_t)((u8_Addr & 0x0F) | C_Bit4) & 0x0000001F;
//   u32_Data |= ((u32_Value << 5) & 0x01FFFFE0);
//   SpiWrite(u32_Data);
// }

// void RX5808::Rtc6705_SetFreq(uint8_t Group, uint8_t ch) {
//   uint8_t temp;
//   uint32_t u32_RegN, u32_RegA;
//   uint32_t u32_Freq;

//   Rtc6705_Write(0x00, D_RTC6705_SYN_RF_R_REG); // 8M/400 = 20K
//   Rtc6705_Write(0x0F, 0x000000);
//   temp = (Group - 1) * 8 + (ch - 1);
//   u32_Freq = (uint32_t)Tab_RTC6705Freq[temp];

//   u32_RegN = ((((u32_Freq * D_RTC6705_SYN_RF_R_REG) / D_Fosc) / 2) / 64);
//   u32_RegA = ((((u32_Freq * D_RTC6705_SYN_RF_R_REG) / D_Fosc) / 2) % 64);
//   u32_Freq = (u32_RegN << 7) | (u32_RegA & 0x7F);
//   Rtc6705_Write(0x01, u32_Freq);
// }

// // Channels to sent to the SPI registers
// static const uint16_t channel_table[] PROGMEM = {
//   #define _CHANNEL_REG_FLO(f) ((f - 479) / 2)
//   #define _CHANNEL_REG_N(f) (_CHANNEL_REG_FLO(f) / 32)
//   #define _CHANNEL_REG_A(f) (_CHANNEL_REG_FLO(f) % 32)
//   #define CHANNEL_REG(f) (_CHANNEL_REG_N(f) << 7) | _CHANNEL_REG_A(f)

//   // A
//   CHANNEL_REG(5865),
//   CHANNEL_REG(5845),
//   CHANNEL_REG(5825),
//   CHANNEL_REG(5805),
//   CHANNEL_REG(5785),
//   CHANNEL_REG(5765),
//   CHANNEL_REG(5745),
//   CHANNEL_REG(5725),

//   // B
//   CHANNEL_REG(5733),
//   CHANNEL_REG(5752),
//   CHANNEL_REG(5771),
//   CHANNEL_REG(5790),
//   CHANNEL_REG(5809),
//   CHANNEL_REG(5828),
//   CHANNEL_REG(5847),
//   CHANNEL_REG(5866),

//   // E
//   CHANNEL_REG(5705),
//   CHANNEL_REG(5685),
//   CHANNEL_REG(5665),
//   CHANNEL_REG(5645),
//   CHANNEL_REG(5885),
//   CHANNEL_REG(5905),
//   CHANNEL_REG(5925),
//   CHANNEL_REG(5945),

//   // F
//   CHANNEL_REG(5740),
//   CHANNEL_REG(5760),
//   CHANNEL_REG(5780),
//   CHANNEL_REG(5800),
//   CHANNEL_REG(5820),
//   CHANNEL_REG(5840),
//   CHANNEL_REG(5860),
//   CHANNEL_REG(5880),

//   // C / R
//   CHANNEL_REG(5658),
//   CHANNEL_REG(5695),
//   CHANNEL_REG(5732),
//   CHANNEL_REG(5769),
//   CHANNEL_REG(5806),
//   CHANNEL_REG(5843),
//   CHANNEL_REG(5880),
//   CHANNEL_REG(5917),

//   // L
//   CHANNEL_REG(5362),
//   CHANNEL_REG(5399),
//   CHANNEL_REG(5436),
//   CHANNEL_REG(5473),
//   CHANNEL_REG(5510),
//   CHANNEL_REG(5547),
//   CHANNEL_REG(5584),
//   CHANNEL_REG(5621),

//   #undef _CHANNEL_REG_FLO
//   #undef _CHANNEL_REG_A
//   #undef _CHANNEL_REG_N
//   #undef CHANNEL_REG
// };

// // Channels with their Mhz Values
// static const uint16_t channel_freq_table[] PROGMEM = {
//   5865, 5845, 5825, 5805, 5785, 5765, 5745, 5725, // A
//   5733, 5752, 5771, 5790, 5809, 5828, 5847, 5866, // B
//   5705, 5685, 5665, 5645, 5885, 5905, 5925, 5945, // E
//   5740, 5760, 5780, 5800, 5820, 5840, 5860, 5880, // F / Airwave
//   5658, 5695, 5732, 5769, 5806, 5843, 5880, 5917, // C / Immersion Raceband
//   5362, 5399, 5436, 5473, 5510, 5547, 5584, 5621, // D / 5.3
// };

// // Encode channel names as an 8-bit value where:
// //      0b00000111 = channel number (zero-indexed)
// //      0b11111000 = channel letter (offset from 'A' character)
// static const uint8_t channel_names[] PROGMEM = {
//   #define _CHANNEL_NAMES(l) (uint8_t) ((l - 65) << 3)
//   #define CHANNEL_NAMES(l) \
//     _CHANNEL_NAMES(l) | 0, \
//     _CHANNEL_NAMES(l) | 1, \
//     _CHANNEL_NAMES(l) | 2, \
//     _CHANNEL_NAMES(l) | 3, \
//     _CHANNEL_NAMES(l) | 4, \
//     _CHANNEL_NAMES(l) | 5, \
//     _CHANNEL_NAMES(l) | 6, \
//     _CHANNEL_NAMES(l) | 7

//   CHANNEL_NAMES('A'),
//   CHANNEL_NAMES('B'),
//   CHANNEL_NAMES('E'),
//   CHANNEL_NAMES('F'), // a.k.a Airwave
//   CHANNEL_NAMES('R'), // C / Immersion Raceband
//   CHANNEL_NAMES('L'),

//   #undef CHANNEL_NAMES
//   #undef _CHANNEL_NAMES
// };

// // All Channels of the above List ordered by Mhz
// static const uint8_t channel_freq_ordered_index[] PROGMEM = {
//   40, // 5362
//   41, // 5399
//   42, // 5436
//   43, // 5473
//   44, // 5510
//   45, // 5547
//   46, // 5584
//   47, // 5621
//   19, // 5645
//   32, // 5658
//   18, // 5665
//   17, // 5685
//   33, // 5695
//   16, // 5705
//    7, // 5725
//   34, // 5732
//    8, // 5733
//   24, // 5740
//    6, // 5745
//    9, // 5752
//   25, // 5760
//    5, // 5765
//   35, // 5769
//   10, // 5771
//   26, // 5780
//    4, // 5785
//   11, // 5790
//   27, // 5800
//    3, // 5805
//   36, // 5806
//   12, // 5809
//   28, // 5820
//    2, // 5825
//   13, // 5828
//   29, // 5840
//   37, // 5843
//    1, // 5845
//   14, // 5847
//   30, // 5860
//    0, // 5865
//   15, // 5866
//   31, // 5880
//   38, // 5880
//   20, // 5885
//   21, // 5905
//   39, // 5917
//   22, // 5925
//   23, // 5945
// };

// static const uint8_t channel_index_to_ordered_index[] PROGMEM = {
//   39,
//   36,
//   32,
//   28,
//   25,
//   21,
//   18,
//   14,
//   16,
//   19,
//   23,
//   26,
//   30,
//   33,
//   37,
//   40,
//   13,
//   11,
//   10,
//    8,
//   43,
//   44,
//   46,
//   47,
//   17,
//   20,
//   24,
//   27,
//   31,
//   34,
//   38,
//   41,
//    9,
//   12,
//   15,
//   22,
//   29,
//   35,
//   42,
//   45,
//    0,
//    1,
//    2,
//    3,
//    4,
//    5,
//    6,
//    7,
// };

// uint16_t RX5808::getSynthRegisterBByChannel(uint8_t index) {
//   return pgm_read_word_near(channel_table + index);
// }

// uint16_t RX5808::getChannelFrequency(uint8_t index) {
//   return pgm_read_word_near(channel_freq_table + index);
// }

// // Returns channel name as a string.
// static char channel_name_buffer[3];
// const char* RX5808::getChannelName(uint8_t index) {
//   uint8_t encodedName = pgm_read_byte_near(channel_names + index);

//   channel_name_buffer[0] = 65 + (encodedName >> 3);
//   channel_name_buffer[1] = 48 + (encodedName & (255 >> (8 - 3))) + 1;
//   channel_name_buffer[2] = '\0';

//   return channel_name_buffer;
// }
