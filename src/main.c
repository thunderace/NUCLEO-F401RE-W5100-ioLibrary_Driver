#include "stm32f4xx.h"
#include "stm32f4xx_nucleo.h"
/* Includes ------------------------------------------------------------------*/
#include "wizchip.h"
#include "socket.h"

#include <stm32f4xx_hal.h>
#include <string.h>

extern void initialise_monitor_handles(void);
wiz_NetInfo wiznetInfo = {	.mac 	= {0x00, 0x08, 0xdc, 0xab, 0xcd, 0xef},	// Mac address
        					.ip 	= {192, 168, 1, 5},						// IP address
							.sn 	= {255, 255, 255, 0},					// Subnet mask
							.gw 	= {192, 168, 1, 1}};						// gateway
static void SystemClock_Config(void);

#define CONN_ESTABLISHED_MSG "Connection established with remote IP: %d.%d.%d.%d:%d\r\n"
#define SENT_MESSAGE_MSG	 "Sent a message. Let's close the socket!\r\n"
#define WRONG_RETVAL_MSG	 "Something went wrong; return value: %d\r\n"
#define WRONG_STATUS_MSG	 "Something went wrong; STATUS: %d\r\n"
#define LISTEN_ERR_MSG		 "LISTEN Error!\r\n"
#define GREETING_MSG 		 "Well done guys! Welcome to the IoT world. Bye!\r\n"
#define NETWORK_MSG  		 "Network configuration:\r\n"
#define IP_MSG 		 		 "  IP ADDRESS:  %d.%d.%d.%d\r\n"
#define NETMASK_MSG	         "  NETMASK:     %d.%d.%d.%d\r\n"
#define GW_MSG 		 		 "  GATEWAY:     %d.%d.%d.%d\r\n"
#define MAC_MSG		 		 "  MAC ADDRESS: %x:%x:%x:%x:%x:%x\r\n"
#define PRINT_NETINFO(netInfo)  																					\
  printf(NETWORK_MSG);											\
  printf("%s %0x:%0x:%0x:%0x:%0x:%0x\n", MAC_MSG, netInfo.mac[0], netInfo.mac[1], netInfo.mac[2], netInfo.mac[3], netInfo.mac[4], netInfo.mac[5]);\
  printf("%s %02d.%02d.%02d.%02d\n", IP_MSG, netInfo.ip[0], netInfo.ip[1], netInfo.ip[2], netInfo.ip[3]);										\
  printf("%s %02d.%02d.%02d.%02d\n", NETMASK_MSG, netInfo.sn[0], netInfo.sn[1], netInfo.sn[2], netInfo.sn[3]);								\
  printf("%s %02d.%02d.%02d.%02d\n", GW_MSG, netInfo.gw[0], netInfo.gw[1], netInfo.gw[2], netInfo.gw[3]);

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
  * @brief  Main program
  * @param  None
  * @retval None
  */

int main(void) {
	initialise_monitor_handles();
	HAL_Init();
	int8_t retVal, sockStatus;
  /* Configure the System clock to 84 MHz */
	SystemClock_Config();

	while (1) {
		wizchipInit(&wiznetInfo);
		PRINT_NETINFO(wiznetInfo);
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
						printf(CONN_ESTABLISHED_MSG, remoteIP[0], remoteIP[1], remoteIP[2], remoteIP[3], remotePort);
						/* Let's send a welcome message and closing socket */
						if((retVal = send(0, (uint8_t *)GREETING_MSG, strlen(GREETING_MSG))) == (int16_t)strlen(GREETING_MSG)) {
							printf(SENT_MESSAGE_MSG);
						} else { /* Oops: something went wrong during data transfer */
							printf(WRONG_RETVAL_MSG, retVal);
						}
						break;
					} else { /* Something went wrong with remote peer, maybe the connection was closed unexpectedly */
						printf(WRONG_STATUS_MSG, sockStatus);
						break;
					}
				}
			} else  {/* Ops: socket not in LISTEN mode. Something went wrong */
				printf(LISTEN_ERR_MSG);
			}
		} else { /* Can't open the socket. This means something is wrong with W5100 configuration: maybe SPI issue? */
			printf(WRONG_RETVAL_MSG, retVal);
		}

		/* We close the socket and start a connection again */
		disconnect(0);
		close(0);
	} // End of Main loop
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
