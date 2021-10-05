################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../API/src/EngineEventCallback.cpp \
../API/src/ForwardingControl.cpp \
../API/src/LocalForwardingEvents.cpp \
../API/src/LocalSubscriptionEvents.cpp \
../API/src/MCPConfig.cpp \
../API/src/MCPRouting.cpp \
../API/src/RemoteTopicTreeSubscriptionEvents.cpp \
../API/src/RouteLookup.cpp \
../API/src/ServerRegistration.cpp \
../API/src/StoreNodeData.cpp \
../API/src/cluster.cpp 

OBJS += \
./API/src/EngineEventCallback.o \
./API/src/ForwardingControl.o \
./API/src/LocalForwardingEvents.o \
./API/src/LocalSubscriptionEvents.o \
./API/src/MCPConfig.o \
./API/src/MCPRouting.o \
./API/src/RemoteTopicTreeSubscriptionEvents.o \
./API/src/RouteLookup.o \
./API/src/ServerRegistration.o \
./API/src/StoreNodeData.o \
./API/src/cluster.o 

CPP_DEPS += \
./API/src/EngineEventCallback.d \
./API/src/ForwardingControl.d \
./API/src/LocalForwardingEvents.d \
./API/src/LocalSubscriptionEvents.d \
./API/src/MCPConfig.d \
./API/src/MCPRouting.d \
./API/src/RemoteTopicTreeSubscriptionEvents.d \
./API/src/RouteLookup.d \
./API/src/ServerRegistration.d \
./API/src/StoreNodeData.d \
./API/src/cluster.d 


# Each subdirectory must supply rules for building sources it contributes
API/src/%.o: ../API/src/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -DSPDR_LINUX -DRUM_UNIX -DBOOST_THREAD_DYN_LINK -DBOOST_DATE_TIME_DYN_LINK -I"${ISM_UTILS_INCLUDE_DIR}" -I"${BOOST_INC_DIR}" -I"${MCC_WORKSPACE_DIR}/MCP_Routing/API/include" -I"${MCC_WORKSPACE_DIR}/MCP_Routing/Util/include" -I"${MCC_WORKSPACE_DIR}/MCP_Routing/Control/include" -I"${MCC_WORKSPACE_DIR}/MCP_Routing/Routing/include" -I"${SPDR_WORKSPACE_DIR}/SpiderCastCpp/API/include" -I"${SPDR_WORKSPACE_DIR}/SpiderCastCpp/Trace/include" -O3 -Wall -c -fmessage-length=0 -fPIC -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


