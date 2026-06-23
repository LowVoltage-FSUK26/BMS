################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (12.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../FreeRTOS/FreeRTOS/Source/portable/MemMang/heap_1.c 

OBJS += \
./FreeRTOS/FreeRTOS/Source/portable/MemMang/heap_1.o 

C_DEPS += \
./FreeRTOS/FreeRTOS/Source/portable/MemMang/heap_1.d 


# Each subdirectory must supply rules for building sources it contributes
FreeRTOS/FreeRTOS/Source/portable/MemMang/%.o FreeRTOS/FreeRTOS/Source/portable/MemMang/%.su FreeRTOS/FreeRTOS/Source/portable/MemMang/%.cyclo: ../FreeRTOS/FreeRTOS/Source/portable/MemMang/%.c FreeRTOS/FreeRTOS/Source/portable/MemMang/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F446xx -c -I../Core/Inc -I"F:/MOHAMMED ESSAM/Low Voltage S2/BMS/qb79616_projects/qb79616_f4/FreeRTOS/FreeRTOS/Source/portable/GCC/ARM_CM3" -I"F:/MOHAMMED ESSAM/Low Voltage S2/BMS/qb79616_projects/qb79616_f4/Config" -I"F:/MOHAMMED ESSAM/Low Voltage S2/BMS/qb79616_projects/qb79616_f4/FreeRTOS/FreeRTOS/Source/include" -I"F:/MOHAMMED ESSAM/Low Voltage S2/BMS/qb79616_projects/qb79616_f4/BMS_Tests" -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-FreeRTOS-2f-FreeRTOS-2f-Source-2f-portable-2f-MemMang

clean-FreeRTOS-2f-FreeRTOS-2f-Source-2f-portable-2f-MemMang:
	-$(RM) ./FreeRTOS/FreeRTOS/Source/portable/MemMang/heap_1.cyclo ./FreeRTOS/FreeRTOS/Source/portable/MemMang/heap_1.d ./FreeRTOS/FreeRTOS/Source/portable/MemMang/heap_1.o ./FreeRTOS/FreeRTOS/Source/portable/MemMang/heap_1.su

.PHONY: clean-FreeRTOS-2f-FreeRTOS-2f-Source-2f-portable-2f-MemMang

