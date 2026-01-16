#include "OLED.h"
#include "i2c.h"
#include "string.h" // 需要用到 memset
#include "OLED_Font.h"
#include "math.h"

/* 显存数组：8页 * 128列 = 1024字节 */
uint8_t OLED_GRAM[8][128];

/* 外部 I2C 句柄 */
extern I2C_HandleTypeDef hi2c1;

// OLED 初始化序列 (保持不变)
void OLED_WriteCommand(uint8_t Command)
{
    HAL_I2C_Mem_Write(&hi2c1, 0x78, 0x00, I2C_MEMADD_SIZE_8BIT, &Command, 1, 100);
}

// 现在写数据只改内存，不再直接发 I2C
void OLED_DrawPoint_Ram(int x, int y, uint8_t t)
{
    // 严格的边界检查
    if (x < 0 || x > 127 || y < 0 || y > 63) return;

    uint8_t pos, bx, temp = 0;
    pos = 7 - y / 8;
    bx = y % 8;
    temp = 1 << (7 - bx);

    if (t) OLED_GRAM[pos][x] |= temp;
    else   OLED_GRAM[pos][x] &= ~temp;
}

// -----------------------------------------------------
// 核心函数：将显存一次性刷到屏幕 (Burst Write)
// -----------------------------------------------------
void OLED_Refresh_Gram(void)
{
    uint8_t i;
    for (i = 0; i < 8; i++)
    {
        // 设置光标到每一页的开头
        OLED_WriteCommand(0xB0 + i); // 设置页地址
        OLED_WriteCommand(0x00);     // 设置列地址低位
        OLED_WriteCommand(0x10);     // 设置列地址高位

        // 暴力传输：一页 128 字节直接发，利用 I2C 连续写入特性
        HAL_I2C_Mem_Write(&hi2c1, 0x78, 0x40, I2C_MEMADD_SIZE_8BIT, OLED_GRAM[i], 128, 100);
    }
}

// 清屏：只清空内存，不发 I2C，极快
void OLED_Clear(void)
{
    memset(OLED_GRAM, 0, sizeof(OLED_GRAM));
}

// 显示字符：修改内存
void OLED_ShowChar(uint8_t Line, uint8_t Column, char Char)
{
    uint8_t *pSrc = (uint8_t *)OLED_F8x16[Char - ' ']; // 获取字模数据指针

    // 计算显存偏移
    // Line 1~4 -> Page 0,2,4,6 (因为是16像素高，占2页)
    uint8_t page = (Line - 1) * 2;
    uint8_t col_start = (Column - 1) * 8;

    // 拷贝上半部分 (8字节) 到显存
    memcpy(&OLED_GRAM[page][col_start], pSrc, 8);

    // 拷贝下半部分 (8字节) 到显存的下一页
    memcpy(&OLED_GRAM[page + 1][col_start], pSrc + 8, 8);
}

// 显示字符串 (逻辑不变，只是调用的 ShowChar 变快了)
void OLED_ShowString(uint8_t Line, uint8_t Column, char *String)
{
    uint8_t i;
    for (i = 0; String[i] != '\0'; i++)
    {
        OLED_ShowChar(Line, Column + i, String[i]);
    }
}

// 辅助函数：次方
uint32_t OLED_Pow(uint32_t X, uint32_t Y)
{
    uint32_t Result = 1;
    while (Y--) Result *= X;
    return Result;
}

// 显示数字
void OLED_ShowNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length)
{
    uint8_t i;
    for (i = 0; i < Length; i++)
    {
        OLED_ShowChar(Line, Column + i, Number / OLED_Pow(10, Length - i - 1) % 10 + '0');
    }
}

// 初始化
void OLED_Init(void)
{
    HAL_Delay(200);
    // ... (你的初始化命令序列保持不变，复制你原来的即可) ...
    OLED_WriteCommand(0xAE); // 关闭显示
    OLED_WriteCommand(0x20); // 设置内存地址模式
    OLED_WriteCommand(0x02); // 02: 页地址模式 (Page Addressing Mode)
    // ... 其他初始化命令 ...
    OLED_WriteCommand(0x8D); OLED_WriteCommand(0x14); // 充电泵
    OLED_WriteCommand(0xAF); // 开启显示

    OLED_Clear();
    OLED_Refresh_Gram(); // 初始刷屏
}

// 画线函数 (Bresenham算法，极快)
void OLED_DrawLine(int x1, int y1, int x2, int y2)
{
    // 修改 1：将 t 改为 int 或 uint16_t，防止 255 溢出死循环
    uint16_t t;
    int xerr = 0, yerr = 0, delta_x, delta_y, distance;
    int incx, incy, uRow, uCol;

    delta_x = x2 - x1;
    delta_y = y2 - y1;
    uRow = x1;
    uCol = y1;

    if (delta_x > 0) incx = 1;
    else if (delta_x == 0) incx = 0;
    else { incx = -1; delta_x = -delta_x; }

    if (delta_y > 0) incy = 1;
    else if (delta_y == 0) incy = 0;
    else { incy = -1; delta_y = -delta_y; }

    if (delta_x > delta_y) distance = delta_x;
    else distance = delta_y;

    for (t = 0; t <= distance + 1; t++)
    {
        // 建议加上边界检查，防止 3D 坐标飞出屏幕导致乱写内存
        // 虽然 OLED_DrawPoint_Ram 内部可能有了，但这里加更安全
        OLED_DrawPoint_Ram(uRow, uCol, 1);

        xerr += delta_x;
        yerr += delta_y;
        if (xerr > distance) { xerr -= distance; uRow += incx; }
        if (yerr > distance) { yerr -= distance; uCol += incy; }
    }
}

// 画矩形 (空心)
void OLED_DrawRectangle(uint8_t x, uint8_t y, uint8_t w, uint8_t h)
{
    OLED_DrawLine(x, y, x + w, y);
    OLED_DrawLine(x, y + h, x + w, y + h);
    OLED_DrawLine(x, y, x, y + h);
    OLED_DrawLine(x + w, y, x + w, y + h);
}

// 画实心爱心
// x_center, y_center: 爱心中心坐标 (推荐 64, 36)
// scale: 缩放比例 (推荐 1.0 ~ 2.5)
void OLED_DrawHeart(int x_center, int y_center, float scale)
{
    float t;
    // 步长越小，边缘越平滑，填充越致密
    // 0.05f 足够保证没有缝隙
    for (t = 0; t < 6.28f; t += 0.05f)
    {
        // 1. 基础心形公式
        // 注意：这里全部用 float 后缀 (f) 确保使用 FPU
        float x = 16.0f * powf(sinf(t), 3);
        float y = 13.0f * cosf(t) - 5.0f * cosf(2 * t) - 2.0f * cosf(3 * t) - cosf(4 * t);

        // 2. 缩放与平移
        // 注意：屏幕 Y 轴向下是正，而数学坐标 Y 轴向上是正，所以 Y 要用减法反转
        int px = x_center + (int)(x * scale);
        int py = y_center - (int)(y * scale);

        // 3. 绘制
        // 从中心点向边缘画线，形成实心效果
        OLED_DrawLine(x_center, y_center, px, py);
    }
}

void Animation_BeatingHeart(void)
{
    // 用于控制心跳节奏的时间变量
    static float time = 0.0f;

    // 心跳速度
    time += 0.15f;
    if (time > 6.28f) time = 0;

    // 计算当前的缩放比例
    // sinf(time) 产生 -1 到 1 的波形
    // 我们将其映射到 1.2 (最小) 到 2.2 (最大) 之间
    // fabsf 也可以用来制造更急促的“收缩感”
    float scale = 1.7f + 0.5f * sinf(time);

    // 绘制爱心 (居中显示)
    // Y坐标选 36 是因为爱心形状上面比较宽，稍微下移一点视觉更居中
    OLED_DrawHeart(64, 36, scale);

    // 在中间写个字
    OLED_ShowString(55, 32, "LOVE");
}

/* --- 3D 精致爱心网格配置 --- */
// 经度切片数 (垂直竖线)，越大越圆，建议 12~20
#define HEART_SLICES 16
// 纬度层数 (水平横线)，建议 10~15
#define HEART_STACKS 12

// 总顶点数
#define TOTAL_VERTS (HEART_SLICES * HEART_STACKS)

// 存储原始 3D 坐标 (静态模型)
float heart_mesh[TOTAL_VERTS][3];

// 存储计算后的 2D 屏幕坐标 (用于绘图)
int scr_points[TOTAL_VERTS][2];

// 旋转角度
float heart_angle_y = 0.0f;
float heart_angle_x = 0.0f;

// 初始化爱心网格数据 (只运行一次)
void Init_Heart_Mesh(void)
{
    int i, j;
    int idx = 0;
    float t, p;

    // t 从 0 到 PI (从底到顶)
    // p 从 0 到 2PI (绕Y轴一圈)

    for (i = 0; i < HEART_STACKS; i++) {
        // 归一化 t，范围 0.0 ~ 3.14
        // 稍微掐头去尾(0.1 ~ 3.0)防止两极点重合导致线太密
        t = 0.1f + (3.0f * i / (HEART_STACKS - 1));

        for (j = 0; j < HEART_SLICES; j++) {
            // 归一化 p，范围 0.0 ~ 6.28
            p = 6.28318f * j / HEART_SLICES;

            // 预计算三角函数
            float sint = sinf(t);
            float cost = cosf(t);
            float sinp = sinf(p);
            float cosp = cosf(p);
            float cost2 = cosf(2*t);
            float cost3 = cosf(3*t);
            float cost4 = cosf(4*t);

            // 3D 心形公式 (倍率调整大小)
            // Y 轴反转一下，否则爱心是倒着的
            float x = 12.0f * (sint * sint * sint) * sinp;
            float y = -(13.0f * cost - 5.0f * cost2 - 2.0f * cost3 - cost4);
            float z = 5.0f * (sint * sint * sint) * cosp;

            // 存入数组
            heart_mesh[idx][0] = x;
            heart_mesh[idx][1] = y;
            heart_mesh[idx][2] = z;
            idx++;
        }
    }
}

void Animation_RefinedHeart(void)
{
    int i, j;

    // 1. 更新旋转角度
    heart_angle_y += 0.03f; // 自转
    // heart_angle_x = -0.6f;
    // heart_angle_z = -0.6f;
    heart_angle_x = 0.5f * sinf(HAL_GetTick() * 0.002f); // 也就是轻轻点头的效果

    // 2. 预计算旋转矩阵
    float cy = cosf(heart_angle_y);
    float sy = sinf(heart_angle_y);
    float cx = cosf(heart_angle_x);
    float sx = sinf(heart_angle_x);

    // 3. 顶点变换 (旋转 + 投影)
    for (i = 0; i < TOTAL_VERTS; i++) {
        float x = heart_mesh[i][0];
        float y = heart_mesh[i][1];
        float z = heart_mesh[i][2];
        float tx, ty, tz;

        // 绕 Y 轴旋转
        tx = x * cy + z * sy;
        tz = -x * sy + z * cy;
        x = tx; z = tz;

        // 绕 X 轴旋转
        ty = y * cx - z * sx;
        tz = y * sx + z * cx;
        y = ty; z = tz;

        // 透视投影
        float dist = z + 60.0f; // 推远一点
        if (dist < 1.0f) dist = 1.0f;
        float scale = 110.0f / dist; // 200是视野系数，越大看起来越近

        // 屏幕坐标转换
        int px = (int)(x * scale) + 64;
        int py = (int)(y * scale) + 32;

        // 边界限制 (Clamp) 防止溢出画线
        if (px < -100) px = -100; if (px > 228) px = 228;
        if (py < -100) py = -100; if (py > 164) py = 164;

        scr_points[i][0] = px;
        scr_points[i][1] = py;
    }

    // 4. 绘制网格连线 (自动编织)
    for (i = 0; i < HEART_STACKS; i++) {
        for (j = 0; j < HEART_SLICES; j++) {

            // 当前点的索引
            int current = i * HEART_SLICES + j;

            // --- 画纬线 (横向连接) ---
            // 连接到下一个点 (j+1)，如果是最后一个点，绕回每层的第一个点 (j=0)
            int next_lon = i * HEART_SLICES + ((j + 1) % HEART_SLICES);
            OLED_DrawLine(
                scr_points[current][0], scr_points[current][1],
                scr_points[next_lon][0], scr_points[next_lon][1]
            );

            // --- 画经线 (纵向连接) ---
            // 连接到下一层同位置的点 (i+1)
            if (i < HEART_STACKS - 1) {
                int next_lat = (i + 1) * HEART_SLICES + j;
                OLED_DrawLine(
                    scr_points[current][0], scr_points[current][1],
                    scr_points[next_lat][0], scr_points[next_lat][1]
                );
            }
        }
    }
}