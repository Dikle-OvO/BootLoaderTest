#ifndef __OLED_H
#define __OLED_H

#include "main.h"

// 引用外部字库 (确保你有 OLED_Font.h 并且变量名为 OLED_F8x16)
extern const uint8_t OLED_F8x16[][16];

void OLED_Init(void);
void OLED_Clear(void);
void OLED_ShowChar(uint8_t Line, uint8_t Column, char Char);
void OLED_ShowString(uint8_t Line, uint8_t Column, char *String);
void OLED_ShowNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length);

// 原来是 uint8_t，改为 int
void OLED_DrawLine(int x1, int y1, int x2, int y2);
void OLED_DrawRectangle(uint8_t x, uint8_t y, uint8_t w, uint8_t h);

// 新增的核心函数
void OLED_Refresh_Gram(void);
void Animation_BeatingHeart(void);
void Init_Heart_Mesh(void);
void Animation_RefinedHeart(void);

#endif