#!/bin/bash
# Host-side logic tests (no hardware / PlatformIO needed). Requires g++.
# usage: ./run.sh            -> runs both suites against the project root (../..)
cd "$(dirname "$0")" || exit 1
ROOT="$(cd ../.. && pwd)"
INC="-Istubs -I$ROOT/include/Common -I$ROOT/include/Services/ComManager -I$ROOT/include/Services/HMIService -I$ROOT/include/Services/IOPinManager -I$ROOT/include/Services/RobotSystemBuilder -I$ROOT/include/Services/CapabilityRegistry -I$ROOT/include/Services/ConfigurationManager -I$ROOT/include/EcuAbstraction/Motor_TA6586 -I$ROOT/include/EcuAbstraction/MotorManager -I$ROOT/lib/GenericRobotController -I$ROOT/lib/GenericLogger -I."
LIB="$ROOT/lib/GenericRobotController/*.cpp $ROOT/lib/GenericLogger/GenericLogger.cpp stubs/Arduino.cpp"
ROBOT="$ROOT/src/Robot_Mega2560/{HMIService,ConfigurationManager,IOPinManager,Motor_TA6586,RobotSystemBuilder,CapabilityRegistry,ComManager}.cpp"
rc=0
g++ -std=c++17 -w -o /tmp/ctrl_tests $INC controller_tests.cpp $LIB && /tmp/ctrl_tests || rc=1
g++ -std=c++17 -w -o /tmp/builder_tests $INC builder_tests.cpp $LIB $(eval echo $ROBOT) && /tmp/builder_tests || rc=1
exit $rc
