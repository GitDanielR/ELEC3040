#include "stm32l4xx.h"

#define arr(...) (int[]){_VA__ARGS_}
#define FALSE 0
#define TRUE 1

int counter = 0;
int keypad_value = 0;
int keypad_map[4][4] = {
	{0x1,0x2,0x3,0xA},
	{0x4,0x5,0x6,0xB},
	{0x7,0x8,0x9,0xC},
	{0xE,0x0,0xF,0xD}
};

int bitmask(int selection, int shifts[], int size)
{
	selection &= 0b11;
	
	int result = 0;
	for (int i = 0; i < size; i++)
	{
		result |= (selection << (shifts[i] * 2));
	}
	return result;
}

void keypad_mode(int columns_output)
{
	int selected_pins[4] = {2,3,4,5};
	int non_selected_pins[4] = {8,9,10,11};
	if (columns_output)
	{
		for (int i = 0; i < 4; i++)
		{
			selected_pins[i] += 6;
			non_selected_pins[i] -= 6;
		}
	}
	
	// setup the driven output
	GPIOA->MODER &= ~(bitmask(0b11, arr(2,3,4,5,8,9,10,11), 8));
	GPIOA->MODER |=  (bitmask(0b01, selected_pins, 4));
	
	// setup pullup resistors on read lines
	GPIOA->PUPDR &= ~(bitmask(0b11, arr(2,3,4,5,8,9,10,11), 8));
	GPIOA->PUPDR |=  (bitmask(0b01, non_selected_pins, 4));
}

void pin_setup()
{
	RCC->AHB2ENR |= 0x03;
	
	// columns output = TRUE
	keypad_mode(TRUE);
	
	// display output
	GPIOB->MODER &= ~(bitmask(0b11, arr(0,3,4,5,6), 5));
	GPIOB->MODER |=  (bitmask(0b01, arr(3,4,5,6), 4));
}

void interrupt_setup()
{
	RCC->APB2ENR |= 0x01;
	
	SYSCFG->EXTICR[0] &= ~(SYSCFG_EXTICR1_EXTI0);
	SYSCFG->EXTICR[0] |=  (SYSCFG_EXTICR1_EXTI0_PB);
	
	EXTI->FTSR1 |= (EXTI_FTSR1_FT0); 
	EXTI->IMR1 	|= (EXTI_IMR1_IM0); 
	EXTI->PR1 	|= (EXTI_PR1_PIF0);
	
	NVIC_EnableIRQ(EXTI0_IRQn);
	NVIC_ClearPendingIRQ(EXTI0_IRQn);
	
	__enable_irq();
}

void display(int *value_to_display)
{
	GPIOB->ODR = (*value_to_display << 3);
}

void count()
{
	counter = counter==9 ? 0 : counter+1;
	display(&counter);
}

void delay(int milliseconds)
{
	int j;
	for (int i = 0; i < 217*(milliseconds); i++)
	{
		j=i;
	}
}

// takes 4 bit value and returns index of 0
int get_decimal_value(int value)
{
	value &= 0b1111;
	for (int i = 0; i < 4; i++)
	{
		if ((value & (1 << i)) == FALSE)
		{
			return i;
		}
	}
	return -1;	// should never happen
}

void EXTI0_IRQHandler()
{
	// drive columns, read rows (2:5)
	delay(3);
	int row = get_decimal_value(GPIOA->IDR >> 2);
	
	// drive rows, read columns (8:11)
	keypad_mode(FALSE);
	delay(3);
	int column = get_decimal_value(GPIOA->IDR >> 8);
	
	keypad_value = keypad_map[row][column];
	display(&keypad_value);
	delay(3000);
	
	keypad_mode(TRUE);	// reset keypad to column mode driving
	EXTI->PR1 |= 1;
	NVIC_ClearPendingIRQ(EXTI0_IRQn);
}

int main()
{
	pin_setup();
	interrupt_setup();
	GPIOA->ODR = 0; 
	
	while (TRUE)
	{
		count();
		delay(1000);
	}
}