#include "i2c.h"
#include "config.h"

#define DEBUG_MSG 1
#include "dbgmsg.h"

I2C_HandleTypeDef hi2c1;

volatile uint8_t I2C_transferRequested = 0;
volatile uint16_t I2C_transferDir = I2C_DIR_UNKNOWN;

/**
* @brief I2C MSP Initialization
* This function configures the hardware resources used in this example
* @param hi2c: I2C handle pointer
* @retval None
*/
void HAL_I2C_MspInit(I2C_HandleTypeDef* hi2c)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  if(hi2c->Instance==I2C1)
  {
  /* USER CODE BEGIN I2C1_MspInit 0 */

  /* USER CODE END I2C1_MspInit 0 */
  
    __HAL_RCC_GPIOB_CLK_ENABLE();
    /**I2C1 GPIO Configuration    
    PB6     ------> I2C1_SCL
    PB7     ------> I2C1_SDA 
    */
    GPIO_InitStruct.Pin = GPIO_PIN_6|GPIO_PIN_7;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
		GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /* Peripheral clock enable */
    __HAL_RCC_I2C1_CLK_ENABLE();
    /* I2C1 interrupt Init */
    HAL_NVIC_SetPriority(I2C1_EV_IRQn, I2C1_EV_PreP, 0);
    HAL_NVIC_EnableIRQ(I2C1_EV_IRQn);
    HAL_NVIC_SetPriority(I2C1_ER_IRQn, I2C1_ER_PreP, 0);
    HAL_NVIC_EnableIRQ(I2C1_ER_IRQn);
  }
}

/**
* @brief I2C MSP De-Initialization
* This function freeze the hardware resources used in this example
* @param hi2c: I2C handle pointer
* @retval None
*/
void HAL_I2C_MspDeInit(I2C_HandleTypeDef* hi2c)
{
  if(hi2c->Instance==I2C1)
  {
    /* Peripheral clock disable */
    __HAL_RCC_I2C1_CLK_DISABLE();
  
    /**I2C1 GPIO Configuration    
    PB6     ------> I2C1_SCL
    PB7     ------> I2C1_SDA 
    */
    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_6|GPIO_PIN_7);

    /* I2C1 interrupt DeInit */
    HAL_NVIC_DisableIRQ(I2C1_EV_IRQn);
    HAL_NVIC_DisableIRQ(I2C1_ER_IRQn);
  }
}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
void I2C1_Init(void)
{
  hi2c1.Instance = I2C1;
	
	// Master settings
  hi2c1.Init.ClockSpeed = 400000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
	
	// Slave settings
	//hi2c1.Mode = HAL_I2C_MODE_SLAVE;
	
  hi2c1.Init.OwnAddress1 = I2C_ADDR << 1;//0x9C;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    errmsg("HAL_I2C_Init() failed\r\n");
		return;
  }
	
	//HAL_I2C_EnableListen_IT(&hi2c1);
}

void I2C1_EV_ISR (void)
{
	HAL_I2C_EV_IRQHandler(&hi2c1);
	//dbgmsg("I2C1_EV_ISR\r\n");
}

void I2C1_ER_ISR (void)
{
	HAL_I2C_ER_IRQHandler(&hi2c1);
	//dbgmsg("I2C1_ER_ISR\r\n");
}


void HAL_I2C_AbortCpltCallback(I2C_HandleTypeDef *hi2c)
{
	errmsg("I2C_AbortCpltCallback\r\n");
}

void HAL_I2C_ErrorCallback(I2C_HandleTypeDef *hi2c)
{
	errmsg("ErrorCode: 0x%08X\r\n", hi2c->ErrorCode);
	
	if(hi2c->ErrorCode == HAL_I2C_ERROR_BERR)
	{
		errmsg("BERR_ERROR\r\n");
	}
}

void HAL_I2C_ListenCpltCallback(I2C_HandleTypeDef *hi2c)
{
	//dbgmsg("STOP bit cames\r\n");
}


void HAL_I2C_AddrCallback(I2C_HandleTypeDef *hi2c, uint8_t TransferDirection, uint16_t AddrMatchCode)
{
  //dbgmsg("I2C1_AddrCallback. Direction: %u\r\n", TransferDirection);
	
	I2C_transferRequested = 1;
	I2C_transferDir = TransferDirection;	
	
	//dbgmsg("XferSize: %u, XferCount: %u, XferOptions: %u, PreviousState: %u\r\n", hi2c->XferSize, hi2c->XferCount, hi2c->XferOptions, hi2c->PreviousState );
	//dbgmsg("State: %u\r\n", hi2c->State);
	
}

