if [ -z ${MARTe2_DIR+x} ]; then echo "Please set the MARTe2_DIR environment variable"; exit; fi
if [ -z ${MARTe2_Components_DIR+x} ]; then echo "Please set the MARTe2_Components_DIR environment variable"; exit; fi

if [ -z ${MARTE_DIR+x} ]; then export MARTE_DIR=.; fi;


LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$MARTE_DIR/.
LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$MARTE_DIR/../GAMs/SimulinkWrapperGAM/
LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$MARTE_DIR/../GAMs/MathExpressionGAM/
LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$MARTE_DIR/../Build/x86-linux/Components/GAMs/DutyCycleGAM/
LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$MARTE_DIR/../Build/x86-linux/Components/GAMs/RTSMGAM/
LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$MARTE_DIR/../Build/x86-linux/Components/GAMs/FunctionGeneratorGAM/
LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$MARTE_DIR/../Build/x86-linux/Components/GAMs/TestGAM/
LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$MARTE_DIR/../Build/x86-linux/Components/GAMs/Raptor/
LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$MARTE_DIR/../Build/x86-linux/Components/GAMs/equinoxGAM/
LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$MARTE_DIR/../Build/x86-linux/Components/GAMs/FFTGAM/
LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$MARTE_DIR/../Build/x86-linux/Components/GAMs/MathExpressionGAM
LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$MARTE_DIR/../Build/x86-linux/Components/GAMs/SpiderCalGAM
LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$MARTE_DIR/../Build/x86-linux/Components/GAMs/PyGAM
LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$MARTE_DIR/../Build/x86-linux/Components/GAMs/ResamplerGAM
LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$MARTE_DIR/../Build/x86-linux/Components/GAMs/PickSampleGAM
LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$MARTE_DIR/../Build/x86-linux/Components/GAMs/DelayGAM
LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/opt/anaconda3/lib/
LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$MARTE_DIR/../Build/x86-linux/Components/DataSources/DTACQAI
LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$MARTE_DIR/../Build/x86-linux/Components/DataSources/DTACQAO
LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$MARTE_DIR/../Build/x86-linux/Components/DataSources/WRTDTimer
LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$MARTE_DIR/../Build/x86-linux/Components/DataSources/SWTrig
LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$MARTE_DIR/../Build/x86-linux/Components/DataSources/mcc118
LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$MARTE_DIR/../Build/x86-linux/Components/DataSources/NI6259DIO_M
LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$MARTE_DIR/../Build/x86-linux/Components/DataSources/NI6368DIO_M
LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$MARTE_DIR/../Build/x86-linux/Components/DataSources/StreamOut
LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$MARTE_DIR/../Build/x86-linux/Components/DataSources/StreamIn
LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$MARTE_DIR/../Build/x86-linux/Components/DataSources/ConsoleOut
LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$MARTE_DIR/../Build/x86-linux/Components/DataSources/MDSReaderNS
LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$MARTE_DIR/../Build/x86-linux/Components/DataSources/TcpIn
LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$MARTE_DIR/../Build/x86-linux/Components/DataSources/TcpOut
LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$MARTE_DIR/../Build/x86-linux/Components/GAMs/MDSReaderGAM
LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$MARTE_DIR/../Build/x86-linux/Components/Interfaces/MDSEventManager
LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$MARTE_DIR/../Build/x86-linux/Components/GAMs/DelayedSimulinkWrapperGAM/
LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$MARTE_DIR/../Build/x86-linux/Components/DataSources/RTNIn
LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$MARTE_DIR/../Build/x86-linux/Components/DataSources/RTNOut
LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$MARTe2_DIR/Build/x86-linux/Core/
LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$MARTe2_Components_DIR/Build/x86-linux/Components/DataSources/EpicsDataSource/
LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$MARTe2_Components_DIR/Build/x86-linux/Components/DataSources/LinuxTimer/
LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$MARTe2_Components_DIR/Build/x86-linux/Components/DataSources/LoggerDataSource/
LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$MARTe2_Components_DIR/Build/x86-linux/Components/DataSources/NI6259/
LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$MARTe2_Components_DIR/Build/x86-linux/Components/DataSources/NI6368/
LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$MARTe2_Components_DIR/Build/x86-linux/Components/DataSources/SDN/
LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$MARTe2_Components_DIR/Build/x86-linux/Components/DataSources/FileDataSource/
LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$MARTe2_Components_DIR/Build/x86-linux/Components/DataSources/RealTimeThreadSynchronisation/
LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$MARTe2_Components_DIR/Build/x86-linux/Components/DataSources/RealTimeThreadAsyncBridge/
LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$MARTe2_Components_DIR/Build/x86-linux/Components/DataSources/UDP/
LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$MARTe2_Components_DIR/Build/x86-linux/Components/GAMs/IOGAM/
LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$MARTe2_Components_DIR/Build/x86-linux/Components/GAMs/ConversionGAM/
LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$MARTe2_Components_DIR/Build/x86-linux/Components/GAMs/SSMGAM/
LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$MARTe2_Components_DIR/Build/x86-linux/Components/GAMs/SimulinkWrapperGAM/
LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$MARTe2_Components_DIR/Build/x86-linux/Components/GAMs/WaveformGAM/
LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$MARTe2_Components_DIR/Build/x86-linux/Components/GAMs/BaseLib2GAM/
LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$MARTe2_Components_DIR/Build/x86-linux/Components/GAMs/ConversionGAM/
LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$MARTe2_Components_DIR/Build/x86-linux/Components/Interfaces/BaseLib2Wrapper/
LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$MARTe2_Components_DIR/Build/x86-linux/Components/Interfaces/EPICSInterface/
LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$MARTe2_Components_DIR/Build/x86-linux/Components/DataSources/MDSWriter
LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/opt/MARTe2/MARTe2-rfx-components/Build/x86-linux/Components/GAMs/MegavoltGAM


LD_LIBRARY_PATH=$MARTe2_Components_DIR/Build/x86-linux/Components/GAMs/SimulinkWrapperGAM:$LD_LIBRARY_PATH

#Disable CPU speed changing
#cpupower frequency-set --governor performance
#service cpuspeed stop

echo $LD_LIBRARY_PATH
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH
#cgdb --args ../Build/linux/Startup/Playground.ex $1 $2 $3 $4
#strace -o/tmp/strace.err ../Build/linux/Startup/Playground.ex $1 $2  $3 $4
$MARTe2_DIR/Docs/User/source/_static/examples/MARTeApp.sh -l RealTimeLoader $1 $2  $3 $4
#valgrind $MARTe2_DIR/Docs/User/source/_static/examples/MARTeApp.sh -l RealTimeLoader $1 $2  $3 $4


#MARTE_DIR/../Build/x86-linux/Startup/Playground.ex $1 $2 $3 $4 
#valgrind $MARTE_DIR/../Build/x86-linux/Startup/Playground.ex $1 $2 $3 $4 
