//4.096MHz crystal
#define F_CPU 4096000

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

//7-segment display digit macros
#define DISPLAY_0          PORTD = 0b00010100
#define DISPLAY_1          PORTD = 0b01110111
#define DISPLAY_2          PORTD = 0b01001100
#define DISPLAY_3          PORTD = 0b01000101
#define DISPLAY_4          PORTD = 0b00100111
#define DISPLAY_5          PORTD = 0b10000101
#define DISPLAY_6          PORTD = 0b10000100
#define DISPLAY_7          PORTD = 0b01010111
#define DISPLAY_8          PORTD = 0b00000100
#define DISPLAY_9          PORTD = 0b00000101
#define DISPLAY_CLEAR      PORTD = 0b11111111

#define DISPLAY_DIGIT1     PORTC |= (1 << 3)
#define DISPLAY_DIGIT2     PORTC |= (1 << 2)
#define DISPLAY_DIGIT3     PORTC |= (1 << 1)
#define DISPLAY_DIGIT4     PORTC |= (1 << 0)
#define DISPLAY_DIGIT_NONE PORTC &= 0b11110000

//LED macros
#define LED2_ON            PORTC |=  (1 << 4)
#define LED2_OFF           PORTC &= ~(1 << 4)
#define LED1_ON            PORTC |=  (1 << 5)
#define LED1_OFF           PORTC &= ~(1 << 5)
#define LED0_ON            PORTC |=  (1 << 6)
#define LED0_OFF           PORTC &= ~(1 << 6)

//Driver macros
#define LIGHT_ON           PORTB |=  (1 << 0)
#define LIGHT_OFF          PORTB &= ~(1 << 0)
#define PUMP_ON            PORTB |=  (1 << 1)
#define PUMP_OFF           PORTB &= ~(1 << 1)

//7-segment display variables
volatile uint8_t display_n = 1;
volatile uint8_t display_1 = 0;
volatile uint8_t display_2 = 0;
volatile uint8_t display_3 = 0;
volatile uint8_t display_4 = 0;

//Vcc measuring
#define VCC_VALUES_SAMPLES 64
uint16_t Vcc_values[VCC_VALUES_SAMPLES];
uint32_t Vcc_value = 0;
uint8_t Vcc_values_index = 0;
uint8_t Vcc_value_valid = 0;

//Debug - LED test
volatile uint16_t counter = 0;

//7-segment display functions

inline void display_number(uint16_t value)
{
	display_1 = value / 1000 % 10;
	display_2 = value / 100 % 10;
	display_3 = value / 10 % 10;
	display_4 = value % 10;
}

inline uint8_t display_digit()
{
	if (display_n == 1) return display_1;
	if (display_n == 2) return display_2;
	if (display_n == 3) return display_3;
	if (display_n == 4) return display_4;
	return 10;
}

inline void display(uint8_t digit)
{
	if (digit == 0) DISPLAY_0;
	else if (digit == 1) DISPLAY_1;
	else if (digit == 2) DISPLAY_2;
	else if (digit == 3) DISPLAY_3;
	else if (digit == 4) DISPLAY_4;
	else if (digit == 5) DISPLAY_5;
	else if (digit == 6) DISPLAY_6;
	else if (digit == 7) DISPLAY_7;
	else if (digit == 8) DISPLAY_8;
	else if (digit == 9) DISPLAY_9;
	else DISPLAY_CLEAR;
}

int main()
{
	//Port directions setup
	DDRB = 0b00000011;
	DDRC = 0b01111111;
	DDRD = 0b11111111;

	//Buttons pull-up
	PORTA = 0b00011111;

	//Timer0 prescaler 64
	TCCR0 |= (1<<CS01) | (1<<CS00);

	//Timer0 overflow interrupt enable
	TIMSK |= (1<<TOIE0); 

	//Enable interrupts
	sei();

	//Set Vref to AVcc
	ADMUX |= (1 << REFS0);

	//Set ADC input to 1.22V bandgap
    ADMUX |= (1 << MUX4) | (1 << MUX3) | (1 << MUX2) | (1 << MUX1);

	//Set ADC prescaler to 32
	ADCSRA |= (1 << ADPS2) | (1 << ADPS0);
	
	//ADC free running mode
	ADCSRA |= (1 << ADATE);

	//Enable ADC
	ADCSRA |= (1 << ADEN);

	//Start ADC free running conversion
	ADCSRA |= (1 << ADSC);

	while(1)
	{
		//Check if ADC conversion complete
	    if (ADCSRA & (1 << ADIF))
		{
			//Clear flag
			ADCSRA |= (1 << ADIF);			

			//Get ADC result, calculate Vcc sample and store it in array
			Vcc_values[Vcc_values_index++] = 1220L * 1023 / ((uint16_t)ADCL + ((uint16_t)(ADCH) << 8));

			//If the end of array is reached
			if (Vcc_values_index == VCC_VALUES_SAMPLES)
			{
				//Go back to the beginning
				Vcc_values_index = 0;

				//Set Vcc valid flag
				Vcc_value_valid = 1;
			}

			//Calculate Vcc by averaging the samples
			Vcc_value = 0;
			for (uint8_t i = 0; i < VCC_VALUES_SAMPLES; i++)
			{
				Vcc_value += Vcc_values[i];
			}
			Vcc_value /= VCC_VALUES_SAMPLES;

			//Debug - display Vcc
			if (Vcc_value_valid) display_number((uint16_t)Vcc_value);			
	    }

		//Menu button
		if (!(PINA & (1<<PA4)))
		{
			
		}
		//M+ button
		if (!(PINA & (1<<PA2)))
		{
			
		}
		//M- button
		if (!(PINA & (1<<PA3)))
		{
			
		}
		//H+ button
		if (!(PINA & (1<<PA0)))
		{
			
		}
		//H- button
		if (!(PINA & (1<<PA1)))
		{
			
		}
	}
}

//250Hz interrupt
ISR(TIMER0_OVF_vect)
{
	//7-segment display
	DISPLAY_DIGIT_NONE;

	display(display_digit());

	if      (display_n == 1) DISPLAY_DIGIT1;
	else if (display_n == 2) DISPLAY_DIGIT2;
	else if (display_n == 3) DISPLAY_DIGIT3;
	else if (display_n == 4) DISPLAY_DIGIT4;

	display_n++;
	if (display_n == 5) display_n = 1;

	//Debug - LED test
	counter += 4;
	if (counter == 8000) counter = 0;

	if (counter < 1000)
	{
		LED2_OFF;
		LED1_OFF;
		LED0_OFF;
	}
	else if (counter < 2000)
	{
		LED2_OFF;
		LED1_OFF;
		LED0_ON;
	}
	else if (counter < 3000)
	{
		LED2_OFF;
		LED1_ON;
		LED0_OFF;
	}
	else if (counter < 4000)
	{
		LED2_OFF;
		LED1_ON;
		LED0_ON;
	}
	else if (counter < 5000)
	{
		LED2_ON;
		LED1_OFF;
		LED0_OFF;
	}
	else if (counter < 6000)
	{
		LED2_ON;
		LED1_OFF;
		LED0_ON;
	}
	else if (counter < 7000)
	{
		LED2_ON;
		LED1_ON;
		LED0_OFF;
	}
	else
	{
		LED2_ON;
		LED1_ON;
		LED0_ON;
	}
}
