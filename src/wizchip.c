#include "stm32f4xx.h"
#include "stm32f4xx_nucleo.h"
/* Includes ------------------------------------------------------------------*/
#include "wizchip_conf.h"
#include "DHCP/dhcp.h"
#include "socket.h"
#include <stm32f4xx_hal.h>
#define DHCP

extern void initialise_monitor_handles(void);
#define SEPARATOR            "=============================================\r\n"
#define WELCOME_MSG  		 "Welcome to STM32Nucleo Ethernet configuration\r\n"

#define PRINT_HEADER() 		\
  printf(SEPARATOR);		\
  printf(WELCOME_MSG);		\
  printf(SEPARATOR);


SPI_HandleTypeDef 	hspi2;
char msg[60];

//////////////////////////////////////////////////
// Socket & Port number definition for Examples //
//////////////////////////////////////////////////
#define SOCK_DHCP       6
#define SOCK_TCPS       0
#define PORT_TCPS		5000

#define DATA_BUF_SIZE   2048
uint8_t gDATABUF[DATA_BUF_SIZE];

////////////////
// DHCP client//
////////////////
#define MY_MAX_DHCP_RETRY			2
uint8_t my_dhcp_retry = 0;
static void Net_Conf();
void my_ip_assign(void);
void my_ip_conflict(void);
static void MX_GPIO_Init(void);
static void MX_SPI2_Init(void);

wiz_NetInfo gWIZNETINFO;

void cs_sel() {
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_RESET); //CS LOW
}

void cs_desel() {
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_SET); //CS HIGH
}

uint8_t spi_rb(void) {
	uint8_t rbuf;
	HAL_SPI_Receive(&hspi2, &rbuf, 1, 0xFFFFFFFF);
	return rbuf;
}

void spi_wb(uint8_t b) {
	HAL_SPI_Transmit(&hspi2, &b, 1, 0xFFFFFFFF);
}

/**
  * @brief  Main program
  * @param  None
  * @retval None
  */

uint8_t wizchipInit(wiz_NetInfo *wiznetInfo) {
	initialise_monitor_handles();
	uint8_t bufSize[] = {2, 2, 2, 2};
	HAL_Init();
	gWIZNETINFO = *wiznetInfo;
  /* Configure the System clock to 84 MHz */
	MX_GPIO_Init();
	MX_SPI2_Init();
	PRINT_HEADER();
	reg_wizchip_cs_cbfunc(cs_sel, cs_desel);
	reg_wizchip_spi_cbfunc(spi_rb, spi_wb);

	wizchip_init(bufSize, bufSize);
	/* DHCP client Initialization */

	if(wiznetInfo->dhcp == NETINFO_DHCP) {
		DHCP_init(SOCK_DHCP, gDATABUF);
		// if you want different action instead default ip assign, update, conflict.
		// if cbfunc == 0, act as default.
		reg_dhcp_cbfunc(my_ip_assign, my_ip_assign, my_ip_conflict);
	} else {
		// Static
		wizchip_setnetinfo(wiznetInfo);
		wizchip_getnetinfo(wiznetInfo);
#ifdef _MAIN_DEBUG_
		Display_Net_Conf();
#endif
		return 1;
	}

	/* Main loop ***************************************/
	while(1) {
		/* DHCP IP allocation and check the DHCP lease time (for IP renewal) */
		if(wiznetInfo->dhcp == NETINFO_DHCP) {
			switch(DHCP_run()) {
				case DHCP_IP_ASSIGN:
				case DHCP_IP_CHANGED:
					/* If this block empty, act with default_ip_assign & default_ip_update */
					//
					// This example calls my_ip_assign in the two case.
					//
					// Add to ...
					//
					break;
				case DHCP_IP_LEASED:
					return 1;
					break;
				case DHCP_FAILED:
					/* ===== Example pseudo code =====  */
					// The below code can be replaced your code or omitted.
					// if omitted, retry to process DHCP
					my_dhcp_retry++;
					if(my_dhcp_retry > MY_MAX_DHCP_RETRY) {
						wiznetInfo->dhcp = NETINFO_STATIC;
						DHCP_stop();      // if restart, recall DHCP_init()
#ifdef _MAIN_DEBUG_
						printf(">> DHCP %d Failed\r\n", my_dhcp_retry);
						Net_Conf();
						Display_Net_Conf();   // print out static netinfo to serial
#endif
						my_dhcp_retry = 0;
						return 1;
					}
					break;
				default:
					break;
			}
		}
	} // End of Main loop
}

void MX_GPIO_Init(void) {

	GPIO_InitTypeDef GPIO_InitStruct;

	/* GPIO Ports Clock Enable */
	__GPIOC_CLK_ENABLE()
	;
	__GPIOH_CLK_ENABLE()
	;
	__GPIOA_CLK_ENABLE()
	;
	__GPIOB_CLK_ENABLE()
	;

	/* Configure GPIO pin : PC13 */
	GPIO_InitStruct.Pin = GPIO_PIN_13;
	GPIO_InitStruct.Mode = GPIO_MODE_EVT_RISING;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

	/* Configure GPIO pin : PA5 */
	GPIO_InitStruct.Pin = GPIO_PIN_5;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_LOW;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
}

/* SPI2 init function */
void MX_SPI2_Init(void) {
	GPIO_InitTypeDef GPIO_InitStruct;

	hspi2.Instance = SPI2;
	hspi2.Init.Mode = SPI_MODE_MASTER;
	hspi2.Init.Direction = SPI_DIRECTION_2LINES;
	hspi2.Init.DataSize = SPI_DATASIZE_8BIT;
	hspi2.Init.CLKPolarity = SPI_POLARITY_LOW;
	hspi2.Init.CLKPhase = SPI_PHASE_1EDGE;
	hspi2.Init.NSS = SPI_NSS_SOFT;
	hspi2.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_4;
	hspi2.Init.FirstBit = SPI_FIRSTBIT_MSB;
	hspi2.Init.TIMode = SPI_TIMODE_DISABLED;
	hspi2.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLED;

    __SPI2_CLK_ENABLE();
    /**SPI2 GPIO Configuration
    PB12     ------> SPI2_NSS
    PB13     ------> SPI2_SCK
    PB14     ------> SPI2_MISO
    PB15     ------> SPI2_MOSI
    */
    GPIO_InitStruct.Pin = GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_15;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF5_SPI2;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /*Configure GPIO pin : PA5 */
    GPIO_InitStruct.Pin = GPIO_PIN_12;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_LOW;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

	HAL_SPI_Init(&hspi2);
}



/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t* file, uint32_t line) {
	 printf("Error on file %s on line %ld\r\n", file, line);

	/* Infinite loop */
	while (1) {
	}
}

static void Net_Conf() {
	/* wizchip netconf */
	ctlnetwork(CN_SET_NETINFO, (void*) &gWIZNETINFO);
}
/*******************************************************
 * @ brief Call back for ip assing & ip update from DHCP
 *******************************************************/
void my_ip_assign(void) {
	getIPfromDHCP(gWIZNETINFO.ip);
	getGWfromDHCP(gWIZNETINFO.gw);
	getSNfromDHCP(gWIZNETINFO.sn);
	getDNSfromDHCP(gWIZNETINFO.dns);
	gWIZNETINFO.dhcp = NETINFO_DHCP;
	/* Network initialization */
	Net_Conf();      // apply from DHCP
#ifdef _MAIN_DEBUG_
	Display_Net_Conf();
	printf("DHCP LEASED TIME : %ld Sec.\r\n", getDHCPLeasetime());
	printf("\r\n");
#endif
}

/************************************
 * @ brief Call back for ip Conflict
 ************************************/
void my_ip_conflict(void) {
#ifdef _MAIN_DEBUG_
	printf("CONFLICT IP from DHCP\r\n");
#endif
	//halt or reset or any...
	while(1); // this example is halt.
}
