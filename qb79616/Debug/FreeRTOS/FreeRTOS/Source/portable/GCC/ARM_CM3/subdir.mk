################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (12.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../FreeRTOS/FreeRTOS/Source/portable/GCC/ARM_CM3/port.c 

OBJS += \
./FreeRTOS/FreeRTOS/Source/portable/GCC/ARM_CM3/port.o 

C_DEPS += \
./FreeRTOS/FreeRTOS/Source/portable/GCC/ARM_CM3/port.d 


# Each subdirectory must supply rules for building sources it contributes
FreeRTOS/FreeRTOS/Source/portable/GCC/ARM_CM3/%.o FreeRTOS/FreeRTOS/Source/portable/GCC/ARM_CM3/%.su FreeRTOS/FreeRTOS/Source/portable/GCC/ARM_CM3/%.cyclo: ../FreeRTOS/FreeRTOS/Source/portable/GCC/ARM_CM3/%.c FreeRTOS/FreeRTOS/Source/portable/GCC/ARM_CM3/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m3 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F103xB -c -I../Core/Inc -I../Drivers/STM32F1xx_HAL_Driver/Inc/Legacy -I../Drivers/STM32F1xx_HAL_Driver/Inc -I../Drivers/CMSIS/Device/ST/STM32F1xx/Include -I../Drivers/CMSIS/Include -I"D:/Mina/Racing Team Tasks/2025-2026 Season/BMS_GitHub_Repo/BMS/qb79616/Config" -I"D:/Mina/Racing Team Tasks/2025-2026 Season/BMS_GitHub_Repo/BMS/qb79616/FreeRTOS/FreeRTOS/Source/include" -I"D:/Mina/Racing Team Tasks/2025-2026 Season/BMS_GitHub_Repo/BMS/qb79616/FreeRTOS/FreeRTOS/Source/portable/GCC/ARM_CM3" -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"

clean: clean-FreeRTOS-2f-FreeRTOS-2f-Source-2f-portable-2f-GCC-2f-ARM_CM3

clean-FreeRTOS-2f-FreeRTOS-2f-Source-2f-portable-2f-GCC-2f-ARM_CM3:
	-$(RM) ./FreeRTOS/FreeRTOS/Source/portable/GCC/ARM_CM3/port.cyclo ./FreeRTOS/FreeRTOS/Source/portable/GCC/ARM_CM3/port.d ./FreeRTOS/FreeRTOS/Source/portable/GCC/ARM_CM3/port.o ./FreeRTOS/FreeRTOS/Source/portable/GCC/ARM_CM3/port.su

.PHONY: clean-FreeRTOS-2f-FreeRTOS-2f-Source-2f-portable-2f-GCC-2f-ARM_CM3

