################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/Ea.c \
../src/Ea_Cfg.c \
../src/MemIf.c \
../src/NvM.c \
../src/NvM_Cfg.c \
../src/eeprom_ex2.c \
../src/main.c 

OBJS += \
./src/Ea.o \
./src/Ea_Cfg.o \
./src/MemIf.o \
./src/NvM.o \
./src/NvM_Cfg.o \
./src/eeprom_ex2.o \
./src/main.o 

C_DEPS += \
./src/Ea.d \
./src/Ea_Cfg.d \
./src/MemIf.d \
./src/NvM.d \
./src/NvM_Cfg.d \
./src/eeprom_ex2.d \
./src/main.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Standard S32DS C Compiler'
	arm-none-eabi-gcc "@src/Ea.args" -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


