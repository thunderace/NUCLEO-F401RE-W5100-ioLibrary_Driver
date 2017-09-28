#include "stm32f4xx.h"
#include "stm32f4xx_nucleo.h"
//#include <stdio.h>
//#include <stdlib.h>
extern void initialise_monitor_handles(void);
/* Includes ------------------------------------------------------------------*/
#include "wizchip_conf.h"
#include "DHCP/dhcp.h"
#include "socket.h"
#include <stm32f4xx_hal.h>
#include <string.h>
#define DHCP
#define SEPARATOR            "=============================================\r\n"
#define WELCOME_MSG  		 "Welcome to STM32Nucleo Ethernet configuration\r\n"
#define NETWORK_MSG  		 "Network configuration:\r\n"
#define IP_MSG 		 		 "  IP ADDRESS:  %d.%d.%d.%d\r\n"
#define NETMASK_MSG	         "  NETMASK:     %d.%d.%d.%d\r\n"
#define GW_MSG 		 		 "  GATEWAY:     %d.%d.%d.%d\r\n"
#define MAC_MSG		 		 "  MAC ADDRESS: %x:%x:%x:%x:%x:%x\r\n"
#define GREETING_MSG 		 "Well done guys! Welcome to the IoT world. Bye!\r\n"
#define CONN_ESTABLISHED_MSG "Connection established with remote IP: %d.%d.%d.%d:%d\r\n"
#define SENT_MESSAGE_MSG	 "Sent a message. Let's close the socket!\r\n"
#define WRONG_RETVAL_MSG	 "Something went wrong; return value: %d\r\n"
#define WRONG_STATUS_MSG	 "Something went wrong; STATUS: %d\r\n"
#define LISTEN_ERR_MSG		 "LISTEN Error!\r\n"

#define PRINT_HEADER() 		\
  printf(SEPARATOR);		\
  printf(WELCOME_MSG);		\
  printf(SEPARATOR);

#define PRINT_NETINFO(netInfo)  																					\
  printf(NETWORK_MSG);											\
  printf("%s %0x:%0x:%0x:%0x:%0x:%0x\n", MAC_MSG, netInfo.mac[0], netInfo.mac[1], netInfo.mac[2], netInfo.mac[3], netInfo.mac[4], netInfo.mac[5]);\
  printf("%s %02d.%02d.%02d.%02d\n", IP_MSG, netInfo.ip[0], netInfo.ip[1], netInfo.ip[2], netInfo.ip[3]);										\
  printf("%s %02d.%02d.%02d.%02d\n", NETMASK_MSG, netInfo.sn[0], netInfo.sn[1], netInfo.sn[2], netInfo.sn[3]);								\
  printf("%s %02d.%02d.%02d.%02d\n", GW_MSG, netInfo.gw[0], netInfo.gw[1], netInfo.gw[2], netInfo.gw[3]);

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
wiz_NetInfo gWIZNETINFO = {	.mac 	= {0x00, 0x08, 0xdc, 0xab, 0xcd, 0xef},	// Mac address
        					.ip 	= {192, 168, 1, 5},						// IP address
							.sn 	= {255, 255, 255, 0},					// Subnet mask
							.gw 	= {192, 168, 1, 1}};						// gateway
static void SystemClock_Config(void);
static void Error_Handler(void);
static void MX_GPIO_Init(void);
static void MX_SPI2_Init(void);


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

int main(void) {
	initialise_monitor_handles();
	unsigned char run_user_applications = 0;
	int8_t retVal, sockStatus;
	uint8_t bufSize[] = {2, 2, 2, 2};
	HAL_Init();

  /* Configure the System clock to 84 MHz */
	SystemClock_Config();

	MX_GPIO_Init();
	MX_SPI2_Init();
	PRINT_HEADER();
	//printf(WELCOME_MSG);
	reg_wizchip_cs_cbfunc(cs_sel, cs_desel);
	reg_wizchip_spi_cbfunc(spi_rb, spi_wb);

	wizchip_init(bufSize, bufSize);
	/* DHCP client Initialization */
#ifdef DHCP
	gWIZNETINFO.dhcp = NETINFO_DHCP;
#endif

	if(gWIZNETINFO.dhcp == NETINFO_DHCP) {
		DHCP_init(SOCK_DHCP, gDATABUF);
		// if you want different action instead default ip assign, update, conflict.
		// if cbfunc == 0, act as default.
		reg_dhcp_cbfunc(my_ip_assign, my_ip_assign, my_ip_conflict);
		run_user_applications = 0; 	// flag for running user's code
	} else {
		// Static
		wizchip_setnetinfo(&gWIZNETINFO);
		wizchip_getnetinfo(&gWIZNETINFO);
#ifdef _MAIN_DEBUG_
		Display_Net_Conf();
#endif
		run_user_applications = 1; 	// flag for running user's code
	}

	/* Main loop ***************************************/
	while(1) {
		/* DHCP IP allocation and check the DHCP lease time (for IP renewal) */
		if(gWIZNETINFO.dhcp == NETINFO_DHCP) {
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
					run_user_applications = 1;
					break;
				case DHCP_FAILED:
					/* ===== Example pseudo code =====  */
					// The below code can be replaced your code or omitted.
					// if omitted, retry to process DHCP
					my_dhcp_retry++;
					if(my_dhcp_retry > MY_MAX_DHCP_RETRY) {
						gWIZNETINFO.dhcp = NETINFO_STATIC;
						DHCP_stop();      // if restart, recall DHCP_init()
#ifdef _MAIN_DEBUG_
						printf(">> DHCP %d Failed\r\n", my_dhcp_retry);
						Net_Conf();
						Display_Net_Conf();   // print out static netinfo to serial
#endif
						my_dhcp_retry = 0;
						run_user_applications = 0;
					}
					break;
				default:
					break;
			}
		}

		while (run_user_applications) {
			PRINT_NETINFO(gWIZNETINFO);
			if((retVal = socket(0, Sn_MR_TCP, 5000, 0)) == 0) {
				/* Put socket in LISTEN mode. This means we are creating a TCP server */
				if((retVal = listen(0)) == SOCK_OK) {
					/* While socket is in LISTEN mode we wait for a remote connection */
					while((sockStatus = getSn_SR(0)) == SOCK_LISTEN) {
						HAL_Delay(100);
					}
					/* OK. Got a remote peer. Let's send a message to it */
					while(1) {
						/* If connection is ESTABLISHED with remote peer */
						if((sockStatus = getSn_SR(0)) == SOCK_ESTABLISHED) {
							uint8_t remoteIP[4];
							uint16_t remotePort;
							/* Retrieving remote peer IP and port number */
							getsockopt(0, SO_DESTIP, remoteIP);
							getsockopt(0, SO_DESTPORT, (uint8_t*)&remotePort);
							sprintf(msg, CONN_ESTABLISHED_MSG, remoteIP[0], remoteIP[1], remoteIP[2], remoteIP[3], remotePort);
							printf(msg);
							/* Let's send a welcome message and closing socket */
							if((retVal = send(0, (uint8_t *)GREETING_MSG, strlen(GREETING_MSG))) == (int16_t)strlen(GREETING_MSG)) {
								printf(SENT_MESSAGE_MSG);
							} else { /* Oops: something went wrong during data transfer */
								sprintf(msg, WRONG_RETVAL_MSG, retVal);
								printf(msg);
							}
							break;
						} else { /* Something went wrong with remote peer, maybe the connection was closed unexpectedly */
							sprintf(msg, WRONG_STATUS_MSG, sockStatus);
							printf(msg);
							break;
						}
					}
				} else  {/* Ops: socket not in LISTEN mode. Something went wrong */
					printf(LISTEN_ERR_MSG);
				}
			} else { /* Can't open the socket. This means something is wrong with W5100 configuration: maybe SPI issue? */
				sprintf(msg, WRONG_RETVAL_MSG, retVal);
				printf(msg);
			}

			/* We close the socket and start a connection again */
			disconnect(0);
			close(0);
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
  * @brief  System Clock Configuration
  *         The system Clock is configured as follow :
  *            System Clock source            = PLL (HSI)
  *            SYSCLK(Hz)                     = 84000000
  *            HCLK(Hz)                       = 84000000
  *            AHB Prescaler                  = 1
  *            APB1 Prescaler                 = 2
  *            APB2 Prescaler                 = 1
  *            HSI Frequency(Hz)              = 16000000
  *            PLL_M                          = 16
  *            PLL_N                          = 336
  *            PLL_P                          = 4
  *            PLL_Q                          = 7
  *            VDD(V)                         = 3.3
  *            Main regulator output voltage  = Scale2 mode
  *            Flash Latency(WS)              = 2
  * @param  None
  * @retval None
  */
static void SystemClock_Config(void) {
	RCC_ClkInitTypeDef RCC_ClkInitStruct;
	RCC_OscInitTypeDef RCC_OscInitStruct;

	/* Enable Power Control clock */
	__HAL_RCC_PWR_CLK_ENABLE();

	/* The voltage scaling allows optimizing the power consumption when the device is
		clocked below the maximum system frequency, to update the voltage scaling value
		regarding system frequency refer to product datasheet.  */
	__HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);

	/* Enable HSI Oscillator and activate PLL with HSI as source */
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
	RCC_OscInitStruct.HSIState = RCC_HSI_ON;
	RCC_OscInitStruct.HSICalibrationValue = 0x10;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
	RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
	RCC_OscInitStruct.PLL.PLLM = 16;
	RCC_OscInitStruct.PLL.PLLN = 336;
	RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
	RCC_OscInitStruct.PLL.PLLQ = 7;
	if(HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
		Error_Handler();
	}

	/* Select PLL as system clock source and configure the HCLK, PCLK1 and PCLK2
		clocks dividers */
	RCC_ClkInitStruct.ClockType = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2);
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
	RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
	RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
	if(HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK) {
		Error_Handler();
	}
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @param  None
  * @retval None
  */
static void Error_Handler(void) {
	/* User may add here some code to deal with this error */
	while(1) {
	}
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
