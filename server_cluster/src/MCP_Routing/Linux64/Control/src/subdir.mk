################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../Control/src/ControlManager.cpp \
../Control/src/ControlManagerImpl.cpp \
../Control/src/FilterTags.cpp \
../Control/src/RemoteServerStatus.cpp \
../Control/src/SubCoveringFilterEventListener.cpp \
../Control/src/SubCoveringFilterPublisher.cpp \
../Control/src/SubCoveringFilterPublisherImpl.cpp \
../Control/src/SubCoveringFilterWireFormat.cpp \
../Control/src/ViewKeeper.cpp 

OBJS += \
./Control/src/ControlManager.o \
./Control/src/ControlManagerImpl.o \
./Control/src/FilterTags.o \
./Control/src/RemoteServerStatus.o \
./Control/src/SubCoveringFilterEventListener.o \
./Control/src/SubCoveringFilterPublisher.o \
./Control/src/SubCoveringFilterPublisherImpl.o \
./Control/src/SubCoveringFilterWireFormat.o \
./Control/src/ViewKeeper.o 

CPP_DEPS += \
./Control/src/ControlManager.d \
./Control/src/ControlManagerImpl.d \
./Control/src/FilterTags.d \
./Control/src/RemoteServerStatus.d \
./Control/src/SubCoveringFilterEventListener.d \
./Control/src/SubCoveringFilterPublisher.d \
./Control/src/SubCoveringFilterPublisherImpl.d \
./Control/src/SubCoveringFilterWireFormat.d \
./Control/src/ViewKeeper.d 


# Each subdirectory must supply rules for building sources it contributes
Control/src/%.o: ../Control/src/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -DSPDR_LINUX -DRUM_UNIX -DBOOST_THREAD_DYN_LINK -DBOOST_DATE_TIME_DYN_LINK -I"${ISM_UTILS_INCLUDE_DIR}" -I"${BOOST_INC_DIR}" -I"${MCC_WORKSPACE_DIR}/MCP_Routing/API/include" -I"${MCC_WORKSPACE_DIR}/MCP_Routing/Util/include" -I"${MCC_WORKSPACE_DIR}/MCP_Routing/Control/include" -I"${MCC_WORKSPACE_DIR}/MCP_Routing/Routing/include" -I"${SPDR_WORKSPACE_DIR}/SpiderCastCpp/API/include" -I"${SPDR_WORKSPACE_DIR}/SpiderCastCpp/Trace/include" -O3 -Wall -c -fmessage-length=0 -fPIC -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


