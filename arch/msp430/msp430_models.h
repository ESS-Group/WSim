
/**
 *  \file   msp430_models.h
 *  \brief  MSP430 MCU definitions 
 *  \author Antoine Fraboulet
 *  \date   2005
 **/


#ifndef MSP430_MODELS_H
#define MSP430_MODELS_H

/**
   MSP430f135
   MSP430f149 
   MSP430f449
   MSP430f1611   
   MSP430f2013
   MSP430f2274
   CC430f6137
*/

#define MCU_NAME         "msp430" /* used in libelf_ntv */
#define MCU_EM_ARCH_ID   105      /* elf EM_ == 0x69    */
#define MCU_BFD_ARCH_ID  62       /* bfd                */

#if defined(WSIM_USES_GNU_BFD)
#define MCU_ARCH_ID MCU_BFD_ARCH_ID
#else
#define MCU_ARCH_ID MCU_EM_ARCH_ID
#endif

struct _infomem_t {
  uint16_t addr;
  uint8_t  value;
};
typedef struct _infomem_t infomem_t;

/* ********************************************************************** */
/* ********************************************************************** */
/* ********************************************************************** */
/* ********************************************************************** */
#if defined(MSP430f135)
	/* ********************************************************************** */
	/* ********************************************************************** */
	/* ********************************************************************** */
	/* ********************************************************************** */

	#define MCU_BFD_MACH_ID   13 /* bfd_mach_msp13 */
	#define MCU_VERSION       "f135"
	#define MCU_MODEL_NAME    "msp430f135"

	#define ADDR_FLASH_STOP   0xFFFFu
	#define ADDR_FLASH_START  0xC000u
	#define ADDR_NVM_STOP     0x10FFu
	#define ADDR_NVM_START    0x1000u
	#define ADDR_BOOT_STOP    0x0fffu
	#define ADDR_BOOT_START   0x0c00u
	#define ADDR_RAM_STOP     0x03FFu
	#define ADDR_RAM_START    0x0200u

	#define INTR_RESET        15
	#define INTR_NMI          14
	#define INTR_TIMERB_0     13
	#define INTR_TIMERB_1     12
	#define INTR_COMP_A       11
	#define INTR_WATCHDOG     10
	#define INTR_USART0_RX     9
	#define INTR_USART0_TX     8
	#define INTR_ADC12         7
	#define INTR_TIMERA_0      6 // CCR0 CCIFG
	#define INTR_TIMERA_1      5 // CCR1, CCR2, TAIFG
	#define INTR_IOPORT1       4
	#define INTR_UNUSED_1      3
	#define INTR_UNUSED_2      2
	#define INTR_IOPORT2       1
	#define INTR_UNUSED_3      0

	#define __msp430_have_16_interrupts
	#define INTR_FIRST_NMI    14
	#define INTR_BASE_IV_ADDR 0xFFE0u

	// infomem
	static infomem_t UNUSED infomem[] = {};

	// system clock
	#define __msp430_have_basic_clock
	#define __msp430_have_xt2 

	// 8 bit blocks
	#define __msp430_have_port1
	#define __msp430_have_port2
	#define __msp430_have_port3
	#define __msp430_have_port4
	#define __msp430_have_port5
	#define __msp430_have_port6
	#define __msp430_have_usart0
	#define __msp430_have_cmpa

	// 16 bit blocks
	#define __msp430_have_timera3
	#define __msp430_have_timerb3
	#define __msp430_have_watchdog
	#define __msp430_have_adc12
	#define __msp430_have_flash

	// Flash erase timings
	#define FLASH_WRITE_TIMING_BYTE    35
	#define FLASH_WRITE_TIMING_FSTBYTE 30
	#define FLASH_WRITE_TIMING_NXTBYTE 21
	#define FLASH_WRITE_TIMING_LSTBYTE  6
	#define FLASH_ERASE_TIMING_MASS  5297
	#define FLASH_ERASE_TIMING_SEG   4819

	/* ********************************************************************** */
	/* ********************************************************************** */
	/* ********************************************************************** */
	/* ********************************************************************** */
#elif defined(MSP430f149)
	/* ********************************************************************** */
	/* ********************************************************************** */
	/* ********************************************************************** */
	/* ********************************************************************** */

	#define MCU_BFD_MACH_ID   14 /* bfd_mach_msp14 */
	#define MCU_VERSION       "f149"
	#define MCU_MODEL_NAME    "msp430f149"

	#define ADDR_FLASH_STOP   0xFFFFu
	#define ADDR_FLASH_START  0x1100u
	#define ADDR_NVM_STOP     0x10FFu
	#define ADDR_NVM_START    0x1000u
	#define ADDR_BOOT_STOP    0x0fffu
	#define ADDR_BOOT_START   0x0c00u
	#define ADDR_RAM_STOP     0x09FFu
	#define ADDR_RAM_START    0x0200u

	#define INTR_RESET        15
	#define INTR_NMI          14
	#define INTR_TIMERB_0     13
	#define INTR_TIMERB_1     12
	#define INTR_COMP_A       11
	#define INTR_WATCHDOG     10
	#define INTR_USART0_RX     9
	#define INTR_USART0_TX     8
	#define INTR_ADC12         7
	#define INTR_TIMERA_0      6 // CCR0 CCIFG
	#define INTR_TIMERA_1      5 // CCR1, CCR2, TAIFG
	#define INTR_IOPORT1       4
	#define INTR_USART1_RX     3
	#define INTR_USART1_TX     2
	#define INTR_IOPORT2       1
	#define INTR_UNUSED        0 // do not define INTR_DAC12

	#define __msp430_have_16_interrupts
	#define INTR_FIRST_NMI    14
	#define INTR_BASE_IV_ADDR 0xFFE0u

	// infomem
	static infomem_t UNUSED infomem[] = {};

	// system clock
	#define __msp430_have_basic_clock
	#define __msp430_have_xt2 

	// hwmul
	#define __msp430_have_hwmul

	// 8 bit blocks
	#define __msp430_have_port1
	#define __msp430_have_port2
	#define __msp430_have_port3
	#define __msp430_have_port4
	#define __msp430_have_port5
	#define __msp430_have_port6
	#define __msp430_have_usart0
	#define __msp430_have_usart1
	#define __msp430_have_cmpa

	// 16 bit blocks
	#define __msp430_have_adc12
	#define __msp430_have_timera3
	#define __msp430_have_timerb7
	#define __msp430_have_watchdog
	#define __msp430_have_flash

	// Flash erase timings
	#define FLASH_WRITE_TIMING_BYTE    35
	#define FLASH_WRITE_TIMING_FSTBYTE 30
	#define FLASH_WRITE_TIMING_NXTBYTE 21
	#define FLASH_WRITE_TIMING_LSTBYTE  6
	#define FLASH_ERASE_TIMING_MASS  5297
	#define FLASH_ERASE_TIMING_SEG   4819

	/* ********************************************************************** */
	/* ********************************************************************** */
	/* ********************************************************************** */
	/* ********************************************************************** */
#elif defined(MSP430f449)
	/* ********************************************************************** */
	/* ********************************************************************** */
	/* ********************************************************************** */
	/* ********************************************************************** */

	#define MCU_BFD_MACH_ID   44  /* bfd_mach_msp44 */
	#define MCU_VERSION       "f449"
	#define MCU_MODEL_NAME    "msp430f449"

	#define ADDR_FLASH_STOP   0xFFFFu
	#define ADDR_FLASH_START  0x1100u
	#define ADDR_NVM_STOP     0x10FFu
	#define ADDR_NVM_START    0x1000u
	#define ADDR_BOOT_STOP    0x0fffu
	#define ADDR_BOOT_START   0x0c00u
	#define ADDR_RAM_STOP     0x09FFu
	#define ADDR_RAM_START    0x0200u

	#define INTR_RESET         15
	#define INTR_NMI           14
	#define INTR_TIMERB_0      13
	#define INTR_TIMERB_1      12
	#define INTR_COMP_A        11
	#define INTR_WATCHDOG      10
	#define INTR_USART0_RX      9
	#define INTR_USART0_TX      8
	#define INTR_ADC12          7
	#define INTR_TIMERA_0       6
	#define INTR_TIMERA_1       5
	#define INTR_IOPORT1        4
	#define INTR_USART1_RX      3
	#define INTR_USART1_TX      2
	#define INTR_IOPORT2        1
	#define INTR_BT             0

	#define __msp430_have_16_interrupts
	#define INTR_FIRST_NMI    14
	#define INTR_BASE_IV_ADDR 0xFFE0u

	// infomem
	static infomem_t UNUSED infomem[] = {};

	// system clock
	#define __msp430_have_fll_and_xt2
	#define __msp430_have_xt2 

	// hwmul
	#define __msp430_have_hwmul

	// 8 bit blocks
	#define __msp430_have_port1
	#define __msp430_have_port2
	#define __msp430_have_port3
	#define __msp430_have_port4
	#define __msp430_have_port5
	#define __msp430_have_port6
	#define __msp430_have_usart0
	#define __msp430_have_usart0_with_i2c
	#define __msp430_have_usart1
	#define __msp430_have_cmpa
	#define __msp430_have_svs_at_0x55
	#define __msp430_have_basic_timer
	#define __msp430_have_lcd

	// 16 bit blocks
	#define __msp430_have_timera3
	#define __msp430_have_timerb7
	#define __msp430_have_watchdog
	#define __msp430_have_adc12
	#define __msp430_have_flash

	// Flash erase timings
	#define FLASH_WRITE_TIMING_BYTE    35
	#define FLASH_WRITE_TIMING_FSTBYTE 30
	#define FLASH_WRITE_TIMING_NXTBYTE 21
	#define FLASH_WRITE_TIMING_LSTBYTE  6
	#define FLASH_ERASE_TIMING_MASS  5297
	#define FLASH_ERASE_TIMING_SEG   4819

	/* ********************************************************************** */
	/* ********************************************************************** */
	/* ********************************************************************** */
	/* ********************************************************************** */
#elif defined(MSP430f1611) || defined(MSP430f1612)
	/* ********************************************************************** */
	/* ********************************************************************** */
	/* ********************************************************************** */
	/* ********************************************************************** */
	#if defined(MSP430f1611)

		#define MCU_BFD_MACH_ID    16 /* bfd_mach_msp16 */
		#define MCU_VERSION        "f1611"
		#define MCU_MODEL_NAME     "msp430f1611"

		#define ADDR_FLASH_STOP    0xFFFFu
		#define ADDR_FLASH_START   0x4000u
		#define ADDR_RAM_STOP      0x38FFu
		#define ADDR_RAM_START     0x1100u
		#define ADDR_NVM_STOP      0x10ffu
		#define ADDR_NVM_START     0x1000u
		#define ADDR_BOOT_STOP     0x0fffu
		#define ADDR_BOOT_START    0x0c00u
		#define ADDR_MIRROR_STOP   0x09ffu
		#define ADDR_MIRROR_START  0x0200u

	#elif defined(MSP430f1612)

		#define MCU_BFD_MACH_ID    16 /* bfd_mach_msp16 */
		#define MCU_VERSION        "f1612"
		#define MCU_MODEL_NAME     "msp430f1612"

		#define ADDR_FLASH_STOP    0xFFFFu
		#define ADDR_FLASH_START   0x2500u
		#define ADDR_RAM_STOP      0x24FFu
		#define ADDR_RAM_START     0x1100u
		#define ADDR_NVM_STOP      0x10ffu
		#define ADDR_NVM_START     0x1000u
		#define ADDR_BOOT_STOP     0x0fffu
		#define ADDR_BOOT_START    0x0c00u
		#define ADDR_MIRROR_STOP   0x09ffu
		#define ADDR_MIRROR_START  0x0200u

	#endif

	#define INTR_RESET        15
	#define INTR_NMI          14
	#define INTR_TIMERB_0     13
	#define INTR_TIMERB_1     12
	#define INTR_COMP_A       11
	#define INTR_WATCHDOG     10
	#define INTR_USART0_RX     9
	#define INTR_USART0_TX     8
	#define INTR_ADC12         7
	#define INTR_TIMERA_0      6 // CCR0 CCIFG
	#define INTR_TIMERA_1      5 // CCR1, CCR2, TAIFG
	#define INTR_IOPORT1       4
	#define INTR_USART1_RX     3
	#define INTR_USART1_TX     2
	#define INTR_IOPORT2       1
	#define INTR_DAC12         0

	#define INTR_DMA          INTR_DAC12
	#define INTR_I2C          INTR_USART0_TX

	#define __msp430_have_16_interrupts
	#define INTR_FIRST_NMI    14
	#define INTR_BASE_IV_ADDR 0xFFE0u

	// infomem
	static infomem_t UNUSED infomem[] = {};

	// system clock
	#define __msp430_have_basic_clock
	#define __msp430_have_xt2 

	// hwmul
	#define __msp430_have_hwmul

	// 8 bit blocks
	#define __msp430_have_port1
	#define __msp430_have_port2
	#define __msp430_have_port3
	#define __msp430_have_port4
	#define __msp430_have_port5
	#define __msp430_have_port6
	#define __msp430_have_usart0
	#define __msp430_have_usart0_with_i2c
	#define __msp430_have_usart1
	#define __msp430_have_cmpa
	#define __msp430_have_svs_at_0x55

	// 16 bit blocks
	#define __msp430_have_timera3
	#define __msp430_have_timerb7
	#define __msp430_have_watchdog
	#define __msp430_have_adc12
	#define __msp430_have_dac12
	#define __msp430_have_dma
	#define __msp430_have_flash

	// Flash erase timings
	#define FLASH_WRITE_TIMING_BYTE    35
	#define FLASH_WRITE_TIMING_FSTBYTE 30
	#define FLASH_WRITE_TIMING_NXTBYTE 21
	#define FLASH_WRITE_TIMING_LSTBYTE  6
	#define FLASH_ERASE_TIMING_MASS  5297
	#define FLASH_ERASE_TIMING_SEG   4819

	/* ********************************************************************** */
	/* ********************************************************************** */
	/* ********************************************************************** */
	/* ********************************************************************** */
#elif defined(MSP430f2012) || defined(MSP430f2013)
	#if defined(MSP430f2012)
		#define MCU_BFD_MACH_ID   20
		#define MCU_VERSION       "f2013"
		#define MCU_MODEL_NAME    "msp430f2013"
	#elif defined(MSP430f2013)
		#define MCU_BFD_MACH_ID   20
		#define MCU_VERSION       "f2012"
		#define MCU_MODEL_NAME    "msp430f2012"
	#endif

	#define ADDR_FLASH_STOP   0xFFFFu
	#define ADDR_FLASH_START  0xF800u
	#define ADDR_NVM_STOP     0x10ffu
	#define ADDR_NVM_START    0x1000u
	#define ADDR_RAM_STOP     0x027fu
	#define ADDR_RAM_START    0x0200u

	#define INTR_RESET        31
	#define INTR_NMI          30
	// 29
	// 28
	#define INTR_COMP_A       27
	#define INTR_WATCHDOG     26
	#define INTR_TIMERA2_0    25
	#define INTR_TIMERA2_1    24     
	// 23
	// 22
	#define INTR_ADC10        21
	#define INTR_USI          20       
	#define INTR_IOPORT2      19
	#define INTR_IOPORT1      18
	// 17 - 0

	#define __msp430_have_32_interrupts
	#define INTR_FIRST_NMI    30
	#define INTR_BASE_IV_ADDR 0xFFC0u

	// infomem
	static infomem_t UNUSED infomem[] = {};

	// system clock
	#define __msp430_have_basic_clock
	#define __msp430_have_xt2 

	// 8 bit blocks
	#define __msp430_have_port1
	#define __msp430_have_port2
	#define __msp430_have_port3
	#define __msp430_have_port4
	#define __msp430_have_port5
	#define __msp430_have_port6
	#define __msp430_have_usi

	// 16 bit blocks
	#define __msp430_have_flash
	#define __msp430_have_timera2
	#define __msp430_have_watchdog

	#if defined(MSP430f2012)
	#define __msp430_have_adc10
	#elif defined(MSP430f2013)
	#define __msp430_have_sd16
	#endif

	// Flash erase timings
	#define FLASH_WRITE_TIMING_BYTE       30
	#define FLASH_WRITE_TIMING_FSTBYTE    25
	#define FLASH_WRITE_TIMING_NXTBYTE    18
	#define FLASH_WRITE_TIMING_LSTBYTE     6
	#define FLASH_ERASE_TIMING_MASS    10593
	#define FLASH_ERASE_TIMING_SEG      4819

	//BASE Register (OFFSET)
	#define USCIA_BASE        0x05d
	#define USCIB_BASE        0x068

	/* ********************************************************************** */
	/* ********************************************************************** */
	/* ********************************************************************** */
	/* ********************************************************************** */
#elif defined(MSP430f2274)
	#define MCU_BFD_MACH_ID   22
	#define MCU_VERSION       "f2274"
	#define MCU_MODEL_NAME    "msp430f2274"

	#define ADDR_FLASH_STOP    0xFFFFu
	#define ADDR_FLASH_START   0x8000u
	#define ADDR_NVM_STOP      0x10ffu
	#define ADDR_NVM_START     0x1000u
	#define ADDR_BOOTMEM_STOP  0x0FFFu
	#define ADDR_BOOTMEM_START 0x0C00u
	#define ADDR_RAM_STOP      0x05FFu
	#define ADDR_RAM_START     0x0200u

	#define INTR_RESET        31
	#define INTR_NMI          30
	#define INTR_TIMERB_0     29
	#define INTR_TIMERB_1     28
	// 27
	#define INTR_WATCHDOG     26
	#define INTR_TIMERA_0     25
	#define INTR_TIMERA_1     24     
	#define INTR_USCIX0_RX    23 // Multiple Source Flag (USCIA0 & USCIB0)
	#define INTR_USCIX0_TX    22 // Multiple Source Flag (USCIA0 & USCIB0)
	#define INTR_ADC10        21
	// 20
	#define INTR_IOPORT2      19
	#define INTR_IOPORT1      18
	// 17 - 0

	#define __msp430_have_32_interrupts
	#define INTR_FIRST_NMI    30
	#define INTR_BASE_IV_ADDR 0xFFC0u

	// infomem
	static infomem_t UNUSED infomem[] = {
	{ 0x10f8, 0x8e },
	{ 0x10f9, 0x8e },
	{ 0x10fa, 0xac },
	{ 0x10fb, 0x8d },
	{ 0x10fc, 0x7a },
	{ 0x10fd, 0x8c },
	{ 0x10fe, 0xb7 },
	{ 0x10ff, 0x85 }
	};

	// system clock
	#define __msp430_have_basic_clock_plus

	// 8 bit blocks
	#define __msp430_have_port1
	#define __msp430_have_port2
	#define __msp430_have_port3
	#define __msp430_have_port4
	#define __msp430_have_sel2
	#define __msp430_have_ren
	#define __msp430_have_uscia0  /* uart/lin + IrDA + SPI */
	#define __msp430_have_uscib0  /* SPI + I2C             */
	#define __msp430_have_adc10
	#define __msp430_have_cmpa
	#define __msp430_have_cmpa_plus
	#define __msp430_have_oa0
	#define __msp430_have_oa1

	// 16 bit blocks
	#define __msp430_have_timera3
	#define __msp430_have_timerb3
	#define __msp430_have_watchdog
	#define __msp430_have_adc10
	#define __msp430_have_flash

	// Flash erase timings
	#define FLASH_WRITE_TIMING_BYTE       30
	#define FLASH_WRITE_TIMING_FSTBYTE    25
	#define FLASH_WRITE_TIMING_NXTBYTE    18
	#define FLASH_WRITE_TIMING_LSTBYTE     6
	#define FLASH_ERASE_TIMING_MASS    10593
	#define FLASH_ERASE_TIMING_SEG      4819

	//BASE Register (OFFSET)
	#define USCIA_BASE        0x05d
	#define USCIB_BASE        0x068

	/* ********************************************************************** */
	/* ********************************************************************** */
	/* ********************************************************************** */
	/* ********************************************************************** */
#elif defined(CC430f6137)
	#define MCU_BFD_MACH_ID   54
	#define MCU_VERSION       "f6137"
	#define MCU_MODEL_NAME    "cc430f6137"

	#define ADDR_FLASH_STOP    0xFFFFu
	#define ADDR_FLASH_START   0x8000u
	#define ADDR_NVM_STOP      0x19FFu
	#define ADDR_NVM_START     0x1800u
	#define ADDR_BOOTMEM_STOP  0x17FFu
	#define ADDR_BOOTMEM_START 0x1000u
	#define ADDR_RAM_STOP      0x2BFFu
	#define ADDR_RAM_START     0x1C00u

	#define INTR_RESET        63
	// 62
	#define INTR_NMI          61 // user NMI, 62 is SYS NMI
	// 60 - Comparator_B
	#define INTR_WATCHDOG     59
	#define INTR_USCIA0_RXTX  58
	#define INTR_USCIB0_RXTX  57
	#define INTR_ADC12        56
	#define INTR_TIMERTA0_0   55
	#define INTR_TIMERTA0_1   54
	#define INTR_RF1A         53
	#define INTR_DMA          52
	#define INTR_TIMERTA1_0   51
	#define INTR_TIMERTA1_1   50
	#define INTR_IOPORT1      49
	#define INTR_IOPORT2      48
	#define INTR_LCDB         47
	// 46 -- RTC
	// 45 -- AES
	// 44-0


	#define __msp430_have_64_interrupts
	#define INTR_FIRST_NMI    61
	//#define INTR_FIRST_NMI    54
	#define INTR_BASE_IV_ADDR 0xFF80u

	// infomem
	static infomem_t UNUSED infomem[] = {};

	// system clock
	#define __msp430_have_ucs
	#define __msp430_have_xt2

	//ucscia ucsib
	//#define __msp430_have_uscia0
	//#define __msp430_have_uscib0

	// 8 bit blocks
	#define __msp430_have_portmap
	#define __msp430_have_port1
	#define __msp430_have_port2
	#define __msp430_have_port3
	#define __msp430_have_port4
	#define __msp430_have_port5 // from gcc4.6+ defines
	#define __msp430_have_portj

	// 16 bit blocks
	#define __msp430_have_flash
	#define __msp430_have_watchdog
	#define __msp430_have_watchdog_plus
	#define __msp430_have_lcdb
	#define __msp430_have_timerTA0
	#define __msp430_have_timerTA1
	#define __msp430_have_pmm
	#define __msp430_have_rtc
	#define __msp430_have_new_sfr

	// Flash erase timings
	#define FLASH_WRITE_TIMING_BYTE       30
	#define FLASH_WRITE_TIMING_FSTBYTE    25
	#define FLASH_WRITE_TIMING_NXTBYTE    18
	#define FLASH_WRITE_TIMING_LSTBYTE     6
	#define FLASH_ERASE_TIMING_MASS    10593
	#define FLASH_ERASE_TIMING_SEG      4819

	//BASE Register (OFFSET)
	#define USCIA_BASE        0x05d
	#define SFR_BASE          0x100
	#define PMM_BASE          0x120
	#define FLASHCTL_BASE     0x140
	#define WATCHDOG_BASE     0x15c
	#define UCS_BASE          0x160
	#define SYSRSTIV_BASE     0x190
	#define PORTMAP_BASE      0x1c0
	#define DIGIIO_BASE       0x200
	#define TIMER_TA0_BASE    0x340
	#define TIMER_TA1_BASE    0x380
	#define RTC_BASE          0x4a0
	#define USCIB_BASE        0x5e0
	#define LCDB_BASE         0xa00

	#define DIGIIO_NEW_OFFSETS
	
	#define WATCHDOG_THREE_BITS_WDTIS

	/* ********************************************************************** */
	/* ********************************************************************** */
	/* ********************************************************************** */
	/* ********************************************************************** */
#else
	#error "you must define one MSP430 mcu model"
#endif // defined(model)

/* ********************************************************************** */
/* ********************************************************************** */
/* ********************************************************************** */
/* ** Common to all models ********************************************** */
/* ********************************************************************** */
/* ********************************************************************** */
/* ********************************************************************** */

#if defined(WSIM_USES_GNU_BFD)
	#define MCU_MACH_ID MCU_BFD_MACH_ID
#else
	#define MCU_MACH_ID MCU_BFD_MACH_ID
#endif

#endif // MSP430_MODELS_H
