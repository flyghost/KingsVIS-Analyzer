#ifndef SDIO_SIMULATION_DATA_GENERATOR
#define SDIO_SIMULATION_DATA_GENERATOR

#include <SimulationChannelDescriptor.h>
#include <AnalyzerHelpers.h>
#include <string>
class SDIOAnalyzerSettings;

#define NUM_DATA_SAMPLES 598
class SDIOSimulationDataGenerator
{
public:
    SDIOSimulationDataGenerator();
    ~SDIOSimulationDataGenerator();

    void Initialize(U32 simulation_sample_rate, SDIOAnalyzerSettings *settings);
    U32 GenerateSimulationData(U64 newest_sample_requested, U32 sample_rate, SimulationChannelDescriptor **simulation_channel);
    void setIdle(void);

protected:
    SDIOAnalyzerSettings *mSettings;
    U32 mSimulationSampleRateHz;

protected:
    void CreateSerialByte();
    ClockGenerator generator;

    SimulationChannelDescriptorGroup mSdioSimulationChannels;
    SimulationChannelDescriptor *mSimClk;
    SimulationChannelDescriptor *mSimCmd;
    SimulationChannelDescriptor *mSimData0;
    SimulationChannelDescriptor *mSimData1;
    SimulationChannelDescriptor *mSimData2;
    SimulationChannelDescriptor *mSimData3;

    bool isDataLine;

private:
    void advanceAllLines(U32 numSamples);

public:
    class DataRepresentation
    {
    private:
        S32 currentDataIndex;
        //bool isDataLine;
        bool lastXferWasData;
        void setCmdBits(U64 data);
        void setDataBits(U64 data);
        SDIOSimulationDataGenerator *sim;
    public:
        void setBits();
        DataRepresentation(SDIOSimulationDataGenerator *theSim);
    };
private:
    DataRepresentation dataRep;

    static const U64 sampleData[NUM_DATA_SAMPLES];

};
#endif //SDIO_SIMULATION_DATA_GENERATOR
