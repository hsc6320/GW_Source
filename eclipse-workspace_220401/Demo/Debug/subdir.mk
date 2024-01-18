################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../Main.cpp \
../MsgHandler.cpp \
../MsgQueue.cpp 

OBJS += \
./Main.o \
./MsgHandler.o \
./MsgQueue.o 

CPP_DEPS += \
./Main.d \
./MsgHandler.d \
./MsgQueue.d 


# Each subdirectory must supply rules for building sources it contributes
%.o: ../%.cpp subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross G++ Compiler'
	arm-linux-gnueabihf-g++ -rdynamic -O0 -g3 -Wall -c -rdynamic -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


