################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../Socket/SocketHandler.cpp \
../Socket/SocketMsgQueue.cpp \
../Socket/socket.cpp 

OBJS += \
./Socket/SocketHandler.o \
./Socket/SocketMsgQueue.o \
./Socket/socket.o 

CPP_DEPS += \
./Socket/SocketHandler.d \
./Socket/SocketMsgQueue.d \
./Socket/socket.d 


# Each subdirectory must supply rules for building sources it contributes
Socket/%.o: ../Socket/%.cpp Socket/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross G++ Compiler'
	arm-linux-gnueabihf-g++ -rdynamic -O0 -g3 -Wall -c -rdynamic -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


