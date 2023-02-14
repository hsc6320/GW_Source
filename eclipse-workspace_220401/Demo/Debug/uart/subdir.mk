################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../uart/uart.cpp 

OBJS += \
./uart/uart.o 

CPP_DEPS += \
./uart/uart.d 


# Each subdirectory must supply rules for building sources it contributes
uart/%.o: ../uart/%.cpp uart/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross G++ Compiler'
	arm-linux-gnueabihf-g++ -rdynamic -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


