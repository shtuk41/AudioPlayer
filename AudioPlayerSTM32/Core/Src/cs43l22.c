/*
 * cs43l22.c
 *
 *  Created on: Mar 19, 2025
 *      Author: shtuk
 */
#include "math.h"
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_def.h"
#include "cs43l22.h"

#define AUDIO_RATE 48000   // 48 kHz sample rate
#define AUDIO_FREQUENCY 1000 // 1 kHz tone frequency
#define AUDIO_VOLUME 0x7F         // Set volume (max)
#define AUDIO_DURATION 2     // Duration of the tone in seconds

#define BUFFER_SIZE (AUDIO_RATE / AUDIO_FREQUENCY)
#define VOLUME_CONVERT_D(Volume)    (((Volume) > 100)? 24:((uint8_t)((((Volume) * 48) / 100) - 24)))

int16_t audio_buffer[BUFFER_SIZE];


extern I2C_HandleTypeDef hi2c1;
extern I2S_HandleTypeDef hi2s3;

static void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

void generate_audio_buffer(void)
{
    for (int i = 0; i < BUFFER_SIZE; i++)
    {
        audio_buffer[i] = (int16_t)(sin(2 * M_PI * AUDIO_FREQUENCY * i / AUDIO_RATE) * AUDIO_VOLUME);
    }
}

static void CS43L22_WriteReg(uint8_t reg, uint8_t value) {

	uint8_t data[2] = { reg, value };

    HAL_StatusTypeDef ret = HAL_I2C_Master_Transmit(&hi2c1, CS43L22_I2C_ADDR, data, 2, HAL_MAX_DELAY);

    if (ret != HAL_OK)
    {
    	Error_Handler();
    }
}

static uint8_t CS43L22_ReadReg(uint8_t reg)
{
    uint8_t value;

    HAL_StatusTypeDef ret = HAL_I2C_Master_Transmit(&hi2c1, CS43L22_I2C_ADDR, &reg, 1, HAL_MAX_DELAY);

    if (ret != HAL_OK)
    {
    	Error_Handler();
    }

    ret = HAL_I2C_Master_Receive(&hi2c1, CS43L22_I2C_ADDR, &value, 1, HAL_MAX_DELAY);

    if (ret != HAL_OK)
    {
    	Error_Handler();
    }

    return value;
}
/********************************************
 *	 initialize codec
 *
 ********************************************/
void CS43L22_Init(void)
{

	// Perform hardware reset
    HAL_GPIO_WritePin(CS43L22_RESET_GPIO, CS43L22_RESET_PIN, GPIO_PIN_RESET);
    HAL_Delay(100);
    HAL_GPIO_WritePin(CS43L22_RESET_GPIO, CS43L22_RESET_PIN, GPIO_PIN_SET);
    HAL_Delay(100);

    // Power on CS43L22 (example)
    CS43L22_WriteReg(CS43L22_POWER_CONTROL_1, 0x01);  // Set power control register to turn on

    //enable right and left headphone
    uint8_t data = 2 << 6;//PDN_HPB[0:1] = 10 (HP-B always on)
    data = 2 << 4;//PDN_HPA[0:1] = 10 (HP-A always on)
    data = 3 << 2;//PDN_SPKB[0:1] = 11 (Speaker B always off)
    data = 3 << 0;//PDN_SPKB[0:1] = 11 (Speaker A always off)

    CS43L22_WriteReg(CS43L22_POWER_CONTROL_2, data);

    // Autodmatic clock detection
    data = 1 << 7;
    CS43L22_WriteReg(CS43L22_CLOCKING_CONTROL, data);

    // Interface control 1
    data = CS43L22_ReadReg(CS43L22_INTERFACE_CONTROL_1);
    data &= (1 << 5); // Clear all bits except bit 5 which is reserved
    data &= ~(1 << 7);  // Slave
    data &= ~(1 << 6);  // Clock polarity: Not inverted
    data &= ~(1 << 4);  // No DSP mode
    data &= ~(1 << 3);  // Left justified, up to 24 bit (default)
    data |= (1 << 2);
    data |=  (3 << 0);  // 16-bit audio word length for I2S interface
    CS43L22_WriteReg(CS43L22_INTERFACE_CONTROL_1,data);

    //Passthrough x Select: Pass A
    data = CS43L22_ReadReg(CS43L22_PASSTHROUGH_A);
    data &= 0xF0;	//Bits 4-7 are reserved;
    data |= (1 << 0); //use AIN1A as source for passthrough
    data |= (1 << 3); //use AIN4A as source for passthrough
    CS43L22_WriteReg(CS43L22_PASSTHROUGH_A,data);

    //Passthrough x Select: Pass B
    data = CS43L22_ReadReg(CS43L22_PASSTHROUGH_B);
    data &= 0xF0;	//Bits 4-7 are reserved;
    data |= (1 << 0); //use AIN1B as source for passthrough
    data |= (1 << 3); //use AIN4B as source for passthrough
    CS43L22_WriteReg(CS43L22_PASSTHROUGH_B,data);

    //Miscellaneous Controls
    data = CS43L22_ReadReg(CS43L22_MISCELLANEOUS_CONTROLS);
    data = 0x02; //ramp up volume to new level
    CS43L22_WriteReg(CS43L22_MISCELLANEOUS_CONTROLS,data);

    //Playback Control 2
    data = CS43L22_ReadReg(CS43L22_PLAYBACK_CONTROL_2);
    data = 0x00;
    CS43L22_WriteReg(CS43L22_PLAYBACK_CONTROL_2,data);

    //Pass through volume
    data = 0x00;
    CS43L22_WriteReg(CS43L22_PASSTHROUGH_VOLUME_A,data);
    CS43L22_WriteReg(CS43L22_PASSTHROUGH_VOLUME_B,data);

    //PCM Volume
    data = 0x00;
    CS43L22_WriteReg(CS43L22_PCM_A_VOLUME,data);
    CS43L22_WriteReg(CS43L22_PCM_B_VOLUME,data);
}

/********************************************
 *	  Power down codec
 *
 ********************************************/
void CS43L22_DeInit(void)
{
	CS43L22_WriteReg(CS43L22_POWER_CONTROL_1, 0x00);
}

/********************************************
 *	  Set volume
 *
 ********************************************/
void CS43L22_SetVolume(uint8_t volume)
{
    if (volume > 100)
    	volume = 100;  // Limit to 100 max

    int8_t tempVol = volume - 50;
    tempVol = tempVol*(127/50);
    uint8_t myVolume =  (uint8_t )tempVol;

    CS43L22_WriteReg(CS43L22_PASSTHROUGH_VOLUME_A, myVolume);
    CS43L22_WriteReg(CS43L22_PASSTHROUGH_VOLUME_B, myVolume);

    myVolume = VOLUME_CONVERT_D(volume);

    CS43L22_WriteReg(CS43L22_REG_VOL_A_CTRL, myVolume);
    CS43L22_WriteReg(CS43L22_REG_VOL_B_CTRL, myVolume);
}

/********************************************
 *	  Mite / Unmute
 *
 ********************************************/
void CS43L22_Mute(uint8_t mute)
{
    if (mute)
    {
        CS43L22_WriteReg(CS43L22_POWER_CONTROL_1, 0x00);
    }
    else
    {
        CS43L22_WriteReg(CS43L22_POWER_CONTROL_1, 0x01);
    }
}

/********************************************
 *	  Enable headphones and turn off speakers
 *
 ********************************************/

void CS43L22_EnableRightLeft(void)
{
	uint8_t data = 0x00;
	data =  (2 << 6);  // PDN_HPB[0:1]  = 10 (HP-B always onCon)
	data |= (2 << 4);  // PDN_HPA[0:1]  = 10 (HP-A always on)
	data |= (3 << 2);  // PDN_SPKB[0:1] = 11 (Speaker B always off)
	data |= (3 << 0);  // PDN_SPKA[0:1] = 11 (Speaker A always off)
	CS43L22_WriteReg(CS43L22_POWER_CONTROL_2,data);
}

/********************************************
 *	  Start procedure
 *
 ********************************************/
void CS43L22_Start(void)
{
	// Required Initialization Settings
	uint8_t data = 0x99;
	CS43L22_WriteReg(CS43L22_00, 0x00);

    // Write 0x80 to register 0x47.
	data = 0x80;
	CS43L22_WriteReg(CS43L22_47,data);
	// Write '1'b to bit 7 in register 0x32.
	data = CS43L22_ReadReg(CS43L22_32);
	data |= 0x80;
	CS43L22_WriteReg(CS43L22_32,data);
	// Write '0'b to bit 7 in register 0x32.
	data = CS43L22_ReadReg(CS43L22_32);
	data &= ~(0x80);
	CS43L22_WriteReg(CS43L22_32,data);
	// Write 0x00 to register 0x00.
	data = 0x00;
	CS43L22_WriteReg(CS43L22_00, data);
	//Set the "Power Ctl 1" register (0x02) to 0x9E
	data = 0x9E;
	CS43L22_WriteReg(CS43L22_POWER_CONTROL_1, data);
}

/********************************************
 *	  Play audio by writing to I2S directly
 *
 ********************************************/
// Start audio playback (implementation can vary depending on your setup)
void CS43L22_Play_I2S(void)
{
	    generate_audio_buffer();

	    uint32_t audio_samples = AUDIO_RATE * AUDIO_DURATION;

	    while (audio_samples > 0)
	    {
	        for (int i = 0; i < BUFFER_SIZE; i++)
	        {
	        	HAL_StatusTypeDef ret = HAL_I2S_Transmit(&hi2s3, (uint16_t*)&audio_buffer[i], 1, HAL_MAX_DELAY);

	            if (ret != HAL_OK)
	            {
	                Error_Handler();
	            }

	            audio_samples--;
	            if (audio_samples == 0)
	            	break;
	        }
	    }
}

void CS43L22_Play(void)
{
	    generate_audio_buffer();

	    uint32_t audio_samples = AUDIO_RATE * AUDIO_DURATION; // Number of samples to play (2 seconds)

	    HAL_StatusTypeDef ret = HAL_I2S_Transmit_DMA(&hi2s3, (uint16_t*)audio_buffer, BUFFER_SIZE);

	    // Start the first DMA transfer
	    if (ret != HAL_OK)
	    {
	        Error_Handler();
	    }

	    // Wait for DMA to finish each transfer before continuing
	    while (audio_samples > 0)
	    {
	        while (HAL_I2S_GetState(&hi2s3) != HAL_I2S_STATE_READY)
	        {
	            // Wait for DMA transfer to complete
	        }

	        audio_samples -= BUFFER_SIZE;

	        if (audio_samples > 0)
	        {
	        	HAL_StatusTypeDef ret = HAL_I2S_Transmit_DMA(&hi2s3, (uint16_t*)audio_buffer, BUFFER_SIZE);

	            if (ret != HAL_OK)
	            {
	                Error_Handler();
	            }
	        }
	    }
}

/********************************************
 *	  Stop play audio
 *
 ********************************************/
void CS43L22_Stop(void)
{
	uint8_t data = 0x01;
	CS43L22_WriteReg(CS43L22_POWER_CONTROL_1, data);
}
