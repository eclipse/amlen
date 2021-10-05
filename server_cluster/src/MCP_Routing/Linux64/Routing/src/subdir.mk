################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../Routing/src/ASMFilter.cpp \
../Routing/src/AugmentedBloomFilter.cpp \
../Routing/src/BloomFilter.cpp \
../Routing/src/CountingBloomFilter.cpp \
../Routing/src/EngineEventCallbackCAdapter.cpp \
../Routing/src/FatalErrorHandler.cpp \
../Routing/src/ForwardingControlCAdapter.cpp \
../Routing/src/GlobalSubManager.cpp \
../Routing/src/GlobalSubManagerImpl.cpp \
../Routing/src/LocalExactSubManager.cpp \
../Routing/src/LocalSubManager.cpp \
../Routing/src/LocalSubManagerImpl.cpp \
../Routing/src/LocalWildcardSubManager.cpp \
../Routing/src/LookupBloomFilterArray.cpp \
../Routing/src/MCPRoutingImpl.cpp \
../Routing/src/PublishLocalBFTask.cpp \
../Routing/src/RemoteSubscriptionStats.cpp \
../Routing/src/RemoteSubscriptionStatsListener.cpp 

C_SRCS += \
../Routing/src/mccBFSet.c \
../Routing/src/mccLookupSet.c \
../Routing/src/mccWildcardBFSet.c 

OBJS += \
./Routing/src/ASMFilter.o \
./Routing/src/AugmentedBloomFilter.o \
./Routing/src/BloomFilter.o \
./Routing/src/CountingBloomFilter.o \
./Routing/src/EngineEventCallbackCAdapter.o \
./Routing/src/FatalErrorHandler.o \
./Routing/src/ForwardingControlCAdapter.o \
./Routing/src/GlobalSubManager.o \
./Routing/src/GlobalSubManagerImpl.o \
./Routing/src/LocalExactSubManager.o \
./Routing/src/LocalSubManager.o \
./Routing/src/LocalSubManagerImpl.o \
./Routing/src/LocalWildcardSubManager.o \
./Routing/src/LookupBloomFilterArray.o \
./Routing/src/MCPRoutingImpl.o \
./Routing/src/PublishLocalBFTask.o \
./Routing/src/RemoteSubscriptionStats.o \
./Routing/src/RemoteSubscriptionStatsListener.o \
./Routing/src/mccBFSet.o \
./Routing/src/mccLookupSet.o \
./Routing/src/mccWildcardBFSet.o 

C_DEPS += \
./Routing/src/mccBFSet.d \
./Routing/src/mccLookupSet.d \
./Routing/src/mccWildcardBFSet.d 

CPP_DEPS += \
./Routing/src/ASMFilter.d \
./Routing/src/AugmentedBloomFilter.d \
./Routing/src/BloomFilter.d \
./Routing/src/CountingBloomFilter.d \
./Routing/src/EngineEventCallbackCAdapter.d \
./Routing/src/FatalErrorHandler.d \
./Routing/src/ForwardingControlCAdapter.d \
./Routing/src/GlobalSubManager.d \
./Routing/src/GlobalSubManagerImpl.d \
./Routing/src/LocalExactSubManager.d \
./Routing/src/LocalSubManager.d \
./Routing/src/LocalSubManagerImpl.d \
./Routing/src/LocalWildcardSubManager.d \
./Routing/src/LookupBloomFilterArray.d \
./Routing/src/MCPRoutingImpl.d \
./Routing/src/PublishLocalBFTask.d \
./Routing/src/RemoteSubscriptionStats.d \
./Routing/src/RemoteSubscriptionStatsListener.d 


# Each subdirectory must supply rules for building sources it contributes
Routing/src/%.o: ../Routing/src/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -DSPDR_LINUX -DRUM_UNIX -DBOOST_THREAD_DYN_LINK -DBOOST_DATE_TIME_DYN_LINK -I"${ISM_UTILS_INCLUDE_DIR}" -I"${BOOST_INC_DIR}" -I"${MCC_WORKSPACE_DIR}/MCP_Routing/API/include" -I"${MCC_WORKSPACE_DIR}/MCP_Routing/Util/include" -I"${MCC_WORKSPACE_DIR}/MCP_Routing/Control/include" -I"${MCC_WORKSPACE_DIR}/MCP_Routing/Routing/include" -I"${SPDR_WORKSPACE_DIR}/SpiderCastCpp/API/include" -I"${SPDR_WORKSPACE_DIR}/SpiderCastCpp/Trace/include" -O3 -Wall -c -fmessage-length=0 -fPIC -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Routing/src/%.o: ../Routing/src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -std=gnu99 -I"${MCC_WORKSPACE_DIR}/MCP_Routing/Util/include" -I"${ISM_UTILS_INCLUDE_DIR}" -I"${MCC_WORKSPACE_DIR}/MCP_Routing/Control/include" -I"${MCC_WORKSPACE_DIR}/MCP_Routing/API/include" -I"${MCC_WORKSPACE_DIR}/MCP_Routing/Include/MCC" -I"${MCC_WORKSPACE_DIR}/MCP_Routing/Routing/include" -O3 -Wall -c -fmessage-length=0 -fPIC -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


