################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../Util/src/AbstractTask.cpp \
../Util/src/ByteBuffer.cpp \
../Util/src/CyclicFileLogger.cpp \
../Util/src/HasErrorCode.cpp \
../Util/src/MCPExceptions.cpp \
../Util/src/SubscriptionPattern.cpp \
../Util/src/TaskExecutor.cpp \
../Util/src/Thread.cpp 

C_SRCS += \
../Util/src/MurmurHash3.c \
../Util/src/city_c.c \
../Util/src/hashFunction.c 

OBJS += \
./Util/src/AbstractTask.o \
./Util/src/ByteBuffer.o \
./Util/src/CyclicFileLogger.o \
./Util/src/HasErrorCode.o \
./Util/src/MCPExceptions.o \
./Util/src/MurmurHash3.o \
./Util/src/SubscriptionPattern.o \
./Util/src/TaskExecutor.o \
./Util/src/Thread.o \
./Util/src/city_c.o \
./Util/src/hashFunction.o 

C_DEPS += \
./Util/src/MurmurHash3.d \
./Util/src/city_c.d \
./Util/src/hashFunction.d 

CPP_DEPS += \
./Util/src/AbstractTask.d \
./Util/src/ByteBuffer.d \
./Util/src/CyclicFileLogger.d \
./Util/src/HasErrorCode.d \
./Util/src/MCPExceptions.d \
./Util/src/SubscriptionPattern.d \
./Util/src/TaskExecutor.d \
./Util/src/Thread.d 


# Each subdirectory must supply rules for building sources it contributes
Util/src/%.o: ../Util/src/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -DSPDR_LINUX -DRUM_UNIX -DBOOST_THREAD_DYN_LINK -DBOOST_DATE_TIME_DYN_LINK -I"${ISM_UTILS_INCLUDE_DIR}" -I"${BOOST_INC_DIR}" -I"${MCC_WORKSPACE_DIR}/MCP_Routing/API/include" -I"${MCC_WORKSPACE_DIR}/MCP_Routing/Util/include" -I"${MCC_WORKSPACE_DIR}/MCP_Routing/Control/include" -I"${MCC_WORKSPACE_DIR}/MCP_Routing/Routing/include" -I"${SPDR_WORKSPACE_DIR}/SpiderCastCpp/API/include" -I"${SPDR_WORKSPACE_DIR}/SpiderCastCpp/Trace/include" -O3 -Wall -c -fmessage-length=0 -fPIC -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Util/src/%.o: ../Util/src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -std=gnu99 -I"${MCC_WORKSPACE_DIR}/MCP_Routing/Util/include" -I"${ISM_UTILS_INCLUDE_DIR}" -I"${MCC_WORKSPACE_DIR}/MCP_Routing/Control/include" -I"${MCC_WORKSPACE_DIR}/MCP_Routing/API/include" -I"${MCC_WORKSPACE_DIR}/MCP_Routing/Include/MCC" -I"${MCC_WORKSPACE_DIR}/MCP_Routing/Routing/include" -O3 -Wall -c -fmessage-length=0 -fPIC -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


