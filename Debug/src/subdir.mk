################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/ApplySequence.cpp \
../src/ApplySequenceGui.cpp \
../src/Connect.cpp \
../src/Creator.cpp \
../src/CreatorGui.cpp \
../src/DNA.cpp \
../src/Disconnect.cpp \
../src/Duplicate.cpp \
../src/ExportStrands.cpp \
../src/ExtendGui.cpp \
../src/ExtendHelix.cpp \
../src/FindFivePrimeEnds.cpp \
../src/Helix.cpp \
../src/HelixBase.cpp \
../src/JSONTranslator.cpp \
../src/Locator.cpp \
../src/PaintStrand.cpp \
../src/RetargetBase.cpp \
../src/ToggleCylinderBaseView.cpp \
../src/ToggleLocatorRender.cpp \
../src/Tracker.cpp \
../src/Utility.cpp \
../src/jsoncpp.cpp \
../src/main.cpp 

OBJS += \
./src/ApplySequence.o \
./src/ApplySequenceGui.o \
./src/Connect.o \
./src/Creator.o \
./src/CreatorGui.o \
./src/DNA.o \
./src/Disconnect.o \
./src/Duplicate.o \
./src/ExportStrands.o \
./src/ExtendGui.o \
./src/ExtendHelix.o \
./src/FindFivePrimeEnds.o \
./src/Helix.o \
./src/HelixBase.o \
./src/JSONTranslator.o \
./src/Locator.o \
./src/PaintStrand.o \
./src/RetargetBase.o \
./src/ToggleCylinderBaseView.o \
./src/ToggleLocatorRender.o \
./src/Tracker.o \
./src/Utility.o \
./src/jsoncpp.o \
./src/main.o 

CPP_DEPS += \
./src/ApplySequence.d \
./src/ApplySequenceGui.d \
./src/Connect.d \
./src/Creator.d \
./src/CreatorGui.d \
./src/DNA.d \
./src/Disconnect.d \
./src/Duplicate.d \
./src/ExportStrands.d \
./src/ExtendGui.d \
./src/ExtendHelix.d \
./src/FindFivePrimeEnds.d \
./src/Helix.d \
./src/HelixBase.d \
./src/JSONTranslator.d \
./src/Locator.d \
./src/PaintStrand.d \
./src/RetargetBase.d \
./src/ToggleCylinderBaseView.d \
./src/ToggleLocatorRender.d \
./src/Tracker.d \
./src/Utility.d \
./src/jsoncpp.d \
./src/main.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -I/usr/autodesk/maya/include -I"/home/johan/workspace/vHelix/include" -O0 -g3 -Wall -c -fmessage-length=0 -fPIC -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


