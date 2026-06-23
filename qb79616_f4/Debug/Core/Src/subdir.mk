################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (12.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/Src/BMS_Config.c \
../Core/Src/Balancing.c \
../Core/Src/Faults.c \
../Core/Src/GUI.c \
../Core/Src/Temperatures.c \
../Core/Src/Voltages.c \
../Core/Src/bq79616_V2.c \
../Core/Src/main.c \
../Core/Src/stm32f4xx_hal_msp.c \
../Core/Src/stm32f4xx_hal_timebase_tim.c \
../Core/Src/stm32f4xx_it.c \
../Core/Src/syscalls.c \
../Core/Src/sysmem.c \
../Core/Src/system_stm32f4xx.c 

OBJS += \
./Core/Src/BMS_Config.o \
./Core/Src/Balancing.o \
./Core/Src/Faults.o \
./Core/Src/GUI.o \
./Core/Src/Temperatures.o \
./Core/Src/Voltages.o \
./Core/Src/bq79616_V2.o \
./Core/Src/main.o \
./Core/Src/stm32f4xx_hal_msp.o \
./Core/Src/stm32f4xx_hal_timebase_tim.o \
./Core/Src/stm32f4xx_it.o \
./Core/Src/syscalls.o \
./Core/Src/sysmem.o \
./Core/Src/system_stm32f4xx.o 

C_DEPS += \
./Core/Src/BMS_Config.d \
./Core/Src/Balancing.d \
./Core/Src/Faults.d \
./Core/Src/GUI.d \
./Core/Src/Temperatures.d \
./Core/Src/Voltages.d \
./Core/Src/bq79616_V2.d \
./Core/Src/main.d \
./Core/Src/stm32f4xx_hal_msp.d \
./Core/Src/stm32f4xx_hal_timebase_tim.d \
./Core/Src/stm32f4xx_it.d \
./Core/Src/syscalls.d \
./Core/Src/sysmem.d \
./Core/Src/system_stm32f4xx.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Src/%.o Core/Src/%.su Core/Src/%.cyclo: ../Core/Src/%.c Core/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F446xx -c -I../Core/Inc -I"F:/MOHAMMED ESSAM/Low Voltage S2/BMS/qb79616_projects/qb79616_f4/FreeRTOS/FreeRTOS/Source/portable/GCC/ARM_CM3" -I"F:/MOHAMMED ESSAM/Low Voltage S2/BMS/qb79616_projects/qb79616_f4/Config" -I"F:/MOHAMMED ESSAM/Low Voltage S2/BMS/qb79616_projects/qb79616_f4/FreeRTOS/FreeRTOS/Source/include" -I"F:/MOHAMMED ESSAM/Low Voltage S2/BMS/qb79616_projects/qb79616_f4/BMS_Tests" -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Core-2f-Src

clean-Core-2f-Src:
	-$(RM) ./Core/Src/BMS_Config.cyclo ./Core/Src/BMS_Config.d ./Core/Src/BMS_Config.o ./Core/Src/BMS_Config.su ./Core/Src/Balancing.cyclo ./Core/Src/Balancing.d ./Core/Src/Balancing.o ./Core/Src/Balancing.su ./Core/Src/Faults.cyclo ./Core/Src/Faults.d ./Core/Src/Faults.o ./Core/Src/Faults.su ./Core/Src/GUI.cyclo ./Core/Src/GUI.d ./Core/Src/GUI.o ./Core/Src/GUI.su ./Core/Src/Temperatures.cyclo ./Core/Src/Temperatures.d ./Core/Src/Temperatures.o ./Core/Src/Temperatures.su ./Core/Src/Voltages.cyclo ./Core/Src/Voltages.d ./Core/Src/Voltages.o ./Core/Src/Voltages.su ./Core/Src/bq79616_V2.cyclo ./Core/Src/bq79616_V2.d ./Core/Src/bq79616_V2.o ./Core/Src/bq79616_V2.su ./Core/Src/main.cyclo ./Core/Src/main.d ./Core/Src/main.o ./Core/Src/main.su ./Core/Src/stm32f4xx_hal_msp.cyclo ./Core/Src/stm32f4xx_hal_msp.d ./Core/Src/stm32f4xx_hal_msp.o ./Core/Src/stm32f4xx_hal_msp.su ./Core/Src/stm32f4xx_hal_timebase_tim.cyclo ./Core/Src/stm32f4xx_hal_timebase_tim.d ./Core/Src/stm32f4xx_hal_timebase_tim.o ./Core/Src/stm32f4xx_hal_timebase_tim.su ./Core/Src/stm32f4xx_it.cyclo ./Core/Src/stm32f4xx_it.d ./Core/Src/stm32f4xx_it.o ./Core/Src/stm32f4xx_it.su ./Core/Src/syscalls.cyclo ./Core/Src/syscalls.d ./Core/Src/syscalls.o ./Core/Src/syscalls.su ./Core/Src/sysmem.cyclo ./Core/Src/sysmem.d ./Core/Src/sysmem.o ./Core/Src/sysmem.su ./Core/Src/system_stm32f4xx.cyclo ./Core/Src/system_stm32f4xx.d ./Core/Src/system_stm32f4xx.o ./Core/Src/system_stm32f4xx.su

.PHONY: clean-Core-2f-Src

