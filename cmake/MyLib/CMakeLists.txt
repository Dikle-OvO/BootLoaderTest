cmake_minimum_required(VERSION 3.22)
# Enable CMake support for ASM and C languages
enable_language(C ASM)

# 指定头文件包含目录
set(MyLib_Include_Dirs
        ${CMAKE_CURRENT_SOURCE_DIR}/../../Hardware/OLED

)

# 指定源文件
set(MyLib_Src
        ${CMAKE_CURRENT_SOURCE_DIR}/../../Hardware/OLED/OLED.c
)

add_library(MyLib OBJECT) # 添加库目标
target_include_directories(MyLib PUBLIC  ${MyLib_Include_Dirs}) # 为库设定可继承的include目录
target_sources(MyLib PRIVATE ${MyLib_Src}) # 为库设定源文件

target_link_libraries(MyLib PUBLIC stm32cubemx) # 为库链接BSP接口库，以调用BSP提供的函数