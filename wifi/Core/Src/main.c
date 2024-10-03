/* USER CODE BEGIN Header */
/**
 **
 * @file           : main.c
 * @brief          : Main program body
 **
 * @attention
 *
 * Copyright (c) 2024 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 **
 */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "string.h"
#include <stdbool.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

#define RxBufferSize 100
char TxBuffer[9999];
unsigned char RxBuffer[RxBufferSize];
int i = 0;
int k = 0;
int z = 0;
int *p = &i;
volatile int *q = &k;
volatile int *a = &z;
uint32_t lastRxTime = 0;
uint32_t delayTime = 3000;

/* Yeni Bayrak Değişkeni */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
UART_HandleTypeDef huart1;
DMA_HandleTypeDef hdma_usart1_rx;
DMA_HandleTypeDef hdma_usart1_tx;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_USART1_UART_Init(void);
/* USER CODE BEGIN PFP */
void TransmitWifitoMQTTfunction();
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

void SetupDMAUSART() {
	HAL_UARTEx_ReceiveToIdle_DMA(&huart1, RxBuffer, RxBufferSize);

	__HAL_DMA_DISABLE_IT(&hdma_usart1_rx, DMA_IT_HT);
}

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size) {

	HAL_Delay(2000);
	if (huart->Instance == USART1) {

		memcpy(RxBuffer, huart->pRxBuffPtr, Size);
		if (*p < 3) {

			if ((strncmp((const char*) RxBuffer, "OK", 2) == 0)
					|| (strncmp((const char*) RxBuffer, "\r\nready\r\n", 5) == 0)) {
				memset(RxBuffer, 0, sizeof(RxBuffer));
				if (*p == 2) {
					HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_SET);
				}
			} else {
				memset(RxBuffer, 0, sizeof(RxBuffer));
				HAL_GPIO_WritePin(GPIOC, GPIO_PIN_3, GPIO_PIN_SET);
				Error_Handler();
			}

		}

		else if (strncmp((const char*) RxBuffer, "AT+QSTAAPINFO", 12) == 0) {
			memcpy(TxBuffer, RxBuffer, sizeof(RxBuffer));
			memset(RxBuffer, 0, sizeof(RxBuffer));
			HAL_UART_Transmit_DMA(&huart1, (uint8_t*) TxBuffer,
					strlen(TxBuffer));
			// HAL_GPIO_WritePin(GPIOA, GPIO_PIN_3, GPIO_PIN_SET); // yapildi

		}

		else if (strncmp((const char*) RxBuffer, "ERROR", 5) == 0) {
			HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_RESET);
			HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_SET); //SADECE KIRMIZI
			HAL_GPIO_WritePin(GPIOC, GPIO_PIN_3, GPIO_PIN_RESET);
			memset(RxBuffer, 0, sizeof(RxBuffer));

		} else if (strncmp((const char*) RxBuffer, "+QSTASTAT:WLAN_CONNECTED",
				24) == 0) {

			HAL_GPIO_WritePin(GPIOC, GPIO_PIN_3, GPIO_PIN_SET);
			*a = 1;

			memset(RxBuffer, 0, sizeof(RxBuffer));

		}
		else if (strncmp((const char*) RxBuffer, "+QSTASTAT:AP_DISCONNECT",
						23) == 0) {

					HAL_GPIO_WritePin(GPIOC, GPIO_PIN_3, GPIO_PIN_RESET);
					*a = 1;

					memset(RxBuffer, 0, sizeof(RxBuffer));

				}
//+QIURC: "recv",10
		else if (strncmp((const char*) RxBuffer, "\r\n+QIURC: \"recv\"", 16)
				== 0) {
			char atCommand[100];
			char *ptr;
			int length;
			ptr = strstr(RxBuffer, "+QIURC: \"recv\",");
			// Sayıyı ayrıştır
			sscanf(ptr, "+QIURC: \"recv\",%d", &length);

			// Yeni string'i oluştur: "AT+QIRD=<length>,256"
			snprintf(atCommand, sizeof(atCommand),"AT+QIRD=%d,256\r\n", length);

			HAL_UART_Transmit_DMA(&huart1, (uint8_t*) atCommand,
					strlen(atCommand));
			memset(RxBuffer, 0, sizeof(RxBuffer));

		} else if (strncmp((const char*) RxBuffer, "+QIRD:", 6) == 0) {
		//	HAL_Delay(1000);

			/*for (int j=0;j<strlen(RxBuffer)-17;j++)
			 {
			 Wifi[j]=RxBuffer[11+j];


			 }*/
			char atCommand[100] = "AT+QSTAAPINFO=";
			char *ptr;
			int length;
			ptr = strstr(RxBuffer, "D: ");
			sscanf(ptr, "D: %d\r\n", &length);
			ptr = strstr(ptr, "\r\n") + 2; // 2 karakter ilerle çünkü "\r\n" atlanacak
			char Wifi[length + 1];        // Uzunluk kadar alan ayır
			strncpy(Wifi, ptr, length);   // Uzunluk kadar kopyala
			Wifi[length] = '\0';          // Null-terminator ekle
			strcat(atCommand, Wifi);
			strcat(atCommand, "\r\n");

			HAL_UART_Transmit_DMA(&huart1, (uint8_t*) atCommand,
					strlen(atCommand));

		}

		else {
			memset(RxBuffer, 0, sizeof(RxBuffer));

		}

	}

	HAL_UARTEx_ReceiveToIdle_DMA(&huart1, RxBuffer, RxBufferSize);
	__HAL_DMA_DISABLE_IT(&hdma_usart1_rx, DMA_IT_HT);

}

void TransmitapFunction() {

	char *Settings[] = { "AT+QSOFTAP=\"Rise_X_Gate_Away\"\r\n",
			"AT+QIOPEN=1,\"TCP LISTENER\",\"\",\"\",20,0\r\n",
			"AT+QIOPEN=2,\"TCP\",\"192.168.19.1\",20.10.0\r\n",
			//	 "AT+QISEND=2,30,\"Superbox_WiFi_EFCA,NA2TRGLL5Y1\"\r\n",
			//"AT+QIRD=10,256\r\n"

	};

	for (*p = 0; *p < 3; (*p)++) {
		memset(TxBuffer, 0, sizeof(*p));
		strcpy(TxBuffer, Settings[*p]);

		HAL_UART_Transmit_DMA(&huart1, (uint8_t*) TxBuffer, strlen(TxBuffer));

		HAL_Delay(700);

	}
}

void BLEHIGH() {
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_7, GPIO_PIN_SET);
}

void TransmitWifitoMQTTfunction()

{
	HAL_Delay(700);
	char *SettingsWifi[] = { "AT+QMTCFG=\"version\",1,4\r\n", // MQTT versiyonunu ayarlar (3.1.1)
			"AT+QMTOPEN=1,\"104.248.21.33\",1883\r\n", // MQTT sunucusuna bağlantı açar (IP: 104.248.21.33, port: 1883)
			"AT+QMTCONN=1,\"Client8320\"\r\n", // MQTT sunucusuna Client8320 kimliğiyle bağlanır
			"AT+QMTSUB=1,1,\"risex/1\",1\r\n", // risex/1 konusuna abone olur, QoS=1
			"AT+QMTPUB=1,1,1,0,\"risex/1\",5,\"Hello\"\r\n" // risex/1 konusuna mesaj yayınlar (26 karakter)
			};

	for (*q = 0; *q < 5; (*q)++) {
		memset(TxBuffer, 0, sizeof(*q));
		strcpy(TxBuffer, SettingsWifi[*q]);

		HAL_UART_Transmit_DMA(&huart1, (uint8_t*) TxBuffer, strlen(TxBuffer));

		HAL_Delay(500);
	}

}

void TransmitBleFunction() {
	char *Settings[] = { "AT+QRST\r\n",
			"AT+QBLEINIT=3\r\n",
			"AT+QBLENAME=Rise_X_Gate_Away\r\n"

	};

	for (*p = 0; *p < 3; (*p)++) {
		memset(TxBuffer, 0, sizeof(*p));
		strcpy(TxBuffer, Settings[*p]);

		HAL_UART_Transmit_DMA(&huart1, (uint8_t*) TxBuffer, strlen(TxBuffer));

		HAL_Delay(500);

	}

}
/* USER CODE END 0 */

/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void) {

	/* USER CODE BEGIN 1 */

	/* USER CODE END 1 */

	/* MCU Configuration--------------------------------------------------------*/

	/* Reset of all peripherals, Initializes the Flash interface and the Systick. */
	HAL_Init();

	/* USER CODE BEGIN Init */

	/* USER CODE END Init */

	/* Configure the system clock */
	SystemClock_Config();
	/* USER CODE BEGIN SysInit */

	/* USER CODE END SysInit */

	/* Initialize all configured peripherals */
	MX_GPIO_Init();
	MX_DMA_Init();
	MX_USART1_UART_Init();
	/* USER CODE BEGIN 2 */
	BLEHIGH();
	SetupDMAUSART();
	TransmitBleFunction();
	TransmitapFunction();

//HAL_UART_Transmit_DMA(&huart1, (uint8_t*)TxBuffer, strlen(TxBuffer));

	/* USER CODE END 2 */

	/* Infinite loop */
	/* USER CODE BEGIN WHILE */
	while (1) {
		/* USER CODE END WHILE */

		/* USER CODE BEGIN 3 */

	}
	/* USER CODE END 3 */
}

/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void) {
	RCC_OscInitTypeDef RCC_OscInitStruct = { 0 };
	RCC_ClkInitTypeDef RCC_ClkInitStruct = { 0 };

	/** Configure the main internal regulator output voltage
	 */
	__HAL_RCC_PWR_CLK_ENABLE();
	__HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

	/** Initializes the RCC Oscillators according to the specified parameters
	 * in the RCC_OscInitTypeDef structure.
	 */
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
	RCC_OscInitStruct.HSIState = RCC_HSI_ON;
	RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
	if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
		Error_Handler();
	}

	/** Initializes the CPU, AHB and APB buses clocks
	 */
	RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
			| RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
	RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
	RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

	if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK) {
		Error_Handler();
	}
}

/**
 * @brief USART1 Initialization Function
 * @param None
 * @retval None
 */
static void MX_USART1_UART_Init(void) {

	/* USER CODE BEGIN USART1_Init 0 */

	/* USER CODE END USART1_Init 0 */

	/* USER CODE BEGIN USART1_Init 1 */

	/* USER CODE END USART1_Init 1 */
	huart1.Instance = USART1;
	huart1.Init.BaudRate = 115200;
	huart1.Init.WordLength = UART_WORDLENGTH_8B;
	huart1.Init.StopBits = UART_STOPBITS_1;
	huart1.Init.Parity = UART_PARITY_NONE;
	huart1.Init.Mode = UART_MODE_TX_RX;
	huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
	huart1.Init.OverSampling = UART_OVERSAMPLING_16;
	if (HAL_UART_Init(&huart1) != HAL_OK) {
		Error_Handler();
	}
	/* USER CODE BEGIN USART1_Init 2 */

	/* USER CODE END USART1_Init 2 */

}

/**
 * Enable DMA controller clock
 */
static void MX_DMA_Init(void) {

	/* DMA controller clock enable */
	__HAL_RCC_DMA2_CLK_ENABLE();

	/* DMA interrupt init */
	/* DMA2_Stream2_IRQn interrupt configuration */
	HAL_NVIC_SetPriority(DMA2_Stream2_IRQn, 1, 0);
	HAL_NVIC_EnableIRQ(DMA2_Stream2_IRQn);
	/* DMA2_Stream7_IRQn interrupt configuration */
	HAL_NVIC_SetPriority(DMA2_Stream7_IRQn, 1, 0);
	HAL_NVIC_EnableIRQ(DMA2_Stream7_IRQn);

}

/**
 * @brief GPIO Initialization Function
 * @param None
 * @retval None
 */
static void MX_GPIO_Init(void) {
	GPIO_InitTypeDef GPIO_InitStruct = { 0 };
	/* USER CODE BEGIN MX_GPIO_Init_1 */
	/* USER CODE END MX_GPIO_Init_1 */

	/* GPIO Ports Clock Enable */
	__HAL_RCC_GPIOH_CLK_ENABLE();
	__HAL_RCC_GPIOC_CLK_ENABLE();
	__HAL_RCC_GPIOA_CLK_ENABLE();

	/*Configure GPIO pin Output Level */
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_7, GPIO_PIN_RESET);

	/*Configure GPIO pin : PC7 */
	GPIO_InitStruct.Pin = GPIO_PIN_7;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

	/* USER CODE BEGIN MX_GPIO_Init_2 */
	/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void) {
	/* USER CODE BEGIN Error_Handler_Debug */

	__disable_irq();
	while (1) {
	}
	/* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
