################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (12.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../FreeRTOS/FreeRTOS/Source/portable/GCC/ARM_CM4/port.c 

OBJS += \
./FreeRTOS/FreeRTOS/Source/portable/GCC/ARM_CM4/port.o 

C_DEPS += \
./FreeRTOS/FreeRTOS/Source/portable/GCC/ARM_CM4/port.d 


# Each subdirectory must supply rules for building sources it contributes
FreeRTOS/FreeRTOS/Source/portable/GCC/ARM_CM4/%.o FreeRTOS/FreeRTOS/Source/portable/GCC/ARM_CM4/%.su FreeRTOS/FreeRTOS/Source/portable/GCC/ARM_CM4/%.cyclo: ../FreeRTOS/FreeRTOS/Source/portable/GCC/ARM_CM4/%.c FreeRTOS/FreeRTOS/Source/portable/GCC/ARM_CM4/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F446xx -c -I../Core/Inc -I"C:/Users/Administrator/Documents/joedocuments/FSUK2026/BMS/qb79616_f4/Config" -I"C:/Users/Administrator/Documents/joedocuments/FSUK2026/BMS/qb79616_f4/FreeRTOS/FreeRTOS/Source/include" -I"C:/Users/Administrator/Documents/joedocuments/FSUK2026/BMS/qb79616_f4/BMS_Tests" -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -I"C:/Users/Administrator/Documents/joedocuments/FSUK2026/BMS/qb79616_f4/FreeRTOS/FreeRTOS/Source/portable/GCC/ARM_CM4" -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-FreeRTOS-2f-FreeRTOS-2f-Source-2f-portable-2f-GCC-2f-ARM_CM4

clean-FreeRTOS-2f-FreeRTOS-2f-Source-2f-portable-2f-GCC-2f-ARM_CM4:
	-$(RM) ./FreeRTOS/FreeRTOS/Source/portable/GCC/ARM_CM4/port.cyclo ./FreeRTOS/FreeRTOS/Source/portable/GCC/ARM_CM4/port.d ./FreeRTOS/FreeRTOS/Source/portable/GCC/ARM_CM4/port.o ./FreeRTOS/FreeRTOS/Source/portable/GCC/ARM_CM4/port.su

.PHONY: clean-FreeRTOS-2f-FreeRTOS-2f-Source-2f-portable-2f-GCC-2f-ARM_CM4

