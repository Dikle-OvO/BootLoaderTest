/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "i2c.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "stdio.h"
#include "OLED.h"
#include <math.h> // 必须包含，用于3D计算
#include "u8g2_port.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
u8g2_t u8g2; // 定义 u8g2 全局句柄
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
uint32_t APP_ADDRESS=0x08008000;
typedef void (*pFunction) (void);
void JumpToApplication(void) {
  if (((*(__IO uint32_t*)APP_ADDRESS) & 0x2FFE0000) == 0x20000000) {
    //2.将中断向量长重定向到应用程序地址
    SCB->VTOR=APP_ADDRESS;
    //3.获取应用程序的栈顶地址和复位处理函数地址
    pFunction app_reset_handler =(pFunction) (*(__IO uint32_t*)(APP_ADDRESS + 4));
    __set_MSP(*(__IO uint32_t*)APP_ADDRESS);
    //4.跳转到应用程序
    app_reset_handler();
  }
}

/* 动画变量定义 */
// FPS
uint32_t frameCount = 0;
uint32_t lastTime = 0;
uint32_t fps = 0;

// 弹跳方块变量
int box_x = 0, box_y = 0;
int box_dir_x = 2, box_dir_y = 2; // 速度

// 3D 立方体变量
float cube_angle_x = 0;
float cube_angle_y = 0;
float cube_angle_z = 0;

/* * 顶点定义逻辑：(Z= -10 后平面的四个点) -> (Z= +10 前平面的四个点)
 * 顺序均为：左下 -> 右下 -> 右上 -> 左上 (逆时针环绕)
 */
float vertices[8][3] = {
  // 后平面 (Index 0-3)
  {-10, -10, -10},
  { 10, -10, -10},
  { 10,  10, -10},
  {-10,  10, -10},

  // 前平面 (Index 4-7)
  {-10, -10,  10},
  { 10, -10,  10},
  { 10,  10,  10},
  {-10,  10,  10}
};

int edges[12][2] = {
  // 后平面 (Back Face)
  {0, 1}, {1, 2}, {2, 3}, {3, 0},
  // 前平面 (Front Face)
  {4, 5}, {5, 6}, {6, 7}, {7, 4},
  // 连接前后 (Connecting Lines)
  {0, 4}, {1, 5}, {2, 6}, {3, 7}
};

/* * 3D 水晶之心模型数据
 * 坐标系：X(左右), Y(上下), Z(前后)
 */

// // 顶点数量：10个
// float vertices[10][3] = {
//   // --- 中轴线上的点 (前后面共用) ---
//   {  0.0f, -20.0f,   0.0f}, // 0: 底部尖端 (Bottom Tip)
//   {  0.0f,   5.0f,   0.0f}, // 1: 顶部中间凹陷 (Top Center)
//
//   // --- 前层 (Front Face, Z > 0) ---
//   {-12.0f,  15.0f,   6.0f}, // 2: 左上瓣 (Top Left Front)
//   { 12.0f,  15.0f,   6.0f}, // 3: 右上瓣 (Top Right Front)
//   {-18.0f,   0.0f,   6.0f}, // 4: 左侧宽 (Side Left Front)
//   { 18.0f,   0.0f,   6.0f}, // 5: 右侧宽 (Side Right Front)
//
//   // --- 后层 (Back Face, Z < 0) ---
//   {-12.0f,  15.0f,  -6.0f}, // 6: 左上瓣 (Back)
//   { 12.0f,  15.0f,  -6.0f}, // 7: 右上瓣 (Back)
//   {-18.0f,   0.0f,  -6.0f}, // 8: 左侧宽 (Back)
//   { 18.0f,   0.0f,  -6.0f}  // 9: 右侧宽 (Back)
// };
//
// // 连线数量：16条
// int edges[16][2] = {
//   // --- 前面轮廓 (Front Face Outline) ---
//   {1, 2}, {2, 4}, {4, 0}, // 左半边
//   {0, 5}, {5, 3}, {3, 1}, // 右半边
//
//   // --- 后面轮廓 (Back Face Outline) ---
//   {1, 6}, {6, 8}, {8, 0}, // 左半边
//   {0, 9}, {9, 7}, {7, 1}, // 右半边
//
//   // --- 前后连接线 (Connectors) ---
//   {2, 6}, // 连接左上瓣
//   {3, 7}, // 连接右上瓣
//   {4, 8}, // 连接左侧
//   {5, 9}  // 连接右侧
// };

// ----------------------------------------------------------------
// 动画: 3D 旋转立方体
// ----------------------------------------------------------------
void Animation_3DCube(void)
{
    float rot_points[8][3]; // 旋转后的点
    int scr_points[8][2];   // 屏幕上的 2D 点
    int i;

    // 旋转速度
    cube_angle_x += 0.05;
    cube_angle_y += 0.08;
    cube_angle_z += 0.03;

    for(i=0; i<8; i++) {
        float x = vertices[i][0];
        float y = vertices[i][1];
        float z = vertices[i][2];
        float temp_x, temp_y, temp_z;

        // 绕 X 轴旋转
        temp_y = y * cos(cube_angle_x) - z * sin(cube_angle_x);
        temp_z = y * sin(cube_angle_x) + z * cos(cube_angle_x);
        y = temp_y; z = temp_z;

        // 绕 Y 轴旋转
        temp_x = x * cos(cube_angle_y) + z * sin(cube_angle_y);
        temp_z = -x * sin(cube_angle_y) + z * cos(cube_angle_y);
        x = temp_x; z = temp_z;

        // 绕 Z 轴旋转
        temp_x = x * cos(cube_angle_z) - y * sin(cube_angle_z);
        temp_y = x * sin(cube_angle_z) + y * cos(cube_angle_z);
        x = temp_x; y = temp_y;

        // 3D 透视投影到 2D
        // z + 60 相当于把物体推远，防止除以0
        float scale = 100 / (z + 60);
        scr_points[i][0] = (int)(x * scale) + 64; // +64 移到屏幕中心 X
        scr_points[i][1] = (int)(y * scale) + 36; // +36 移到屏幕中心 Y
    }

    // 绘制连线
    for(i=0; i<12; i++) {
        OLED_DrawLine(
            scr_points[edges[i][0]][0], scr_points[edges[i][0]][1],
            scr_points[edges[i][1]][0], scr_points[edges[i][1]][1]
        );
    }
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

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
  MX_I2C1_Init();
  /* USER CODE BEGIN 2 */
  HAL_GPIO_TogglePin(GPIOC,GPIO_PIN_6);

  // 1. 初始化 OLED
  // OLED_Init();
  u8g2_Init_STM32(&u8g2);
  while (1)
  {
    // --- 开始一帧 ---

    // 1. 清空缓冲区 (类似于之前的 memset)
    u8g2_ClearBuffer(&u8g2);

    // 2. 绘制内容

    // 设置字体 (u8g2 有几千种字体，ncen 表示不包含生僻字，B 表示粗体，14是高度)
    u8g2_SetFont(&u8g2, u8g2_font_ncenB14_tr);
    u8g2_DrawStr(&u8g2, 0, 15, "Hello World");

    // 画个空心框
    u8g2_DrawFrame(&u8g2, 0, 20, 50, 30);

    // 画个实心圆
    u8g2_DrawDisc(&u8g2, 100, 40, 10, U8G2_DRAW_ALL);

    // 3. 发送缓冲区到屏幕 (类似于之前的 OLED_Refresh_Gram)
    u8g2_SendBuffer(&u8g2);

    // --- 结束一帧 ---

    HAL_Delay(100);
  }

  // 2. 记录当前时间
  lastTime = HAL_GetTick();

  // JumpToApplication();
  uint32_t switch_timer = 0;
  uint8_t anim_mode = 1; // 0: 方块, 1: 立方体
  Init_Heart_Mesh();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    /* --- 1. 清空显存 (极快) --- */
    OLED_Clear();

    /* --- 2. 绘制 UI 信息 --- */
    OLED_ShowString(1, 1, "STM32 G4");
    OLED_ShowString(1, 11, "FPS:");
    OLED_ShowNum(1, 14, fps, 3);

    /* --- 3. 绘制动画 --- */
    if (anim_mode == 0) {
      OLED_ShowString(2, 11, "BOUNCE");

    } else {
      OLED_ShowString(2, 11, "3D CUBE");
      Animation_3DCube();
      // Animation_BeatingHeart();
      // Animation_RefinedHeart();
    }

    /* --- 4. 刷新到屏幕 (Burst Write) --- */
    OLED_Refresh_Gram();
    /* --- 5. 逻辑控制 --- */
    // FPS 计算
    frameCount++;
    if (HAL_GetTick() - lastTime >= 1000) {
      fps = frameCount;
      frameCount = 0;
      lastTime = HAL_GetTick();
    }

    // 每 5 秒切换一次动画
    // if (HAL_GetTick() - switch_timer > 5000) {
    //   anim_mode = !anim_mode;
    //   switch_timer = HAL_GetTick();
    //   // 清屏过渡一下
    //   OLED_Clear();
    //   OLED_Refresh_Gram();
    // }
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1_BOOST);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = RCC_PLLM_DIV4;
  RCC_OscInitStruct.PLL.PLLN = 85;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM17 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */

  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM17)
  {
    HAL_IncTick();
  }
  /* USER CODE BEGIN Callback 1 */

  /* USER CODE END Callback 1 */
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
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
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
