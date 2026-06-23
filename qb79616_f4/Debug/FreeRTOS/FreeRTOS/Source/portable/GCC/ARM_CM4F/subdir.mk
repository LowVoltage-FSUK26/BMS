################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (12.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../FreeRTOS/FreeRTOS/Source/portable/GCC/ARM_CM4F/port.c 

OBJS += \
./FreeRTOS/FreeRTOS/Source/portable/GCC/ARM_CM4F/port.o 

C_DEPS += \
./FreeRTOS/FreeRTOS/Source/portable/GCC/ARM_CM4F/port.d 


# Each subdirectory must supply rules for building sources it contributes
FreeRTOS/FreeRTOS/Source/portable/GCC/ARM_CM4F/%.o FreeRTOS/FreeRTOS/Source/portable/GCC/ARM_CM4F/%.su FreeRTOS/FreeRTOS/Source/portable/GCC/ARM_CM4F/%.cyclo: ../FreeRTOS/FreeRTOS/Source/portable/GCC/ARM_CM4F/%.c FreeRTOS/FreeRTOS/Source/portable/GCC/ARM_CM4F/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F446xx -c -I../Core/Inc -I"D:/Mina/Racing Team Tasks/2025-2026 Season/BMS_GitHub_Repo/BMS/qb79616_f4/Config" -I"D:/Mina/Racing Team Tasks/2025-2026 Season/BMS_GitHub_Repo/BMS/qb79616_f4/FreeRTOS/FreeRTOS/Source/include" -I"D:/Mina/Racing Team Tasks/2025-2026 Season/BMS_GitHub_Repo/BMS/qb79616_f4/BMS_Tests" -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -I"D:/Mina/Racing Team Tasks/2025-2026 Season/BMS_GitHub_Repo/BMS/qb79616_f4/FreeRTOS/FreeRTOS/Source/portable/GCC/ARM_CM4F" -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-FreeRTOS-2f-FreeRTOS-2f-Source-2f-portable-2f-GCC-2f-ARM_CM4F

clean-FreeRTOS-2f-FreeRTOS-2f-Source-2f-portable-2f-GCC-2f-ARM_CM4F:
	-$(RM) ./FreeRTOS/FreeRTOS/Source/portable/GCC/ARM_CM4F/port.cyclo ./FreeRTOS/FreeRTOS/Source/portable/GCC/ARM_CM4F/port.d ./FreeRTOS/FreeRTOS/Source/portable/GCC/ARM_CM4F/port.o ./FreeRTOS/FreeRTOS/Source/portable/GCC/ARM_CM4F/port.su

.PHONY: clean-FreeRTOS-2f-FreeRTOS-2f-Source-2f-portable-2f-GCC-2f-ARM_CM4F

