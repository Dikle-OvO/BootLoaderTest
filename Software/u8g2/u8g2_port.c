#include "u8g2_port.h"
#include "i2c.h" // 一定要包含 CubeMX 生成的 i2c.h

extern I2C_HandleTypeDef hi2c1; // 引用你的 I2C 句柄

/*
 * 核心回调函数1：处理硬件 I2C 通信
 * 这是一个通用的 STM32 HAL I2C 写法
 */
uint8_t u8x8_byte_stm32_hw_i2c(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
    static uint8_t buffer[32]; // 临时缓冲
    static uint8_t buf_idx;
    uint8_t *data;

    switch(msg)
    {
        case U8X8_MSG_BYTE_SEND:
            data = (uint8_t *)arg_ptr;
            while( arg_int > 0 )
            {
                buffer[buf_idx++] = *data;
                data++;
                arg_int--;
            }
            break;

        case U8X8_MSG_BYTE_INIT:
            // I2C 已经在 main 里的 MX_I2C1_Init 初始化过了，这里留空即可
            break;

        case U8X8_MSG_BYTE_SET_DC:
            // I2C 驱动不需要 DC 引脚，忽略
            break;

        case U8X8_MSG_BYTE_START_TRANSFER:
            buf_idx = 0;
            break;

        case U8X8_MSG_BYTE_END_TRANSFER:
            // 发送数据
            // u8x8_GetI2CAddress(u8x8) 会自动获取你在 Setup 里设置的地址
            // >> 1 是因为 HAL 库需要的地址是 7位地址左移一位后的结果，u8g2 存的也是这个，但为了保险通常直接用 u8x8 里的
            if(HAL_I2C_Master_Transmit(&hi2c1, u8x8_GetI2CAddress(u8x8), buffer, buf_idx, 1000) != HAL_OK)
                return 0;
            break;

        default:
            return 0;
    }
    return 1;
}

/*
 * 核心回调函数2：处理延时和 GPIO
 */
uint8_t u8x8_gpio_and_delay_stm32(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
    switch(msg)
    {
        case U8X8_MSG_DELAY_100NANO: // 延时 100ns
            __NOP();
            break;
        case U8X8_MSG_DELAY_10MICRO: // 延时 10us
            for (uint16_t n = 0; n < 320; n++) __NOP();
            break;
        case U8X8_MSG_DELAY_MILLI:   // 延时 ms
            HAL_Delay(arg_int);
            break;
        case U8X8_MSG_GPIO_I2C_CLOCK:
        case U8X8_MSG_GPIO_I2C_DATA:
            // 硬件 I2C 不需要手动控制引脚，留空
            break;
        default:
            return 0;
    }
    return 1;
}

/*
 * 封装好的初始化函数，直接在 main 里调用这个
 */
void u8g2_Init_STM32(u8g2_t *u8g2)
{
    // 1. 设置驱动
    // 参数1: u8g2 结构体
    // 参数2: 旋转方向 (U8G2_R0, R1, R2, R3)
    // 参数3: 字节发送回调 (我们写的)
    // 参数4: GPIO回调 (我们写的)

    // 注意：这里选用了 SSD1306 128x64 noname f (f代表 full framebuffer，全显存模式)
    // 如果你的屏幕是 SH1106，把 ssd1306 改成 sh1106 即可
    u8g2_Setup_ssd1306_i2c_128x64_noname_f(u8g2, U8G2_R0, u8x8_byte_stm32_hw_i2c, u8x8_gpio_and_delay_stm32);

    // 2. 设置 I2C 地址 (默认通常是 0x78)
    u8x8_SetI2CAddress(&u8g2->u8x8, 0x78);

    // 3. 初始化显示
    u8g2_InitDisplay(u8g2);

    // 4. 唤醒屏幕
    u8g2_SetPowerSave(u8g2, 0);
}