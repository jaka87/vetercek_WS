#ifndef power2_h
#define power2_h

#if defined(__AVR_HAVE_PRR0_PRSPI0)
#define power_spi0_enable()      (PRR0 &= (uint8_t)~(1 << PRSPI0))
#define power_spi0_disable()     (PRR0 |= (uint8_t)(1 << PRSPI0))
#endif

#if defined(__AVR_HAVE_PRR1_PRSPI1)
#define power_spi1_enable()      (PRR1 &= (uint8_t)~(1 << PRSPI1))
#define power_spi1_disable()     (PRR1 |= (uint8_t)(1 << PRSPI1))
#endif

#if defined(__AVR_HAVE_PRR0_PRTWI0)
#define power_twi0_enable()      (PRR0 &= (uint8_t)~(1 << PRTWI0))
#define power_twi0_disable()     (PRR0 |= (uint8_t)(1 << PRTWI0))
#endif

#if defined(__AVR_HAVE_PRR1_PRTWI1)
#define power_twi1_enable()      (PRR1 &= (uint8_t)~(1 << PRTWI1))
#define power_twi1_disable()     (PRR1 |= (uint8_t)(1 << PRTWI1))
#endif

#if defined(__AVR_HAVE_PRR1_PRPTC)
#define power_ptc_enable()      (PRR1 &= (uint8_t)~(1 << PRPTC))
#define power_ptc_disable()     (PRR1 |= (uint8_t)(1 << PRPTC))
#endif

#endif
