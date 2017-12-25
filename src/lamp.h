#ifndef LAMP_H
#define LAMP_H

#define F_CPU 16000000L

#include <avr/io.h>
#include <util/delay.h>
#include <stdlib.h>

/* #define DBG_MODE */

/********** Board-specific parameters ***********/
#define DDR_RED    DDRB
#define DDR_GREEN  DDRB
#define DDR_BLUE   DDRB

#define PORT_RED   PORTB
#define PORT_GREEN PORTB
#define PORT_BLUE  PORTB

#define PIN_RED    (1 << PB1)
#define PIN_GREEN  (1 << PB2)
#define PIN_BLUE   (1 << PB3)

#define TCCR_INIT_RED   { \
                            TCCR1A |= (1 << COM1A1) | (0 << COM1A0) | (0 << WGM11) | (1 << WGM10); \
							TCCR1B |= (0 << WGM13)  | (1 << WGM12)  | (0 << CS12)  | (0 << CS11) | (1 << CS10); \
                        }

#define TCCR_INIT_BLUE  { \
						    TCCR2 |= (1 << WGM20) | (1 << WGM21) | \
							         (1 << COM21) | (0 << COM20) | \
                                     (0 << CS22) | (0 << CS21) | (1 << CS20); \
						}

#define TCCR_INIT_GREEN { \
                            TCCR1A |= (1 << COM1B1) | (0 << COM1B0) | (0 << WGM11) | (1 << WGM10); \
    						TCCR1B |= (0 << WGM13)  | (1 << WGM12)  | (0 << CS12)  | (0 << CS11) | (1 << CS10); \
                        }

#define OCR_RED    OCR1A
#define OCR_GREEN  OCR1B
#define OCR_BLUE   OCR2

#define MIC_ADC_CHANNEL 0

#define DDR_BTN      DDRD
#define PORT_BTN     PORTD
#define PIN_REG_BTN  PIND
#define PIN_BTN      (1 << PD0)

#define DDR_DBG_LED  DDRD
#define PORT_DBG_LED PORTD
#define PIN_DBG_LED  (1 << PD1)

/********** Algorithm configuration parameters ***********/
/* 1-255 */
#define COLOR_MAX	200

/* In percents */
#define COLOR_CLEAR_PROBABILITY 35

/* In milliseconds */
#define MOOD_MODE_COLOR_DELAY         30
#define SOUND_LAMP_MODE_COLOR_DELAY   15

/* In seconds */
#define TIME_TO_HOLD_COLOR 10

/* Loud sound level */
#define LOUD_SOUND 200

/* Number of blinks when switching mode */
#define MODE_SWITCH_BLINKS 5

/* Size of the buffer for sound frequency analysis */
#define SOUND_BUF_SIZE 	128

/* Zero sound amplitude value. Defines which mic level is considered as silence. */
#define SOUND_AMPL_ZERO 100

/**********       Some macros            ***********/
#define SET_RED(value)   OCR_RED   = (value)
#define SET_GREEN(value) OCR_GREEN = (value)
#define SET_BLUE(value)  OCR_BLUE  = (value)

#define BTN_VALUE        ((PIN_REG_BTN & PIN_BTN) ? 0 : 1)

#define DBG_LED_ON       (PORT_DBG_LED &= ~(PIN_DBG_LED))
#define DBG_LED_OFF      (PORT_DBG_LED |=  (PIN_DBG_LED))

/**********       Modes            ***********/
typedef enum
{
	MoodLamp = 0,	/* Simple mood lamp mode */
	SoundLamp,		/* "Change color on loud sound" mode */
	SoundAnalysis,  /* Sound analysis mode */
	ModeLastInvalid
} Mode;

typedef void (*ModeCallback)(void);
typedef void (*ModeSwitchCallback)(void);

#endif /* LAMP_H */
