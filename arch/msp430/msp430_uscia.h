
/**
 *  \file   msp430_uscia.h
 *  \brief  MSP430 USCIA definition (based on "msp430_usart.h" UART MODE)
 *  \author Julien Carpentier
 *  \date   2011
 **/

#ifndef MSP430_USCIA_H
#define MSP430_USCIA_H

#if defined(WORDS_BIGENDIAN)
struct __attribute__ ((packed)) ucaxctl0_t {
  uint8_t
    ucpen:1,
    ucpar:1,    
    ucmsb:1,    
    uc7bit:1,  
    ucspb:1, 
    ucmode:2,   
    ucsync:1;   
};
#else
struct __attribute__ ((packed)) ucaxctl0_t {
  uint8_t
    ucsync:1,
    ucmode:2,
    ucspb:1,
    uc7bit:1,
    ucmsb:1,
    ucpar:1,
    ucpen:1;
};
#endif

#if defined(WORDS_BIGENDIAN)
struct __attribute__ ((packed)) ucaxctl1_t {
  uint8_t
    ucssel:2,
    ucrxeie:1,
    ucbrkie:1,  
    ucdorm:1,  
    uctxaddr:1, 
    uctxbrk:1,   
    ucswrst:1;  
};
#else
struct __attribute__ ((packed)) ucaxctl1_t {
  uint8_t
    ucswrst:1,
    uctxbrk:1, 
    uctxaddr:1,
    ucdorm:1,
    ucbrkie:1, 
    ucrxeie:1,
    ucssel:2;
};
#endif

#if defined(WORDS_BIGENDIAN)
struct __attribute__ ((packed)) ucaxmctl_t {
  uint8_t
    ucbrf:4,
    ucbrs:3,
    ucos16:1;
};
#else
struct __attribute__ ((packed)) ucaxmctl_t {
  uint8_t
    ucos16:1,
    ucbrs:3,
    ucbrf:4;
};
#endif

#if defined(WORDS_BIGENDIAN)
struct __attribute__ ((packed)) ucaxstat_t {
  uint8_t
    uclisten:1,
    ucfe:1, 
    ucoe:1,
    ucpe:1,
    ucbrk:1, 
    ucrxerr:1,
    ucaddr:1,
    ucbusy:1;
};
#else
struct __attribute__ ((packed)) ucaxstat_t {
  uint8_t
    ucbusy:1,
    ucaddr:1,
    ucrxerr:1,
    ucbrk:1,
    ucpe:1,
    ucoe:1,
    ucfe:1,
    uclisten:1;
};
#endif

#if defined(WORDS_BIGENDIAN)
struct __attribute__ ((packed)) ucaxirtctl_t {
  uint8_t
    ucirtxpl:6,
    ucirtxclk:1,
    uciren:1;
};
#else
struct __attribute__ ((packed)) ucaxirtctl_t {
  uint8_t
    uciren:1,
    ucirtxclk:1,
    ucirtxpl:6;
};
#endif

#if defined(WORDS_BIGENDIAN)
struct __attribute__ ((packed)) ucaxirrctl_t {
  uint8_t
    ucirrxfl:6,
    ucirrxpl:1,
    ucirrxfe:1;
};
#else
struct __attribute__ ((packed)) ucaxirrctl_t {
  uint8_t
    ucirrxfe:1,
    ucirrxpl:1,
    ucirrxfl:6;
};
#endif

#if defined(WORDS_BIGENDIAN)
struct __attribute__ ((packed)) ucaxabctl_t {
  uint8_t
    reserved0:2,
    ucdelim:2,
    ucstoe:1,
    ucbtoe:1,
    reserved1:1,
    ucabden:1;
};
#else
struct __attribute__ ((packed)) ucaxabctl_t {
  uint8_t
    ucabden:1,
    reserved1:1,
    ucbtoe:1,
    ucstoe:1,
    ucdelim:2,
    reserved0:2;
};
#endif

#if defined(__msp430_have_new_uscia)

#if defined(WORDS_BIGENDIAN)
struct __attribute__ ((packed)) ucaxie_t {
  uint8_t
    reserved:6,
    uctxie:1,
    ucrxie:1;
};
#else
struct __attribute__ ((packed)) ucaxie_t {
  uint8_t
    ucrxie:1,
    uctxie:1,
    reserved:6;
};
#endif

#if defined(WORDS_BIGENDIAN)
struct __attribute__ ((packed)) ucaxifg_t {
  uint8_t
    reserved:6,
    uctxifg:1,
    ucrxifg:1;
};
#else
struct __attribute__ ((packed)) ucaxifg_t {
  uint8_t
    ucrxifg:1,
    uctxifg:1,
    reserved:6;
};
#endif

#if defined(WORDS_BIGENDIAN)
struct __attribute__ ((packed)) ucaxiv_t {
  uint16_t
    reserved:13,
    ucivx:2,
    reserved1:1;
};
#else
struct __attribute__ ((packed)) ucaxiv_t {
  uint16_t
    reserved1:1,
    ucivx:2,
    reserved:13;
};
#endif

#endif
  
struct msp430_uscia_t
{
  union {
    struct ucaxctl0_t    b;
    int8_t               s;
  } ucaxctl0;
  union {
    struct ucaxctl1_t    b;
    int8_t               s;
  } ucaxctl1;
  union {
    struct ucaxmctl_t    b;
    int8_t               s;
  } ucaxmctl;
  union {
    struct ucaxstat_t    b;
    int8_t               s;
  } ucaxstat;
  union {
    struct ucaxabctl_t   b;
    int8_t               s;
  } ucaxabctl;
  union {
    struct ucaxirtctl_t  b;
    int8_t               s;
  } ucaxirtctl;
  union {
    struct ucaxirrctl_t  b;
    int8_t               s;
  } ucaxirrctl;
#if defined(__msp430_have_new_uscia)
  union {
    struct ucaxie_t      b;
    int8_t               s;
  } ucaxie;
  union {
    struct ucaxifg_t     b;
    int8_t               s;
  } ucaxifg;
  union {
    struct ucaxiv_t      b;
    int16_t              s;
  } ucaxiv;
#endif
  
  uint8_t   ucaxbr0;
  uint8_t   ucaxbr1;
  uint8_t   ucaxrxbuf;
  uint8_t   ucaxtxbuf;
  uint32_t  ucaxbr_div;
  
  uint8_t   ucaxrxbuf_full;
  uint8_t   ucaxrx_shift_buf;
  uint8_t   ucaxrx_shift_empty;
  uint8_t   ucaxrx_shift_ready;
  int32_t   ucaxrx_shift_delay;
  uint8_t   ucaxrx_slave_rx_done;

  uint8_t   ucaxtxbuf_full;
  uint8_t   ucaxtx_shift_buf;
  uint8_t   ucaxtx_shift_empty;
  uint8_t   ucaxtx_shift_ready;
  int32_t   ucaxtx_shift_delay;
  int32_t   ucaxtx_full_delay;  /* delay between tx_buff and shifter */
  
};





/* ************************************************** */
/* ************************************************** */
/* ************************************************** */

#if defined(__msp430_have_uscia0)
#if defined(__msp430_have_new_uscia)

#define USCIA0_START  USCIA_BASE
#define USCIA0_END    USCIA_BASE + 0x1e

#define UCA0CTL0      USCIA_BASE + 0x01
#define UCA0CTL1      USCIA_BASE + 0x00
#define UCA0BR0       USCIA_BASE + 0x06
#define UCA0BR1       USCIA_BASE + 0x07
#define UCA0MCTL      USCIA_BASE + 0x08
#define UCA0STAT      USCIA_BASE + 0x0a
#define UCA0RXBUF     USCIA_BASE + 0x0c
#define UCA0TXBUF     USCIA_BASE + 0x0e
#define UCA0ABCTL     USCIA_BASE + 0x10
#define UCA0IRTCTL    USCIA_BASE + 0x12
#define UCA0IRRCTL    USCIA_BASE + 0x13 
#define UCA0IE        USCIA_BASE + 0x1c
#define UCA0IFG       USCIA_BASE + 0x1d
#define UCA0IV        USCIA_BASE + 0x1e

int16_t msp430_uscia0_read16(uint16_t addr);
void msp430_uscia0_write16(uint16_t UNUSED addr, int8_t UNUSED val);

#else

#define USCIA0_START  USCIA_BASE
#define USCIA0_END    USCIA_BASE+10

#define UCA0ABCTL     USCIA_BASE
#define UCA0IRTCTL    USCIA_BASE+1
#define UCA0IRRCTL    USCIA_BASE+2
#define UCA0CTL0      USCIA_BASE+3
#define UCA0CTL1      USCIA_BASE+4
#define UCA0BR0       USCIA_BASE+5
#define UCA0BR1       USCIA_BASE+6
#define UCA0MCTL      USCIA_BASE+7
#define UCA0STAT      USCIA_BASE+8
#define UCA0RXBUF     USCIA_BASE+9
#define UCA0TXBUF     USCIA_BASE+10
#endif


void   msp430_uscia0_create();
void   msp430_uscia0_reset();
void   msp430_uscia0_update();
int8_t msp430_uscia0_read (uint16_t addr);
void   msp430_uscia0_write(uint16_t addr, int8_t val);
int    msp430_uscia0_chkifg();


int    msp430_uscia0_dev_read_uart      (uint8_t *val);
void   msp430_uscia0_dev_write_uart     (uint8_t val);
int    msp430_uscia0_dev_write_uart_ok  ();


#else
#define msp430_uscia0_create() do { } while (0)
#define msp430_uscia0_reset()  do { } while (0)
#define msp430_uscia0_update() do { } while (0)
#endif
/* ************************************************** */
/* ************************************************** */
/* ************************************************** */
#endif

/* ************************************************** */
/* ************************************************** */
/* ************************************************** */


