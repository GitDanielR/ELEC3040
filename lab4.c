#include "stm32l4xx.h"

#define TRUE 1
#define FALSE 0
#define OUTPUT_MODE 0b01
#define arr(...) (int[]){__VA_ARGS__}

int countA = 0;
int countB = 0;
int run = FALSE;
int up = TRUE;

int pin_mask(int two_bit_value, int shifts[], int size)
{
	two_bit_value &= 0b11;
	
	int result = 0;
	for (int i = 0; i < size; i++)
	{
		result |= (two_bit_value << shifts[i]*2);
	}
	return result;
}	

void pin_setup()
{
	RCC->AHB2ENR |= 0x03;		// GPIOA, GPIOB clock
	
	GPIOA->MODER &= ~(pin_mask(0b11, arr(1, 2, 5, 6, 7, 8, 9, 10, 11, 12), 10));	// clear
	GPIOA->MODER |=  (pin_mask(OUTPUT_MODE, arr(5, 6, 7, 8, 9, 10, 11, 12), 8));	// output
	
	GPIOB->MODER &= ~(pin_mask(0b11, arr(3, 4), 2));								// clear
	GPIOB->MODER |=  (pin_mask(OUTPUT_MODE, arr(3, 4), 2));							// output
}

void interrupt_setup()
{
	RCC->APB2ENR |= 0x01;

	// set PA1 & PA2 to handle EXTI1 & EXTI2
	SYSCFG->EXTICR[0] &= ~(SYSCFG_EXTICR1_EXTI1 	| SYSCFG_EXTICR1_EXTI2);
	SYSCFG->EXTICR[0] |=  (SYSCFG_EXTICR1_EXTI1_PA 	| SYSCFG_EXTICR1_EXTI2_PA);
	
	// EXTI1 & EXTI2 setup falling edge, unmask, and clear pending
	EXTI->FTSR1 |= (EXTI_FTSR1_FT1 	| EXTI_FTSR1_FT2); 
	EXTI->IMR1 	|= (EXTI_IMR1_IM1 	| EXTI_IMR1_IM2); 
	EXTI->PR1 	|= (EXTI_PR1_PIF1 	| EXTI_PR1_PIF2); 
	
	// enable & clear pending from NVIC
	NVIC_EnableIRQ(EXTI1_IRQn);
	NVIC_EnableIRQ(EXTI2_IRQn);
	NVIC_ClearPendingIRQ(EXTI1_IRQn);
	NVIC_ClearPendingIRQ(EXTI2_IRQn);
	
	__enable_irq();
}

void wrap(int *value, int up)
{
	*value = up ? *value+1 : *value-1;
	*value = *value > 9 ? 0 : *value < 0 ? 9 : *value;
}

void count()
{
	if (run == FALSE) 
	{
		return;
	}
	
	wrap(&countA, up);
	wrap(&countB, TRUE);
	GPIOA->ODR = (countA << 5 | countB << 9);
}

void delay()
{
	int j;
	//for (int i = 0; i < 110000*(countA+countB); i++)
	for (int i = 0; i < 220000; i++)
	{
		j = i;
	}
}

void reset_handler(int bit, int NVIC_flag)
{
	GPIOB->ODR = (run << 3 | up << 4);
	EXTI->PR1 |= (1 << bit);
	NVIC_ClearPendingIRQ(NVIC_flag);
}

// run button (EXTI1)
void EXTI1_IRQHandler()
{
	run = !run;
	reset_handler(1, EXTI1_IRQn);
}

// up button (EXTI2)
void EXTI2_IRQHandler()
{
	up = !up;
	reset_handler(2, EXTI2_IRQn);
}

int main()
{
	pin_setup();
	interrupt_setup();
	
	while (TRUE)
	{
		delay();
		count();
	}
}