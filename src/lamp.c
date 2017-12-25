#include "lamp.h"

/* Global variables */
static volatile unsigned short 	mic_level	      = 0; /* Last mic level read from ADC */
static volatile unsigned char 	is_button_pressed = 0; /* 1 if button is pressed, 0 otherwise. */

/******************************************************************/
/******************** INITIALIZATION ******************************/
/******************************************************************/
#ifdef DBG_MODE
static void initialize_dbg_led(void)
{
	DDR_DBG_LED  |= PIN_DBG_LED;
	PORT_DBG_LED |= PIN_DBG_LED;
}

static void toggle_dbg_led(void)
{
	static char val = 0;

	if (!val)
		DBG_LED_ON;
	else
		DBG_LED_OFF;

	val ^= 1;
}
#endif /* DBG_MODE */

static void initialize_pwm(void)
{
	/* Initialize red channel */
	DDR_RED |= PIN_RED;
	TCCR_INIT_RED;
	SET_RED(0);

    /* Initialize green channel */
	DDR_GREEN |= PIN_GREEN;
	TCCR_INIT_GREEN;
	SET_GREEN(0);

    /* Initialize blue channel */
    DDR_BLUE |= PIN_BLUE;
	TCCR_INIT_BLUE;
	SET_BLUE(0);
}

static void initialize_adc(void)
{
    /* Select channel */
	ADMUX |= MIC_ADC_CHANNEL;

	/* Select AVCC as voltage reference */
	ADMUX |= (0 << REFS1) | (1 << REFS0);

	/* Enable adc. Prescaler = 128. */
	ADCSRA |= (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);
}

/******************************************************************/
/******************** HELPER FUNCTIONS ****************************/
/******************************************************************/
static void update_adc(void)
{
	/* Start conversion */
	ADCSRA |= (1 << ADSC);

	/* Wait until conversion is finished */
	while (ADCSRA & (1 << ADSC)) ;

	mic_level = ADCW;
}

static void check_btn(void)
{
	is_button_pressed = BTN_VALUE;
}

/*
 * This functions delays execution for approximately ms milliseconds.
 * It's not simple delay because we have to check if user pressed the button
 * and update current microphone level from ADC.
 *
 * Returns 1 if button was pressed, 0 otherwise.
 */
static unsigned char schedule_delay(unsigned char ms)
{
	do
	{
		for (unsigned int i = 0; i < 10; ++i)
			update_adc();
		
		check_btn();

		if (is_button_pressed)
			return 1;

	} while (--ms);

	return 0;
}

/*
 * Picks random color in RGB format.
 * This function may return "clean" color, i.e. red, green or blue.
 * How often "clean" colors will be picked is contrtolled with COLOR_CLEAR_PROBABILITY.
 */
static void pick_random_color(unsigned char* r, unsigned char* g, unsigned char* b)
{
	*r = rand() % COLOR_MAX; 
	*g = rand() % COLOR_MAX; 
	*b = rand() % COLOR_MAX; 

	if ((rand() % 100) < COLOR_CLEAR_PROBABILITY)
	{
		unsigned char *rgb[] = {r, g, b};
		unsigned char clean_color_idx = rand() % 3;
		
		++clean_color_idx;
		*(rgb[clean_color_idx % 3]) = 0;

		++clean_color_idx;
		*(rgb[clean_color_idx % 3]) = 0;
	}
}

/******************************************************************/
/******************** MOOD LAMP MODE ******************************/
/******************************************************************/
static void switch_to_mood_lamp_mode(void)
{
	/* Reset colors */
	SET_RED(0);
	SET_GREEN(0);
	SET_BLUE(0);

	/* Blink few times with blue */
	for (unsigned char i = 0; i < MODE_SWITCH_BLINKS; ++i)
	{
		SET_BLUE(COLOR_MAX);
		_delay_ms(15); _delay_ms(15); _delay_ms(15); _delay_ms(15); _delay_ms(15); _delay_ms(15);
		
		SET_BLUE(0);
		_delay_ms(15); _delay_ms(15); _delay_ms(15); _delay_ms(15); _delay_ms(15); _delay_ms(15);
	}
}

static void mood_lamp_mode(void)
{
	unsigned char red = 0, green = 0, blue = 0;
	unsigned char target_red, target_green, target_blue;
	
    do
	{
		pick_random_color(&target_red, &target_green, &target_blue);
		
		/* Smoothly switch to selected color */
		while (target_red != red || target_green != green || target_blue != blue)
		{
			if (red != target_red)
			{
				red += (target_red > red) ? 1 : -1;
				SET_RED(red);
			}

			if (green != target_green)
			{
				green += (target_green > green) ? 1 : -1;
				SET_GREEN(green);
			}

			if (blue != target_blue)
			{
				blue += (target_blue > blue) ? 1 : -1;
				SET_BLUE(blue);
			}

			/* Sleep some time */
			if (schedule_delay(MOOD_MODE_COLOR_DELAY))
				return; /* Return if button was pressed */
		}
		
		/* Hold picked color for a while */
		for (unsigned int i = 0; i < TIME_TO_HOLD_COLOR * 100; ++i)
		{
			if (schedule_delay(10))
				return;
		}
	} while(1);
}

/******************************************************************/
/******************** SOUND LAMP MODE *****************************/
/******************************************************************/
static void switch_to_sound_lamp_mode(void)
{
	/* Reset colors */
	SET_RED(0);
	SET_GREEN(0);
	SET_BLUE(0);

	/* Blink few times with red */
	for (unsigned char i = 0; i < MODE_SWITCH_BLINKS; ++i)
	{
		SET_RED(COLOR_MAX);
		_delay_ms(15); _delay_ms(15); _delay_ms(15); _delay_ms(15); _delay_ms(15); _delay_ms(15);
		
		SET_RED(0);
		_delay_ms(15); _delay_ms(15); _delay_ms(15); _delay_ms(15); _delay_ms(15); _delay_ms(15);
	}
}

static unsigned short sound_lamp_mode_get_mic(unsigned char r, unsigned char g, unsigned char b)
{
	unsigned short ret = 0;

	/* Turn off PWM to minimize noise on ADC */
	SET_RED(0); SET_GREEN(0); SET_BLUE(0);

	/* Run few ADC conversions for more precise result */
	update_adc();
	update_adc();
	update_adc();
	
	ret = mic_level;

	/* Restore PWM */
	SET_RED(r); SET_GREEN(g); SET_BLUE(b);

	return ret;
}

static void sound_lamp_mode(void)
{
	unsigned char red = 0, green = 0, blue = 0;
	unsigned char target_red, target_green, target_blue;
	
    do
	{

new_color_selection:

		pick_random_color(&target_red, &target_green, &target_blue);
		
		/* Smoothly switch to selected color */
		while (target_red != red || target_green != green || target_blue != blue)
		{
			if (red != target_red)
			{
				red += (target_red > red) ? 1 : -1;
				SET_RED(red);
			}

			if (green != target_green)
			{
				green += (target_green > green) ? 1 : -1;
				SET_GREEN(green);
			}

			if (blue != target_blue)
			{
				blue += (target_blue > blue) ? 1 : -1;
				SET_BLUE(blue);
			}

			if (schedule_delay(SOUND_LAMP_MODE_COLOR_DELAY))
				return; /* Return if button was pressed */
			
			/* If sound is loud enough */
			if (sound_lamp_mode_get_mic(red, green, blue) > LOUD_SOUND)
			{
#ifdef DBG_MODE
				DBG_LED_ON;
				_delay_ms(10);
				DBG_LED_OFF;
#endif /* DBG_MODE */

				/* Go and pick new color */
				goto new_color_selection;
			}
		}
		
		/* Stay in selected color untill loud sound detected */
		while (1)
		{
			if (schedule_delay(10))
				return; /* Return if button was pressed */

		    /* If sound is loud enough */
			if (sound_lamp_mode_get_mic(red, green, blue) > LOUD_SOUND)
			{
#ifdef DBG_MODE
				DBG_LED_ON;
				_delay_ms(10);
				DBG_LED_OFF;
#endif /* DBG_MODE */

				/* Go and pick new color */
				goto new_color_selection;
			}
		}
	} while(1);
}

/******************************************************************/
/******************** SOUND ANALYSIS MODE *************************/
/******************************************************************/
static void switch_to_sound_analysis_mode(void)
{
	/* Reset colors */
	SET_RED(0);
	SET_GREEN(0);
	SET_BLUE(0);

	/* Blink few times with green */
	for (unsigned char i = 0; i < MODE_SWITCH_BLINKS; ++i)
	{
		SET_GREEN(COLOR_MAX);
		_delay_ms(15); _delay_ms(15); _delay_ms(15); _delay_ms(15); _delay_ms(15); _delay_ms(15);
		
		SET_GREEN(0);
		_delay_ms(15); _delay_ms(15); _delay_ms(15); _delay_ms(15); _delay_ms(15); _delay_ms(15);
	}
}

static void convert_freq_to_rgb(float freq, unsigned char* r, unsigned char* g, unsigned char* b)
{
	unsigned short H = 359 * freq * 2; /* Only hue depends on frequency */
	unsigned char  S = 255;
	unsigned char  V = 255;

	while (H >= 360)
		H -= 360;
	
	/* HSV -> RGB conversion */
	unsigned char Hi = H/60;
	float f = (float)H / 60.0 - Hi;
	unsigned char p = V * (255 - S) / 255;
	unsigned char q = V * (255 - f * S) / 255;
	unsigned char t = V * (255 - (1.0 - f) * S) / 255;

	switch (Hi)
	{
		case 0:
			*r = V; *g = t; *b = p;
			break;
		case 1:
			*r = q; *g = V; *b = p;
			break;
		case 2:
			*r = p; *g = V; *b = t;
			break;
		case 3:
			*r = p; *g = q; *b = V;
			break;
		case 4:
			*r = t; *g = p; *b = V;
			break;
		case 5:
			*r = V; *g = p; *b = q;
			break;
	}
}

static void sound_analysis_mode(void)
{
	unsigned short buf[SOUND_BUF_SIZE] = {0};
	unsigned freq;
	unsigned char r = 0, g = 0, b = 0;

	while (1)
	{
		/* Fill the buffer with sound */
		for (unsigned short i = 0; i < SOUND_BUF_SIZE; ++i)
		{
			update_adc();
			update_adc();
			update_adc();
			
			buf[i] = mic_level;
	
			SET_RED(r);
			SET_GREEN(g);
			SET_BLUE(b);
	
			_delay_us(50);
		}

		/* Calculate approximate sound frequency */
		freq = 0;
		for (unsigned short i = 0; i < SOUND_BUF_SIZE - 1; ++i)
		{
			signed char s1 = (signed)SOUND_AMPL_ZERO - (signed)buf[i]     > 0 ? 1 : -1;
			signed char s2 = (signed)SOUND_AMPL_ZERO - (signed)buf[i + 1] > 0 ? 1 : -1;

			if (s1 != s2)
				++freq;			
		}

		/* Convert frequency to color */
		convert_freq_to_rgb((float)freq / SOUND_BUF_SIZE /* To [0, 1] range */, &r, &g, &b);	

		/* Set color */
		SET_RED(r);
		SET_GREEN(g);
		SET_BLUE(b);

		if (schedule_delay(5))
			return;
	}
}

/******************************************************************/
/************************** MAIN **********************************/
/******************************************************************/

/* Callback functions table for different modes */
static const ModeCallback 		mode_callbacks[] 		= { mood_lamp_mode, 		  sound_lamp_mode, 			 sound_analysis_mode };
static const ModeSwitchCallback mode_switch_callbacks[] = { switch_to_mood_lamp_mode, switch_to_sound_lamp_mode, switch_to_sound_analysis_mode };

int main()
{
	/* Simple mood lamp mode by default */
	Mode mode = MoodLamp;

	/* Initialize stuff */
#ifdef DBG_MODE
	initialize_dbg_led();
#endif /* DBG_MODE */
    initialize_pwm();
	initialize_adc();
  
	/* Get some input from microphone */
  	update_adc();
	
	/* Now use this input as seed for random number generator */
	srand(mic_level);

	/* Main program loop */
	while (1)
	{
		mode_callbacks[mode]();

		/* Switch to the next mode */
		mode = (mode + 1) % ModeLastInvalid;

		/* Let user know that we are changing mode (i.e. blink few times with the LED) */
		mode_switch_callbacks[mode]();
	}

	return 0;
}
