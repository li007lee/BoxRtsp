################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/cJSON.c \
../src/client_list.c \
../src/dev_list.c \
../src/dev_opt.c \
../src/libevent_server.c \
../src/list.c \
../src/main.c \
../src/video_data_list.c 

OBJS += \
./src/cJSON.o \
./src/client_list.o \
./src/dev_list.o \
./src/dev_opt.o \
./src/libevent_server.o \
./src/list.o \
./src/main.o \
./src/video_data_list.o 

C_DEPS += \
./src/cJSON.d \
./src/client_list.d \
./src/dev_list.d \
./src/dev_opt.d \
./src/libevent_server.d \
./src/list.d \
./src/main.d \
./src/video_data_list.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


