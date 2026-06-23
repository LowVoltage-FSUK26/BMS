################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (12.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../BMS_Tests/BMS_tests.c 

OBJS += \
./BMS_Tests/BMS_tests.o 

C_DEPS += \
./BMS_Tests/BMS_tests.d 


# Each subdirectory must supply rules for building sources it contributes
BMS_Tests/%.o BMS_Tests/%.su BMS_Tests/%.cyclo: ../BMS_Tests/%.c BMS_Tests/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F446xx -c -I../Core/Inc -I"D:/Mina/Racing Team Tasks/2025-2026 Season/BMS_GitHub_Repo/BMS/qb79616_f4/Config" -I"D:/Mina/Racing Team Tasks/2025-2026 Season/BMS_GitHub_Repo/BMS/qb79616_f4/FreeRTOS/FreeRTOS/Source/include" -I"D:/Mina/Racing Team Tasks/2025-2026 Season/BMS_GitHub_Repo/BMS/qb79616_f4/BMS_Tests" -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -I"D:/Mina/Racing Team Tasks/2025-2026 Season/BMS_GitHub_Repo/BMS/qb79616_f4/FreeRTOS/FreeRTOS/Source/portable/GCC/ARM_CM4F" -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-BMS_Tests

clean-BMS_Tests:
	-$(RM) ./BMS_Tests/BMS_tests.cyclo ./BMS_Tests/BMS_tests.d ./BMS_Tests/BMS_tests.o ./BMS_Tests/BMS_tests.su

.PHONY: clean-BMS_Tests

