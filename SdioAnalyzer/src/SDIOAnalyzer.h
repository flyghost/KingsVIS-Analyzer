#ifndef SDIO_ANALYZER_H
#define SDIO_ANALYZER_H

#include <Analyzer.h>
#include "SDIOAnalyzerResults.h"
#include "SDIOSimulationDataGenerator.h"

#define Min_Cmd_Start_Width 2

class SDIOAnalyzerSettings;
class ANALYZER_EXPORT SDIOAnalyzer : public Analyzer
{
public:
    
    SDIOAnalyzer();         // SDIO构造函数
    virtual ~SDIOAnalyzer();// SDIO析构函数
    virtual void SetupResults();
    virtual void WorkerThread();

    virtual U32 GenerateSimulationData(U64 newest_sample_requested, U32 sample_rate, SimulationChannelDescriptor **simulation_channels);
    virtual U32 GetMinimumSampleRateHz();

    virtual const char *GetAnalyzerName() const;
    virtual bool NeedsRerun();

    enum frameTypes {FRAME_START, FRAME_DIR, FRAME_CMD, FRAME_ACMD, FRAME_RESP, FRAME_ARG, FRAME_LONG_ARG, FRAME_CRC, FRAME_STOP, DATA};
    bool isACMD(U32 cmd1, U64 cmd2);

protected: //vars
    std::auto_ptr< SDIOAnalyzerSettings > mSettings;
    std::auto_ptr< SDIOAnalyzerResults > mResults;

    AnalyzerChannelData *mClock;
    AnalyzerChannelData *mCmd;
    AnalyzerChannelData *mDAT0;
    AnalyzerChannelData *mDAT1;
    AnalyzerChannelData *mDAT2;
    AnalyzerChannelData *mDAT3;

    SDIOSimulationDataGenerator mSimulationDataGenerator;
    bool mSimulationInitilized;

private:
    enum analyzerStep {FIND_CMD_START, FIND_DATA_START, READ_DATA, ANLY_CMD};
    U32 anlyStep;

    bool FrameStateMachine();
    enum frameStates {START_BIT, TRANSMISSION_BIT, COMMAND, ARGUMENT, CRC7, STOP};
    U32 frameState;

    bool getDataLinesStartBit();
    bool readDataLines();
    U64 clkCurrentSmpNum;
    U64 dataStartSmpNum;
    U64 cmdStartSmpNum;
    U64 cmdStartBitWidth;
    U16 dataNum;

    BitState isCmd; //1:CMD, 0:Response
    //bool isCmdApp;

};

extern "C" ANALYZER_EXPORT const char *__cdecl GetAnalyzerName();
extern "C" ANALYZER_EXPORT Analyzer *__cdecl CreateAnalyzer();
extern "C" ANALYZER_EXPORT void __cdecl DestroyAnalyzer(Analyzer *analyzer);

#endif //SDIO_ANALYZER_H
