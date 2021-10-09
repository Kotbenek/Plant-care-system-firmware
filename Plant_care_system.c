//4.096MHz crystal
#define F_CPU 4096000

#include <avr/io.h>
#include <avr/interrupt.h>

//7-segment display digit macros
#define DISPLAY_0          PORTD =  0b00010100
#define DISPLAY_1          PORTD =  0b01110111
#define DISPLAY_2          PORTD =  0b01001100
#define DISPLAY_3          PORTD =  0b01000101
#define DISPLAY_4          PORTD =  0b00100111
#define DISPLAY_5          PORTD =  0b10000101
#define DISPLAY_6          PORTD =  0b10000100
#define DISPLAY_7          PORTD =  0b01010111
#define DISPLAY_8          PORTD =  0b00000100
#define DISPLAY_9          PORTD =  0b00000101
#define DISPLAY_CLEAR      PORTD =  0b11111111
#define DISPLAY_SHOW_DOT   PORTD &= 0b11111011

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
volatile uint8_t display_1 = 10;
volatile uint8_t display_2 = 10;
volatile uint8_t display_3 = 10;
volatile uint8_t display_4 = 10;

//Vcc measuring
#define VCC_VALUES_SAMPLES 128
uint16_t Vcc_values[VCC_VALUES_SAMPLES];
volatile uint16_t Vcc_value = 0;
uint32_t Vcc_value_temp = 0;
uint16_t ADC_result = 0;
uint8_t Vcc_values_index = 0;
uint8_t Vcc_value_valid = 0;

//Menu
//LED_2 LED_1 LED_0 - description
//000 - displaying Vcc ( V.vv)
//001 - time setting (HH:MM)
//010 - watering time setting (DD HH) - DD is every X days, HH is the hour of watering
//011 - watering duration setting (SSS.f) - SSS is seconds, f is s/10
//100 - lamp time on - (HH:MM)
//101 - lamp time off - (HH:MM)
//110 - displaying time - (HH:MM)
//111 - displaying watering countdown - (HH:MM)
volatile uint8_t menu_option = 0;
volatile uint8_t menu_button_delay_counter = 0;
volatile uint8_t display_vcc_delay_counter = 32; //Initial delay of 32 * 4 = 128ms

//System variables
volatile uint8_t time_hour = 0;
volatile uint8_t time_minute = 0;
volatile uint8_t time_second = 0;
volatile uint8_t time_4_milliseconds = 0;

//7-segment display functions

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

//Menu display functions

inline void display_vcc()
{
	display_vcc_delay_counter--;	
	if (display_vcc_delay_counter == 0)
	{
		display_1 = 10;
		display_2 = Vcc_value / 1000 % 10;
		display_3 = Vcc_value / 100 % 10;
		display_4 = Vcc_value  / 10 % 10;

		//125 * 4ms = 500ms delay
		display_vcc_delay_counter = 125;
	}
}

inline void display_time()
{
	display_1 = time_hour / 10 % 10;
	display_2 = time_hour % 10;
	display_3 = time_minute / 10 % 10;
	display_4 = time_minute % 10;
}

inline void display_watering_time_setting()
{
	//TODO
}

inline void display_watering_duration_setting()
{
	//TODO
}

inline void display_lamp_time_on()
{
	//TODO
}

inline void display_lamp_time_off()
{
	//TODO
}

inline void display_watering_countdown()
{
	//TODO
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
			//Safe copy and clear flag
			cli();
			ADC_result = ((uint16_t)ADCL + ((uint16_t)(ADCH) << 8));
			ADCSRA |= (1 << ADIF);
			sei();
			
			//Calculate Vcc sample and store it in array
			Vcc_values[Vcc_values_index++] = 1230L * 1024 / ADC_result;
			
			//Circular buffer
			//If the end of array is reached, go back to the beginning
			if (Vcc_values_index == VCC_VALUES_SAMPLES)
			{
				Vcc_values_index = 0;
			}
			
			//Calculate Vcc by averaging the samples
			Vcc_value_temp = 0;
			for (uint8_t i = 0; i < VCC_VALUES_SAMPLES; i++)
			{
				Vcc_value_temp += Vcc_values[i];
			}
			Vcc_value_temp /= VCC_VALUES_SAMPLES;

			//Safe copy and set flag
			cli();
			Vcc_value = (uint16_t)Vcc_value_temp;
			if (Vcc_values_index == 0) Vcc_value_valid = 1;
			sei();		
	    }

		//Menu button
		if (!(PINA & (1<<PA4)))
		{
			//Safe menu option change
			cli();
			menu_option++;
			if (menu_option == 8) menu_option = 0;
			display_vcc_delay_counter = 1;
			sei();

			//125 * 4ms = 500ms delay
			menu_button_delay_counter = 125;
			while (menu_button_delay_counter);
		}
		//H+ button
		if (!(PINA & (1<<PA0)))
		{
			//Safe menu option change
			cli();
			if (menu_option == 1)
			{
				time_hour++;
				if (time_hour == 24) time_hour = 0;
			}
			sei();

			//50 * 4ms = 200ms delay
			menu_button_delay_counter = 50;
			while (menu_button_delay_counter);
		}
		//H- button
		if (!(PINA & (1<<PA1)))
		{
			//Safe menu option change
			cli();
			if (menu_option == 1)
			{
				time_hour--;
				if (time_hour == 255) time_hour = 23;
			}
			sei();

			//50 * 4ms = 200ms delay
			menu_button_delay_counter = 50;
			while (menu_button_delay_counter);
		}
		//M+ button
		if (!(PINA & (1<<PA2)))
		{
			//Safe menu option change
			cli();
			if (menu_option == 1)
			{
				time_minute++;
				if (time_minute == 60) time_minute = 0;
				time_second = 0;
				time_4_milliseconds = 0;
			}
			sei();

			//50 * 4ms = 200ms delay
			menu_button_delay_counter = 50;
			while (menu_button_delay_counter);
		}
		//M- button
		if (!(PINA & (1<<PA3)))
		{
			//Safe menu option change
			cli();
			if (menu_option == 1)
			{
				time_minute--;
				if (time_minute == 255) time_minute = 59;
				time_second = 0;
				time_4_milliseconds = 0;
			}
			sei();

			//50 * 4ms = 200ms delay
			menu_button_delay_counter = 50;
			while (menu_button_delay_counter);
		}
	}
}

//250Hz interrupt
ISR(TIMER0_OVF_vect)
{
	//7-segment display - turn off
	DISPLAY_DIGIT_NONE;

	//Menu display handling
	if (menu_option == 0)
	{
		display_vcc();
	}
	else if (menu_option == 1)
	{
		display_time();
	}
	else if (menu_option == 2)
	{
		display_watering_time_setting();
	}
	else if (menu_option == 3)
	{
		display_watering_duration_setting();
	}
	else if (menu_option == 4)
	{
		display_lamp_time_on();
	}
	else if (menu_option == 5)
	{
		display_lamp_time_off();
	}
	else if (menu_option == 6)
	{
		display_time();
	}
	else if (menu_option == 7)
	{
		display_watering_countdown();
	}

	//7-segment display - set digit
	display(display_digit());
	
	//Display dot according to the selected menu option
	if (menu_option == 0)
	{
		if (display_n == 1) DISPLAY_SHOW_DOT;
	}
	else if (menu_option == 1)
	{
		if (display_n == 2) DISPLAY_SHOW_DOT;
	}
	else if (menu_option == 2)
	{
		//No dot
	}
	else if (menu_option == 3)
	{
		if (display_n == 3) DISPLAY_SHOW_DOT;
	}
	else if (menu_option == 4)
	{
		if (display_n == 2) DISPLAY_SHOW_DOT;
	}
	else if (menu_option == 5)
	{
		if (display_n == 2) DISPLAY_SHOW_DOT;
	}
	else if (menu_option == 6)
	{
		if (display_n == 2) DISPLAY_SHOW_DOT;
	}
	else if (menu_option == 7)
	{
		if (display_n == 2) DISPLAY_SHOW_DOT;
	}

	//7-segment display - turn on digit
	if      (display_n == 1) DISPLAY_DIGIT1;
	else if (display_n == 2) DISPLAY_DIGIT2;
	else if (display_n == 3) DISPLAY_DIGIT3;
	else if (display_n == 4) DISPLAY_DIGIT4;

	//7-segment display - move to next digit
	display_n++;
	if (display_n == 5) display_n = 1;
	
	//Decrement delay counter if it's greater than zero
	if (menu_button_delay_counter) menu_button_delay_counter--;

	//Menu option LEDs
	if (menu_option == 0)
	{
		LED2_OFF;
		LED1_OFF;
		LED0_OFF;
	}
	else if (menu_option == 1)
	{
		LED2_OFF;
		LED1_OFF;
		LED0_ON;
	}
	else if (menu_option == 2)
	{
		LED2_OFF;
		LED1_ON;
		LED0_OFF;
	}
	else if (menu_option == 3)
	{
		LED2_OFF;
		LED1_ON;
		LED0_ON;
	}
	else if (menu_option == 4)
	{
		LED2_ON;
		LED1_OFF;
		LED0_OFF;
	}
	else if (menu_option == 5)
	{
		LED2_ON;
		LED1_OFF;
		LED0_ON;
	}
	else if (menu_option == 6)
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

	//Timekeeping
	time_4_milliseconds++;
	if (time_4_milliseconds == 250)
	{
		time_4_milliseconds = 0;
		time_second++;
		if (time_second == 60)
		{
			time_second = 0;
			time_minute++;
			if (time_minute == 60)
			{
				time_minute = 0;
				time_hour++;
				if (time_hour == 24)
				{
					time_hour = 0;
				}
			}
		}
	}
}
