
#include <stdio.h>
#include <string.h>

#include "stm32f1xx_hal.h"
#include "uart.h"
#include "timer.h"
#include "i2c.h"
#include "flash.h"


#define DEBUG_MSG 1
#include "dbgmsg.h"


// I2C-slave registers map
#define COOLER1_REG		0x10		// set cooler1 pwm (0..100)
#define COOLER2_REG		0x20		// set cooler2 pwm (0..100)
#define STATUS1_REG		0x11		// get cooler1 current pwm value
#define STATUS2_REG		0x21		// get cooler2 current pwm value

/* Private variables ---------------------------------------------------------*/
IWDG_HandleTypeDef hiwdg;

uint8_t i2cBuf[2] = {0,0};
uint32_t coolers_pwm[2] = {0,0};


/* Private function prototypes -----------------------------------------------*/
static int SystemClock_Config(void);
static void GPIO_Init(void);
static void IWDG_Init(void);

int main(void)
{	
  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();
	GPIO_Init();
  /* Configure the system clock */
  int res = SystemClock_Config();

  /* Initialize all configured peripherals */
	UART1_Init(115200);		// debug port	
	if(res < 0) errmsg("SystemClock_Config() failed (%d)\r\n", res);
  
  TIM3_PWM_Init();
	I2C1_Init();
  IWDG_Init();
	flash_read_page(ADDR_FLASH_PAGE_127, coolers_pwm, sizeof(coolers_pwm));
	for (int i = 0; i < sizeof(coolers_pwm); i++){
		if (coolers_pwm[i] > 100) coolers_pwm[i] = 100;
	}
	
	dbgmsg("~ Cooler Controller starts ~\r\n");
	dbgmsg("Cooler1 PWM: %u, Cooler2 PWM: %u\r\n", coolers_pwm[0], coolers_pwm[1]);
	
	
	TIM3_PWM_ConfigAndStart(TIM_CHANNEL_1, 0);
	TIM3_PWM_ConfigAndStart(TIM_CHANNEL_3, 0);
	
	uint32_t cnt = 0;
	
	
  while (1)
  {
		HAL_I2C_EnableListen_IT(&hi2c1);
		cnt++;
		if(cnt >= 300000){
			HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
			cnt = 0;
		}
		
		if(I2C_transferRequested)
		{
			HAL_I2C_Slave_Seq_Receive_IT(&hi2c1, i2cBuf, 1, I2C_FIRST_FRAME);
			while (HAL_I2C_GetState(&hi2c1) != HAL_I2C_STATE_LISTEN);
			
			// Writing to register
			if((i2cBuf[0] == COOLER1_REG) || (i2cBuf[0] == COOLER2_REG)) 
			{
				HAL_I2C_Slave_Seq_Receive_IT(&hi2c1, &i2cBuf[1], 1, I2C_LAST_FRAME);
				while (HAL_I2C_GetState(&hi2c1) != HAL_I2C_STATE_LISTEN);
				
				dbgmsg("I2C: set 0x%02X to 0x%02X\r\n", i2cBuf[0], i2cBuf[1]);
				if(i2cBuf[1] > 100) i2cBuf[1] = 100;
				
				switch(i2cBuf[0])
				{
					case COOLER1_REG:
						TIM3_PWM_ConfigAndStart(TIM_CHANNEL_1, i2cBuf[1]);
						coolers_pwm[0] = i2cBuf[1];
						flash_write_page(ADDR_FLASH_PAGE_127, coolers_pwm, 2);
						break;
					
					case COOLER2_REG:
						TIM3_PWM_ConfigAndStart(TIM_CHANNEL_3, i2cBuf[1]);
						coolers_pwm[1] = i2cBuf[1];
						flash_write_page(ADDR_FLASH_PAGE_127, coolers_pwm, 2);
						break;
				}
			}
			// Reading register
			else
			{
				//HAL_Delay(1);		
				dbgmsg("I2C: get 0x%02X ", i2cBuf[0]);

				uint8_t responce = 0xFF; // Default value
				
				switch (i2cBuf[0])
				{
					case STATUS1_REG:
						responce = (uint8_t)coolers_pwm[0];
						break;
					
					case STATUS2_REG:
						responce = (uint8_t)coolers_pwm[1];
						break;
				}
				
				dbgmsg("sending 0x%02X\r\n", responce);
				
				HAL_I2C_Slave_Seq_Transmit_IT(&hi2c1, &responce, 1, I2C_LAST_FRAME);
				while (HAL_I2C_GetState(&hi2c1) != HAL_I2C_STATE_READY);
			}
			
			I2C_transferRequested = 0;
			I2C_transferDir = I2C_DIR_UNKNOWN;
			HAL_I2C_EnableListen_IT(&hi2c1);
		}

		HAL_IWDG_Refresh(&hiwdg);
  }

}

void HAL_MspInit(void)
{
  __HAL_RCC_AFIO_CLK_ENABLE();
  __HAL_RCC_PWR_CLK_ENABLE();

  /* System interrupt init*/

  /** NOJTAG: JTAG-DP Disabled and SW-DP Enabled */
  __HAL_AFIO_REMAP_SWJ_NOJTAG();
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
static int SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the CPU, AHB and APB busses clocks */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSI|RCC_OSCILLATORTYPE_HSE
                              |RCC_OSCILLATORTYPE_LSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.LSEState = RCC_LSE_ON;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) 
		return -1;

  /** Initializes the CPU, AHB and APB busses clocks */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
		return -2;
	
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_RTC;
  PeriphClkInit.RTCClockSelection = RCC_RTCCLKSOURCE_LSE;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
		return -3;
	
	return 0;
}

/**
  * @brief IWDG Initialization Function
  * @param None
  * @retval None
  */
static void IWDG_Init(void)
{
  hiwdg.Instance = IWDG;
  hiwdg.Init.Prescaler = IWDG_PRESCALER_16;
  hiwdg.Init.Reload = 4095;
  if (HAL_IWDG_Init(&hiwdg) != HAL_OK)
  {
    errmsg("HAL_IWDG_Init() failed\r\n");
  }
}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();

  /*Configure GPIO pin : PC13 */
  GPIO_InitStruct.Pin = GPIO_PIN_13;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);  
	
	/*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);

}

