################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../PDM2PCM/App/pdm2pcm.c 

OBJS += \
./PDM2PCM/App/pdm2pcm.o 

C_DEPS += \
./PDM2PCM/App/pdm2pcm.d 


# Each subdirectory must supply rules for building sources it contributes
PDM2PCM/App/%.o PDM2PCM/App/%.su PDM2PCM/App/%.cyclo: ../PDM2PCM/App/%.c PDM2PCM/App/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F469xx -c -I../Core/Inc -I../USB_HOST/App -I../USB_HOST/Target -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Middlewares/ST/STM32_USB_Host_Library/Core/Inc -I../Middlewares/ST/STM32_USB_Host_Library/Class/CDC/Inc -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -I"C:/Users/petro/STM32CubeIDE/workspace_1.7.0/f469_start/f469_start/PDM2PCM/App" -I"C:/Users/petro/STM32CubeIDE/workspace_1.7.0/f469_start/f469_start/Middlewares/ST/STM32_Audio/Addons/PDM/Inc" -I"C:/Users/petro/STM32CubeIDE/workspace_1.7.0/f469_start/f469_start/Middlewares/ST/STM32_Audio/Addons/PDM/Lib" -I"C:/Users/petro/STM32CubeIDE/workspace_1.7.0/f469_start/f469_start/Middlewares" -I"C:/Users/petro/STM32CubeIDE/workspace_1.7.0/f469_start/f469_start/PDM2PCM" -I../Drivers/CMSIS_DSP/Include -I../Drivers/CMSIS_DSP/PrivateInclude -O0 -ffunction-sections -fdata-sections -Wall -Wextra -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-PDM2PCM-2f-App

clean-PDM2PCM-2f-App:
	-$(RM) ./PDM2PCM/App/pdm2pcm.cyclo ./PDM2PCM/App/pdm2pcm.d ./PDM2PCM/App/pdm2pcm.o ./PDM2PCM/App/pdm2pcm.su

.PHONY: clean-PDM2PCM-2f-App

