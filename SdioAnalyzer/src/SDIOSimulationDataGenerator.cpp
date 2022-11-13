#include "SDIOSimulationDataGenerator.h"
#include "SDIOAnalyzerSettings.h"

#include <AnalyzerHelpers.h>

SDIOSimulationDataGenerator::SDIOSimulationDataGenerator()
    : generator(),
      mSdioSimulationChannels(),
      mSimClk(0),
      mSimCmd(0),
      mSimData0(0),
      mSimData1(0),
      mSimData2(0),
      mSimData3(0),
      dataRep(this)
{

}

SDIOSimulationDataGenerator::~SDIOSimulationDataGenerator()
{
}

void SDIOSimulationDataGenerator::Initialize(U32 simulation_sample_rate, SDIOAnalyzerSettings *settings)
{
    mSimulationSampleRateHz = simulation_sample_rate;
    mSettings = settings;

    mSimClk = mSdioSimulationChannels.Add(mSettings->mClockChannel, simulation_sample_rate, BIT_HIGH);
    mSimCmd = mSdioSimulationChannels.Add(mSettings->mCmdChannel, simulation_sample_rate, BIT_HIGH);

    if (settings->mDAT0Channel != UNDEFINED_CHANNEL) {
        mSimData0 = mSdioSimulationChannels.Add(mSettings->mDAT0Channel, simulation_sample_rate, BIT_HIGH);
    } else {
        mSimData0 = NULL;
    }

    if (settings->mDAT1Channel != UNDEFINED_CHANNEL) {
        mSimData1 = mSdioSimulationChannels.Add(mSettings->mDAT1Channel, simulation_sample_rate, BIT_HIGH);
    } else {
        mSimData1 = NULL;
    }

    if (settings->mDAT2Channel != UNDEFINED_CHANNEL) {
        mSimData2 = mSdioSimulationChannels.Add(mSettings->mDAT2Channel, simulation_sample_rate, BIT_HIGH);
    } else {
        mSimData2 = NULL;
    }

    if (settings->mDAT3Channel != UNDEFINED_CHANNEL) {
        mSimData3 = mSdioSimulationChannels.Add(mSettings->mDAT3Channel, simulation_sample_rate, BIT_HIGH);
    } else {
        mSimData3 = NULL;
    }

    // setup clock generator
    double freq = simulation_sample_rate;
    //freq /= 2.0;
    freq /= 20.0;
    generator.Init(freq, simulation_sample_rate);
}

U32 SDIOSimulationDataGenerator::GenerateSimulationData(U64 largest_sample_requested, U32 sample_rate, SimulationChannelDescriptor **simulation_channel)
{
    U64 adjusted_largest_sample_requested = AnalyzerHelpers::AdjustSimulationTargetSample(largest_sample_requested, sample_rate, mSimulationSampleRateHz);
    // std::cout << "SDIOSimulationDataGenerator::GenerateSimulationData................................" << std::endl;
    // std::cout << "current sample: " << mSimClk->GetCurrentSampleNumber() << ", largest_sample_requested: "<< largest_sample_requested <<  std::endl;

    // start off with idle bits for first 10,000 samples
    if (mSimClk->GetCurrentSampleNumber() < 10000 && mSimClk->GetCurrentSampleNumber() < adjusted_largest_sample_requested) {
        while (mSimClk->GetCurrentSampleNumber() < 10000) {
            setIdle();
        }
    }

    while ((mSimClk->GetCurrentSampleNumber() + 4) < adjusted_largest_sample_requested) {
        CreateSerialByte();
    }

    *simulation_channel = mSdioSimulationChannels.GetArray();
    return mSdioSimulationChannels.GetCount();
}

void SDIOSimulationDataGenerator::CreateSerialByte()
{
    dataRep.setBits();
}

void SDIOSimulationDataGenerator::advanceAllLines(U32 numSamples)
{
    mSimClk->Advance(numSamples);
    mSimCmd->Advance(numSamples);

    if (NULL != mSimData0) {
        mSimData0->Advance(numSamples);
    }

    if (NULL != mSimData1) {
        mSimData1->Advance(numSamples);
    }

    if (NULL != mSimData2) {
        mSimData2->Advance(numSamples);
    }

    if (NULL != mSimData3) {
        mSimData3->Advance(numSamples);
    }
}

void SDIOSimulationDataGenerator::setIdle(void)
{
    U32 numSamples = 0;
    numSamples = generator.AdvanceByHalfPeriod();
    mSimClk->Transition();
    advanceAllLines(numSamples);

    numSamples = generator.AdvanceByHalfPeriod();
    mSimClk->Transition();
    advanceAllLines(numSamples);
}

SDIOSimulationDataGenerator::DataRepresentation::DataRepresentation(SDIOSimulationDataGenerator *theSim)
    : currentDataIndex(-1),
      lastXferWasData(false),
      sim(theSim)
{
}

void SDIOSimulationDataGenerator::DataRepresentation::setBits()
{
    currentDataIndex++;
    if (currentDataIndex < NUM_DATA_SAMPLES) {
        U64 smpDat = sampleData[currentDataIndex];
        if (smpDat > 0xff) {
            setCmdBits(smpDat);
            lastXferWasData = false;
        } else {
            setDataBits(smpDat);
            lastXferWasData = true;
        }
    } else { // we're done with data, just advance clock
        sim->setIdle();
    }
}

void SDIOSimulationDataGenerator::DataRepresentation::setCmdBits(U64 data)
{
    BitExtractor bitExtractor(data, AnalyzerEnums::MsbFirst, 48);

    // iterate through the BitExtractor and set data
    int i, j;
    U32 numSamples = 0;
    numSamples = sim->generator.AdvanceByHalfPeriod();
    sim->mSimClk->Transition();
    sim->advanceAllLines(numSamples);

    // put some idle before this word
    for (i = 0; i < 8; i++) {
        numSamples = sim->generator.AdvanceByHalfPeriod();
        sim->mSimClk->Transition();
        sim->advanceAllLines(numSamples);
    }

    if (sim->mSettings->mSampleRead == RISING_EDGE) {
        if (sim->mSimClk->GetCurrentBitState() == BIT_LOW) {
            numSamples = sim->generator.AdvanceByHalfPeriod();
            sim->mSimClk->Transition();
            sim->advanceAllLines(numSamples);
        }
    } else {
        if (sim->mSimClk->GetCurrentBitState() == BIT_HIGH) {
            numSamples = sim->generator.AdvanceByHalfPeriod();
            sim->mSimClk->Transition();
            sim->advanceAllLines(numSamples);
        }
    }


    BitState bit_state;
    for (i = 0; i < 48; i++) {
        // set the cmd line
        bit_state = bitExtractor.GetNextBit();
        sim->mSimCmd->TransitionIfNeeded(bit_state);

        // walk clock and data lines
        for (j = 0; j < 2; j++) {
            numSamples = sim->generator.AdvanceByHalfPeriod();
            sim->mSimClk->Transition();
            sim->advanceAllLines(numSamples);
        }
    }
    // put some idle after this word
    for (i = 0; i < 4; i++) {
        numSamples = sim->generator.AdvanceByHalfPeriod();
        sim->mSimClk->Transition();
        sim->advanceAllLines(numSamples);
    }
}

void SDIOSimulationDataGenerator::DataRepresentation::setDataBits(U64 data)
{
    BitExtractor bitExtractor(data, AnalyzerEnums::MsbFirst, 8);
    // iterate through the BitExtractor and set data
    int i, j;
    U32 numSamples = 0;
    if (lastXferWasData == false) {
        numSamples = sim->generator.AdvanceByHalfPeriod();
        sim->mSimClk->Transition();
        sim->advanceAllLines(numSamples);

        // we want to put data out on high clock, it will be sampled on rising edge
        if (sim->mSimClk->GetCurrentBitState() == BIT_LOW) {
            numSamples = sim->generator.AdvanceByHalfPeriod();
            sim->mSimClk->Transition();
            sim->advanceAllLines(numSamples);
        }
        // put start bit, which will be 1 cycle
        // set the cmd line

        if (NULL != sim->mSimData3) {
            sim->mSimData3->TransitionIfNeeded(BIT_LOW);
        }
        if (NULL != sim->mSimData2) {
            sim->mSimData2->TransitionIfNeeded(BIT_LOW);
        }
        if (NULL != sim->mSimData1) {
            sim->mSimData1->TransitionIfNeeded(BIT_LOW);
        }
        if (NULL != sim->mSimData0) {
            sim->mSimData0->TransitionIfNeeded(BIT_LOW);
        }
        for (i = 0; i < 2; i++) {
            numSamples = sim->generator.AdvanceByHalfPeriod();
            sim->mSimClk->Transition();
            sim->advanceAllLines(numSamples);
        }
    }

    BitState bit_state;
    i = 0;
    while (i < 8) {
        // set the cmd line
        bit_state = bitExtractor.GetNextBit();
        if (NULL != sim->mSimData3) {
            i++;
            sim->mSimData3->TransitionIfNeeded(bit_state);
        }
        if (NULL != sim->mSimData2) {
            i++;
            sim->mSimData2->TransitionIfNeeded(bit_state);
        }
        if (NULL != sim->mSimData1) {
            i++;
            sim->mSimData1->TransitionIfNeeded(bit_state);
        }
        if (NULL != sim->mSimData0) {
            i++;
            sim->mSimData0->TransitionIfNeeded(bit_state);
        }

        // walk clock and data lines
        for (j = 0; j < 2; j++) {
            numSamples = sim->generator.AdvanceByHalfPeriod();
            sim->mSimClk->Transition();
            sim->advanceAllLines(numSamples);
        }
    }
    // numSamples = sim->generator.AdvanceByHalfPeriod();
    // sim->mSimClk->Transition();
    // sim->advanceAllLines(numSamples);
}

// sample data for generator
const U64 SDIOSimulationDataGenerator::sampleData[] = {
    0x7400000C0039, 0x7480000C089F, 0x400000000095, 0x48000001AA87, 0x45000000005B, 0x3F10FF8000FF, 0x450030000087, 0x3F10FF8000FF,
    0x450030000087, 0x3F10FF8000FF, 0x450030000087, 0x3F10FF8000FF, 0x450030000087, 0x3F90FF8000FF, 0x430000000021, 0x0300010000EB,
    0x4700010000DD, 0x0700001E00A1, 0x7400000000D1, 0x340000103245, 0x7400001000A3, 0x34000010DFA1, 0x74000024006D, 0x340000100125,
    0x740000260041, 0x340000100037, 0x74000012008F, 0x340000100037, 0x7400001400FB, 0x340000101005, 0x7400001600D7, 0x340000100037,
    0x7400200000B7, 0x340000102053, 0x74002002009B, 0x34000010047F, 0x7400200400EF, 0x340000107A1D, 0x7400200600C3, 0x340000100037,
    0x740020080007, 0x3400001043C9, 0x7400200A002B, 0x3400001041ED, 0x7400200C005F, 0x340000102141, 0x7400200E0073, 0x340000100213,
    0x7400201000C5, 0x340000100CEF, 0x7400201200E9, 0x340000100037, 0x74002014009D, 0x340000102277, 0x7400201600B1, 0x34000010047F,
    0x740020180075, 0x340000100037, 0x7400201A0059, 0x340000100037, 0x7400201C002D, 0x3400001008A7, 0x7400201E0001, 0x340000103173,
    0x740020200053, 0x34000010FFC5, 0x7400000E0015, 0x3400001040FF, 0x7480000E42CF, 0x3400001042DB, 0x74000200006D, 0x340000100749,
    0x740002120033, 0x340000100037, 0x740002140047, 0x340000102053, 0x74000216006B, 0x340000100037, 0x74004000001D, 0x340000102141,
    0x740040020031, 0x340000100213, 0x740040040045, 0x340000100CEF, 0x740040060069, 0x340000100037, 0x7400400800AD, 0x340000102277,
    0x7400400A0081, 0x340000102AE7, 0x7400400C00F5, 0x340000100125, 0x7400400E00D9, 0x340000100125, 0x74004010006F, 0x340000100037,
    0x740040120043, 0x340000100037, 0x740040140037, 0x340000100037, 0x74004016001B, 0x340000100037, 0x7400401800DF, 0x340000100037,
    0x7400401A00F3, 0x340000100037, 0x7400401C0087, 0x340000100037, 0x7400401E00AB, 0x340000100037, 0x7400402000F9, 0x340000100037,
    0x7400402200D5, 0x340000100037, 0x7400402400A1, 0x340000100037, 0x74004026008D, 0x3400001008A7, 0x740040280049, 0x340000100037,
    0x7400402A0065, 0x3400001080B5, 0x7400402C0011, 0x340000101FEB, 0x7400402E003D, 0x340000100037, 0x74004030008B, 0x3400001050CD,
    0x7400403200A7, 0x3400001050CD, 0x7400403400D3, 0x3400001064D3, 0x7400403600FF, 0x340000100037, 0x74004038003B, 0x340000100037,
    0x7400403A0017, 0x340000100037, 0x7400403C0063, 0x340000100037, 0x7400403E004F, 0x340000100037, 0x7400404000C7, 0x340000100037,
    0x7400404200EB, 0x340000100037, 0x74004044009F, 0x3400001064D3, 0x7400404600B3, 0x340000100037, 0x740040480077, 0x3400001050CD,
    0x7400404A005B, 0x340000100037, 0x7400404C002F, 0x3400001064D3, 0x7400404E0003, 0x340000100037, 0x7400405000B5, 0x340000100037,
    0x740040520099, 0x340000100037, 0x7400405400ED, 0x340000100037, 0x7400405600C1, 0x340000100037, 0x740040580005, 0x340000100037,
    0x7400405A0029, 0x340000100037, 0x7400405C005D, 0x340000100037, 0x7400405E0071, 0x340000100037, 0x740040600023, 0x34000010FFC5,
    0x7480022000BF, 0x340000100037, 0x7480022202B7, 0x340000100213, 0x740000040089, 0x340000100037, 0x74800004029B, 0x340000100213,
    0x7400000600A5, 0x340000100213, 0x7480022000BF, 0x340000100037, 0x7480022202B7, 0x340000100213, 0x7594000010C1, 0x35000010005B,
    0x00, 0x00, 0x02, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0, 0xF0, 0xF0, 0xF0,
    0x22, 0x0E, 0x3E, 0x21, 0xC2, 0xC2, 0x11, 0x03, 0xFF, 0xFE, 0xEF, 0xEF, 0xEE, 0xEE, 0xEE, 0xEE,
    0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEF,
    0x741000080001, 0x340000100037, 0x741000080001, 0x340000100037, 0x741000080001, 0x340000100037, 0x340000100125, 0x741000380097,
    0x34000010680B, 0x7410003A00BB, 0x340000100037, 0x7410003C00CF, 0x340000100037, 0x7514000068F9, 0x35000010005B,
    0x00, 0x00, 0x03, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x59, 0x00, 0x0C, 0x00, 0x04, 0x13,
    0x03, 0x01, 0x52, 0x01, 0x04, 0x13, 0x03, 0x01, 0x52, 0x02, 0x04, 0x00, 0x00, 0x01, 0x52, 0x03,
    0x04, 0x00, 0x00, 0x04, 0x52, 0x04, 0x01, 0x00, 0x05, 0x01, 0x50, 0x06, 0x01, 0x00, 0x07, 0x01,
    0x00, 0x08, 0x01, 0x00, 0x09, 0x0B, 0x53, 0x65, 0x72, 0x69, 0x61, 0x6C, 0x4E, 0x75, 0x6D, 0x31,
    0x00, 0x0A, 0x10, 0x52, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x0B, 0x10, 0x43, 0x4C, 0x4E, 0x83, 0x76, 0x41, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0x6C, 0x08, 0xDC, 0x85, 0x67, 0x3A, 0x0C, 0x3E,
    0x7594000010C1, 0x35000010005B,
    0x00, 0x00, 0x02, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0, 0xF0, 0xF0, 0xF0,
    0x22, 0x0D, 0x0D, 0x22, 0xF2, 0xF2, 0x22, 0x00, 0xFF, 0xFE, 0xEF, 0xEF, 0xEE, 0xEE, 0xEE, 0xEE,
    0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE,
    0xEE, 0xEE, 0xEE, 0xEF,
    0x741000080001, 0x340000100125, 0x741000380097, 0x34000010680B, 0x7410003A00BB, 0x340000100037, 0x7410003C00CF, 0x340000100037,
    0x7514000068F9, 0x35000010005B,
    0x00, 0x00, 0x03, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x59, 0x00, 0x0C, 0x00, 0x04, 0x13,
    0x03, 0x01, 0x52, 0x01, 0x04, 0x13, 0x03, 0x01, 0x52, 0x02, 0x04, 0x00, 0x00, 0x01, 0x52, 0x03,
    0x04, 0x00, 0x00, 0x04, 0x52, 0x04, 0x01, 0x00, 0x05, 0x01, 0x50, 0x06, 0x01, 0x00, 0x07, 0x01,
    0x00, 0x08, 0x01, 0x00, 0x09, 0x0B, 0x53, 0x65, 0x72, 0x69, 0x61, 0x6C, 0x4E, 0x75, 0x6D, 0x31,
    0x00, 0x0A, 0x10, 0x52, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x0B, 0x10, 0x43, 0x4C, 0x4E, 0x83, 0x76, 0x41, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0x6F, 0x38, 0xEF, 0xB6, 0x67, 0x39, 0x0C, 0x3E,
    0x7594000010C1, 0x35000010005B,
    0x00, 0x00, 0x02, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0, 0xF0, 0xF0, 0xF0,
    0x22, 0x0C, 0x1C, 0x23, 0xE2, 0xE2, 0x33, 0x01, 0xFF, 0xFE, 0xEF, 0xEF, 0xEE, 0xEE, 0xEE, 0xEE,
    0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE,
    0xEE, 0xEE, 0xEE, 0xEF,
    0x741000080001, 0x340000100125, 0x741000380097, 0x34000010680B, 0x7410003A00BB, 0x340000100037
};
