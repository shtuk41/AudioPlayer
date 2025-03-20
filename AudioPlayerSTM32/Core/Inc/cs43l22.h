/*
 * cs43l22.h
 *
 *  Created on: Mar 19, 2025
 *      Author: shtuk
 */
#ifndef __CS43L22_H
#define __CS43L22_H


//CS43L22 CIRRUS PINS/REGISTERS
#define CS43L22_I2C_ADDR      0x94  // I2C address for CS43L22
#define CS43L22_RESET_PIN      GPIO_PIN_4  // Example GPIO pin for reset
#define CS43L22_RESET_GPIO     GPIOD      // Example GPIO port for reset pin
#define CS43L22_REG_SAMPLERATE 0x0C

// CS43L22 Register addresses (simplified example)
#define CS43L22_POWER_CONTROL_1 0x02
#define CS43L22_POWER_CONTROL_2 0x04
#define CS43L22_CLOCKING_CONTROL 0x05
#define CS43L22_INTERFACE_CONTROL_1 0x06
#define CS43L22_PASSTHROUGH_A 0x08
#define CS43L22_PASSTHROUGH_B 0x09
#define CS43L22_MISCELLANEOUS_CONTROLS 0x0E
#define CS43L22_PLAYBACK_CONTROL_2 0x0F
#define CS43L22_PASSTHROUGH_VOLUME_A 0x14
#define CS43L22_PASSTHROUGH_VOLUME_B 0x15
#define CS43L22_PCM_A_VOLUME 0x1A
#define CS43L22_PCM_B_VOLUME 0x1B
#define CS43L22_REG_VOL_A_CTRL   0x20
#define CS43L22_REG_VOL_B_CTRL   0x21

#define CS43L22_00 0x00
#define CS43L22_47 0x47
#define CS43L22_32 0x32

void CS43L22_Init(void);
void CS43L22_DeInit(void);
void CS43L22_SetVolume(uint8_t);
void CS43L22_Mute(uint8_t);
void CS43L22_Start(void);
void CS43L22_Play(void);
void CS43L22_Stop(void);
void CS43L22_EnableRightLeft(void);

#endif








