#ifndef __U8G2_PORT_H
#define __U8G2_PORT_H

#include "main.h"
#include "u8g2.h"

// 硬件I2C回调函数
uint8_t u8x8_byte_stm32_hw_i2c(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr);

// GPIO和延时回调函数
uint8_t u8x8_gpio_and_delay_stm32(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr);

// 初始化封装函数
void u8g2_Init_STM32(u8g2_t *u8g2);

#endif