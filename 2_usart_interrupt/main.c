#include <stdint.h>
#include "reg.h"
#include "blink.h"

void init_usart1(void)
{
	//PB6->TX PB7->RX
	//RCC EN GPIOB
	SET_BIT(RCC_BASE + RCC_AHB1ENR_OFFSET, GPIO_EN_BIT(GPIO_PORTB));

	//GPIO Configurations->10/0/01/00
	//MODER->10
	SET_BIT(GPIO_BASE(GPIO_PORTB) + GPIOx_MODER_OFFSET, MODERy_1_BIT(6));
	CLEAR_BIT(GPIO_BASE(GPIO_PORTB) + GPIOx_MODER_OFFSET, MODERy_0_BIT(6));
	
	SET_BIT(GPIO_BASE(GPIO_PORTB) + GPIOx_MODER_OFFSET, MODERy_1_BIT(7));
	CLEAR_BIT(GPIO_BASE(GPIO_PORTB) + GPIOx_MODER_OFFSET, MODERy_0_BIT(7));
	
	//OTY->0
	CLEAR_BIT(GPIO_BASE(GPIO_PORTB) + GPIOx_OTYPER_OFFSET, OTy_BIT(6));
	CLEAR_BIT(GPIO_BASE(GPIO_PORTB) + GPIOx_OTYPER_OFFSET, OTy_BIT(7));
	
	//OSPEED->01
	CLEAR_BIT(GPIO_BASE(GPIO_PORTB) + GPIOx_OSPEEDR_OFFSET, OSPEEDRy_1_BIT(6));
	SET_BIT(GPIO_BASE(GPIO_PORTB) + GPIOx_OSPEEDR_OFFSET, OSPEEDRy_0_BIT(6));
	
	CLEAR_BIT(GPIO_BASE(GPIO_PORTB) + GPIOx_OSPEEDR_OFFSET, OSPEEDRy_1_BIT(7));
	SET_BIT(GPIO_BASE(GPIO_PORTB) + GPIOx_OSPEEDR_OFFSET, OSPEEDRy_0_BIT(7));
	
	//PUPDR->00
	CLEAR_BIT(GPIO_BASE(GPIO_PORTB) + GPIOx_PUPDR_OFFSET, PUPDRy_1_BIT(6));
	CLEAR_BIT(GPIO_BASE(GPIO_PORTB) + GPIOx_PUPDR_OFFSET, PUPDRy_0_BIT(6));

	CLEAR_BIT(GPIO_BASE(GPIO_PORTB) + GPIOx_PUPDR_OFFSET, PUPDRy_1_BIT(7));
	CLEAR_BIT(GPIO_BASE(GPIO_PORTB) + GPIOx_PUPDR_OFFSET, PUPDRy_0_BIT(7));
	
	//AF SEL
	WRITE_BITS(GPIO_BASE(GPIO_PORTB) + GPIOx_AFRL_OFFSET,
				AFRLy_3_BIT(6), AFRLy_0_BIT(6), 7);
	WRITE_BITS(GPIO_BASE(GPIO_PORTB) + GPIOx_AFRL_OFFSET,
				AFRLy_3_BIT(7), AFRLy_0_BIT(7), 7);
				
	//RCC EN USART1
	SET_BIT(RCC_BASE + RCC_APB2ENR_OFFSET, USART1EN);

	//Baud
	const unsigned int BAUD = 115200;
	const unsigned int SYSCLK_MHZ = 168;
	const double USARTDIV = SYSCLK_MHZ * 1.0e6 / 16 / BAUD;

	const uint32_t DIV_MANTISSA = (uint32_t)(USARTDIV);
	const uint32_t DIV_FRACTION = (uint32_t)((USARTDIV - DIV_MANTISSA) * 16);
	
	WRITE_BITS(USART1_BASE + USART_BRR_OFFSET, DIV_MANTISSA_11_BIT, DIV_MANTISSA_0_BIT, DIV_MANTISSA);
	WRITE_BITS(USART1_BASE + USART_BRR_OFFSET, DIV_FRACTION_3_BIT, DIV_FRACTION_0_BIT, DIV_FRACTION);

	//USART  Configurations
	//usart1 enable
	SET_BIT(USART1_BASE + USART_CR1_OFFSET, UE_BIT);
	//set TE
	SET_BIT(USART1_BASE + USART_CR1_OFFSET, TE_BIT);
	//set RE
	SET_BIT(USART1_BASE + USART_CR1_OFFSET, RE_BIT);
}

void usart1_send_char(const char ch)
{
	//wait util TXE==1
	while (!READ_BIT(USART1_BASE + USART_SR_OFFSET, TXE_BIT))
		;
	
	REG(USART1_BASE + USART_DR_OFFSET) = ch;
}

char usart1_receive_char(void)
{
	//wait util RXNE==1
	while (!READ_BIT(USART1_BASE + USART_SR_OFFSET, RXNE_BIT))
		;
	
	return (char)REG(USART1_BASE + USART_DR_OFFSET);
}

int main(void)
{
	init_usart1();

	char *hello = "Hello world!\r\n";

	//send Hello world
	while (*hello != '\0')
		usart1_send_char(*hello++);
	//Interrupt set
	SET_BIT(USART1_BASE + USART_CR1_OFFSET, RXNEIE_BIT);
	//NVIC Configurations
	SET_BIT(NVIC_ISER_BASE + NVIC_ISERn_OFFSET(1), 5);//IRQ37
	
	blink(LED_BLUE);
}

void usart1_handler(void)
{
	//receive char and resend it
	char ch;
	if(!READ_BIT(USART1_BASE + USART_SR_OFFSET, ORE_BIT)){
		ch = usart1_receive_char();
		if (ch == '\r')
			usart1_send_char('\n');

		usart1_send_char(ch);
		blink_count(LED_RED, 2);
	}
	else//ORE occur
	{
		char *err = "ERROR\r\n";
		while (*err != '\0')
			usart1_send_char(*err++);
		WRITE_BITS(USART1_BASE + USART_DR_OFFSET, 8, 0, 0);
		blink_count(LED_GREEN,5);
		WRITE_BITS(USART1_BASE + USART_DR_OFFSET, 8, 0, '\n');
	}
}
