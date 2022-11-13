#include "SDIOAnalyzer.h"
#include "SDIOAnalyzerSettings.h"
#include <AnalyzerChannelData.h>

SDIOAnalyzer::SDIOAnalyzer()
    : Analyzer(),
      mSettings(new SDIOAnalyzerSettings()),
      mSimulationInitilized(false)
{
    SetAnalyzerSettings(mSettings.get());
}

SDIOAnalyzer::~SDIOAnalyzer()
{
    KillThread();
}

void SDIOAnalyzer::SetupResults()
{
    mResults.reset(new SDIOAnalyzerResults(this, mSettings.get()));
    SetAnalyzerResults(mResults.get());
    mResults->AddChannelBubblesWillAppearOn(mSettings->mCmdChannel);
    if (!mSettings->mDoNotGenerateDatFrames) {
        mResults->AddChannelBubblesWillAppearOn(mSettings->mDAT0Channel);
    }
}

void SDIOAnalyzer::WorkerThread()
{
    mClock = GetAnalyzerChannelData(mSettings->mClockChannel);
    mCmd = GetAnalyzerChannelData(mSettings->mCmdChannel);
    mDAT0 = GetAnalyzerChannelData(mSettings->mDAT0Channel);
    mDAT1 = mSettings->mDAT1Channel == UNDEFINED_CHANNEL ? NULL : GetAnalyzerChannelData(mSettings->mDAT1Channel);
    mDAT2 = mSettings->mDAT2Channel == UNDEFINED_CHANNEL ? NULL : GetAnalyzerChannelData(mSettings->mDAT2Channel);
    mDAT3 = mSettings->mDAT3Channel == UNDEFINED_CHANNEL ? NULL : GetAnalyzerChannelData(mSettings->mDAT3Channel);

    anlyStep = FIND_CMD_START;
    frameState = START_BIT;
    clkCurrentSmpNum = 0; //当前CLK采样点
    dataNum = 0; //数据读取计数

    U64 smpNum = 0;
    bool existData = false;
    bool readData;
    for (; ;) {
        switch (anlyStep) {
        case FIND_CMD_START: { //查找CMD开始位
            //CMD上无跳变，总线上全为DATA数据
            if (!mCmd->DoMoreTransitionsExistInCurrentData()) {
                anlyStep = FIND_DATA_START;
                break;
            }
            //判断当前mCmd状态，为低则向前移动两次，为高向前移动一次，查找下降沿
            if (mCmd->GetBitState() == BIT_LOW) {
                mCmd->AdvanceToNextEdge();
            }
            mCmd->AdvanceToNextEdge();
            smpNum = mCmd->GetSampleNumber(); //下降沿位置
            cmdStartBitWidth = mCmd->GetSampleOfNextEdge() - smpNum; //负脉宽
            if (cmdStartBitWidth <= 1) {
                anlyStep = FIND_CMD_START;
                break;
            }
            anlyStep = FIND_DATA_START;
        }
        case FIND_DATA_START: { //查找DATA开始位
            cmdStartSmpNum = mCmd->GetSampleNumber(); //CMD_Start采样点
            existData = getDataLinesStartBit(); //判断是否存在DATA
            if (!existData) {
                mClock->AdvanceToAbsPosition(cmdStartSmpNum);
                anlyStep = ANLY_CMD;
                break;
            } else {
                anlyStep = READ_DATA;
            }
        }
		case READ_DATA: { //读取DATA
			dataNum = 0;
			readData = true;
			while ((readData) && (mDAT0->WouldAdvancingToAbsPositionCauseTransition(cmdStartSmpNum))) {
				readData = readDataLines();
			}
            if (!mSettings->mDoNotGenerateDatFrames) {
                if (dataNum > 1) {
                    mResults->CommitPacketAndStartNewPacket();
                }
            }
			mClock->AdvanceToAbsPosition(mCmd->GetSampleNumber());
			anlyStep = ANLY_CMD;
		}
        case ANLY_CMD://解析CMD指令
            if (FrameStateMachine()) {
                anlyStep = FIND_CMD_START;
            }
            break;
        default: break;
        }

        mResults->CommitResults();
        ReportProgress(mClock->GetSampleNumber());
    }
}

bool SDIOAnalyzer::FrameStateMachine()
{
    static U64 startOfNextFrame = 0;
    static U32 frameCounter = 0;
    static U8 respLength;
    static U8 respType;
    static U64 temp;
    static U64 temp2;
    static char lastHostCmd = -1;

    if (mSettings->mSampleRead == RISING_EDGE) {
        if (mClock->GetBitState() == BIT_HIGH) {
            mClock->AdvanceToNextEdge();
        }
    } else {
        if (mClock->GetBitState() == BIT_LOW) {
            mClock->AdvanceToNextEdge();
        }
    }
    mClock->AdvanceToNextEdge(); //周期跳变
    U64 smpMid = mClock->GetSampleNumber();

    mCmd->AdvanceToAbsPosition(smpMid);
    mDAT0->AdvanceToAbsPosition(smpMid);
    if (mDAT1) {
        mDAT1->AdvanceToAbsPosition(smpMid);
    }
    if (mDAT2) {
        mDAT2->AdvanceToAbsPosition(smpMid);
    }
    if (mDAT3) {
        mDAT3->AdvanceToAbsPosition(smpMid);
    }

    U64 smpEnd = mClock->GetSampleOfNextEdge(); //周期结束
    if (frameState == START_BIT) {
        if (mCmd->GetBitState() == BIT_LOW) {
            mResults->AddMarker(smpMid, AnalyzerResults::Start, mSettings->mCmdChannel);
            if (mSettings->mSampleRead == RISING_EDGE) {
                mResults->AddMarker(smpMid, AnalyzerResults::UpArrow, mSettings->mClockChannel);
            } else {
                mResults->AddMarker(smpMid, AnalyzerResults::DownArrow, mSettings->mClockChannel);
            }
            if (!mSettings->mDoNotGenerateStartFrames) {
                Frame frame;
                frame.mType = FRAME_START;
                frame.mFlags = 0;
                frame.mData1 = 0;
                frame.mStartingSampleInclusive = cmdStartSmpNum;
                frame.mEndingSampleInclusive = smpEnd - 1;
                mResults->AddFrame(frame);
            }
            startOfNextFrame = smpEnd;
            frameState = TRANSMISSION_BIT;
        } else {
            anlyStep = FIND_CMD_START;
        }
    } else if (frameState == TRANSMISSION_BIT) {
        if (mSettings->mSampleRead == RISING_EDGE) {
            mResults->AddMarker(smpMid, AnalyzerResults::UpArrow, mSettings->mClockChannel);
        } else {
            mResults->AddMarker(smpMid, AnalyzerResults::DownArrow, mSettings->mClockChannel);
        }
        Frame frame;
        frame.mType = FRAME_DIR;
        frame.mFlags = 0;
        frame.mData1 = mCmd->GetBitState();
        frame.mStartingSampleInclusive = startOfNextFrame;
        frame.mEndingSampleInclusive = smpEnd - 1;
        mResults->AddFrame(frame);

        isCmd = mCmd->GetBitState(); // 1:CMD; 0:Response
        frameState = COMMAND;
        frameCounter = 6;
        startOfNextFrame = frame.mEndingSampleInclusive + 1;
        temp = 0;
    } else if (frameState == COMMAND) {
        if (mSettings->mSampleRead == RISING_EDGE) {
            mResults->AddMarker(smpMid, AnalyzerResults::UpArrow, mSettings->mClockChannel);
        } else {
            mResults->AddMarker(smpMid, AnalyzerResults::DownArrow, mSettings->mClockChannel);
        }
        temp = temp << 1 | mCmd->GetBitState();
        frameCounter--;

        if (frameCounter == 0) {
            Frame frame;
            frame.mFlags = 0;
            frame.mData1 = temp; // Select the first 6 bits
            frame.mData2 = 0;
            frame.mStartingSampleInclusive = startOfNextFrame;
            frame.mEndingSampleInclusive = smpEnd - 1;
            if (isCmd) { //bit1: Host to Slave
                if (isACMD(lastHostCmd, temp)) {
                    frame.mType = FRAME_ACMD;
                    lastHostCmd = (char)temp | 0x80;
                } else {
                    frame.mType = FRAME_CMD;
                    lastHostCmd = (char)temp;
                }
                respLength = 32;
                respType = 1;
            } else { //bit0: Slave to Host
                frame.mType = FRAME_RESP;
                frame.mFlags = lastHostCmd;
                if (lastHostCmd == 2 || lastHostCmd == 9 || lastHostCmd == 10) { //R2
                    respLength = 128;
                    respType = 2;
                } else {
                    respLength = 32;
                    respType = 1;
                }
            }
            mResults->AddFrame(frame);
            frameState = ARGUMENT;
            frameCounter = respLength;
            startOfNextFrame = frame.mEndingSampleInclusive + 1;
            temp = 0;
        }
    } else if (frameState == ARGUMENT) {
        if (mSettings->mSampleRead == RISING_EDGE) {
            mResults->AddMarker(smpMid, AnalyzerResults::UpArrow, mSettings->mClockChannel);
        } else {
            mResults->AddMarker(smpMid, AnalyzerResults::DownArrow, mSettings->mClockChannel);
        }
        temp = temp << 1 | mCmd->GetBitState();
        frameCounter--;

        if (!isCmd && frameCounter == 1 && respType == 2) { //R2
            temp = temp << 1 | 0x01; //添加最低位,补1
            Frame frame;
            frame.mType = FRAME_LONG_ARG;
            frame.mFlags = 0;
            frame.mData1 = temp2; //高64位
            frame.mData2 = temp;  //低64位
            frame.mStartingSampleInclusive = startOfNextFrame;
            frame.mEndingSampleInclusive = smpEnd - 1;
            mResults->AddFrame(frame);

            frameState = STOP;
            frameCounter = 1;
            startOfNextFrame = frame.mEndingSampleInclusive + 1;
            temp = 0;
        } else if (frameCounter == 0) {
            Frame frame;
            frame.mType = FRAME_ARG;
            frame.mFlags = 0;
            frame.mData1 = temp; // Select the first 6 bits
            frame.mStartingSampleInclusive = startOfNextFrame;
            frame.mEndingSampleInclusive = smpEnd - 1;
            mResults->AddFrame(frame);

            frameState = CRC7;
            frameCounter = 7;
            startOfNextFrame = frame.mEndingSampleInclusive + 1;
            temp = 0;
        } else if (frameCounter == 64 && !isCmd) {
            temp2 = temp;
            temp = 0;
        }
    } else if (frameState == CRC7) {
        if (mSettings->mSampleRead == RISING_EDGE) {
            mResults->AddMarker(smpMid, AnalyzerResults::UpArrow, mSettings->mClockChannel);
        } else {
            mResults->AddMarker(smpMid, AnalyzerResults::DownArrow, mSettings->mClockChannel);
        }
        temp = temp << 1 | mCmd->GetBitState();
        frameCounter--;

        if (frameCounter == 0) {
            Frame frame;
            frame.mType = FRAME_CRC;
            frame.mFlags = 0;
            frame.mData1 = temp;
            frame.mStartingSampleInclusive = startOfNextFrame;
            frame.mEndingSampleInclusive = smpEnd - 1;
            mResults->AddFrame(frame);

            frameState = STOP;
            startOfNextFrame = frame.mEndingSampleInclusive + 1;
            temp = 0;
        }
    } else if (frameState == STOP) {
        mResults->AddMarker(smpMid, AnalyzerResults::Stop, mSettings->mCmdChannel);
        if (mSettings->mSampleRead == RISING_EDGE) {
            mResults->AddMarker(smpMid, AnalyzerResults::UpArrow, mSettings->mClockChannel);
        } else {
            mResults->AddMarker(smpMid, AnalyzerResults::DownArrow, mSettings->mClockChannel);
        }
        if (!mSettings->mDoNotGenerateStartFrames) {
            Frame frame;
            frame.mType = FRAME_STOP;
            frame.mFlags = 0;
            frame.mData1 = 1;
            frame.mStartingSampleInclusive = startOfNextFrame;
            frame.mEndingSampleInclusive = smpEnd - 1;
            mResults->AddFrame(frame);
        }
        mResults->CommitPacketAndStartNewPacket();
        frameState = START_BIT;
        return true;
    }
    return false;
}

bool SDIOAnalyzer::getDataLinesStartBit()
{
    U64 datSmpNum[4] = { 0 };
    U64 minSmpNum = 0; //数据线上，跳变沿首次出现的位置(mDATx采样点至mCmd下降沿)
    bool isStartBit = false;//mDATx上有下降沿，则总线上是数据传输
    while (!isStartBit && minSmpNum != cmdStartSmpNum) {
		if (mDAT0->WouldAdvancingToAbsPositionCauseTransition(cmdStartSmpNum)) {
            datSmpNum[0] = mDAT0->GetSampleOfNextEdge();
            if (mDAT0->GetBitState()) {
                isStartBit = true;
            }
        } else {
            datSmpNum[0] = cmdStartSmpNum;
        }
        minSmpNum = datSmpNum[0];
        if (mDAT1 != NULL) {
            if (mDAT1->WouldAdvancingToAbsPositionCauseTransition(cmdStartSmpNum)) {
                datSmpNum[1] = mDAT1->GetSampleOfNextEdge();
                BitState bit = mDAT1->GetBitState();
                if (datSmpNum[1] < minSmpNum) {
                    minSmpNum = datSmpNum[1];
                    if (bit) {
                        isStartBit = true;
                    } else {
                        isStartBit = false;
                    }
                } else if (datSmpNum[1] == minSmpNum) {
                    if (!isStartBit) {
                        if (bit) {
                            isStartBit = true;
                        }
                    }
                }
            } else {
                datSmpNum[1] = cmdStartSmpNum;
            }
        }
        if (mDAT2 != NULL) {
            if (mDAT2->WouldAdvancingToAbsPositionCauseTransition(cmdStartSmpNum)) {
                datSmpNum[2] = mDAT2->GetSampleOfNextEdge();
                BitState bit = mDAT2->GetBitState();
                if (datSmpNum[2] < minSmpNum) {
                    minSmpNum = datSmpNum[2];
                    if (bit) {
                        isStartBit = true;
                    } else {
                        isStartBit = false;
                    }
                } else if (datSmpNum[2] == minSmpNum) {
                    if (!isStartBit) {
                        if (bit) {
                            isStartBit = true;
                        }
                    }
                }
            } else {
                datSmpNum[2] = cmdStartSmpNum;
            }
        }
        if (mDAT3 != NULL) {
            if (mDAT3->WouldAdvancingToAbsPositionCauseTransition(cmdStartSmpNum)) {
                datSmpNum[3] = mDAT3->GetSampleOfNextEdge();
                BitState bit = mDAT3->GetBitState();
                if (datSmpNum[3] < minSmpNum) {
                    minSmpNum = datSmpNum[3];
                    if (bit) {
                        isStartBit = true;
                    } else {
                        isStartBit = false;
                    }
                } else if (datSmpNum[3] == minSmpNum) {
                    if (!isStartBit) {
                        if (bit) {
                            isStartBit = true;
                        }
                    }
                }
            } else {
                datSmpNum[3] = cmdStartSmpNum;
            }
        }

        if (!isStartBit) { //移动至CMD的下降沿
            mClock->AdvanceToAbsPosition(minSmpNum);
            mDAT0->AdvanceToAbsPosition(minSmpNum);
            if (mDAT1 != NULL) {
                mDAT1->AdvanceToAbsPosition(minSmpNum);
            }
            if (mDAT2 != NULL) {
                mDAT2->AdvanceToAbsPosition(minSmpNum);
            }
            if (mDAT3 != NULL) {
                mDAT3->AdvanceToAbsPosition(minSmpNum);
            }
        }
    }

    if (!isStartBit) {
        return false;
    }

    mClock->AdvanceToAbsPosition(minSmpNum); //如果存在DATA，则将mClock移动到DATA的起始位
    mDAT0->AdvanceToAbsPosition(minSmpNum);
    if (mDAT1 != NULL) {
        mDAT1->AdvanceToAbsPosition(minSmpNum);
    }
    if (mDAT2 != NULL) {
        mDAT2->AdvanceToAbsPosition(minSmpNum);
    }
    if (mDAT3 != NULL) {
        mDAT3->AdvanceToAbsPosition(minSmpNum);
    }

    if (mSettings->mSampleRead == RISING_EDGE) { //上升沿读取数据
        if (mClock->GetBitState() == BIT_HIGH) { // 移动到下降沿
            mClock->AdvanceToNextEdge();
        }
    } else {
        if (mClock->GetBitState() == BIT_LOW) { //移动到上升沿
            mClock->AdvanceToNextEdge();
        }
    }

    mClock->AdvanceToNextEdge(); //移动到读取数据的边沿
//    if (!mSettings->mDoNotGenerateDatFrames) {
//        mResults->AddMarker(mClock->GetSampleNumber(), AnalyzerResults::Start, mSettings->mClockChannel);
//    }
    mClock->AdvanceToNextEdge(); //后移一个边沿
    dataStartSmpNum = mClock->GetSampleNumber(); //下一个Frame的起始位置
    mClock->AdvanceToNextEdge(); //后移一个边沿

    return true;
}

bool SDIOAnalyzer::readDataLines()
{
    DataBuilder dataData;
    U64 dataValue = 0;
    U64 startSmp = 0;
    U32 i = 0;
    bool quit = false;

    // advance all the data lines to our clock
    clkCurrentSmpNum = mClock->GetSampleNumber();
    mDAT0->AdvanceToAbsPosition(clkCurrentSmpNum);
    if (NULL != mDAT1) {
        mDAT1->AdvanceToAbsPosition(clkCurrentSmpNum);
    }
    if (NULL != mDAT2) {
        mDAT2->AdvanceToAbsPosition(clkCurrentSmpNum);
    }
    if (NULL != mDAT3) {
        mDAT3->AdvanceToAbsPosition(clkCurrentSmpNum);
    }

    startSmp = dataStartSmpNum;
    dataData.Reset(&dataValue, AnalyzerEnums::MsbFirst, 8);
    dataNum++;
    for (i = 0; i < 8;) {
        if (!mSettings->mDoNotGenerateDatFrames) {
            if (mSettings->mSampleRead == RISING_EDGE) {
                mResults->AddMarker(clkCurrentSmpNum, AnalyzerResults::UpArrow, mSettings->mClockChannel);
            } else {
                mResults->AddMarker(clkCurrentSmpNum, AnalyzerResults::DownArrow, mSettings->mClockChannel);
            }
        }
        if (NULL != mDAT3) {
            i++;
            dataData.AddBit(mDAT3->GetBitState());
        }
        if (NULL != mDAT2) {
            i++;
            dataData.AddBit(mDAT2->GetBitState());
        }
        if (NULL != mDAT1) {
            i++;
            dataData.AddBit(mDAT1->GetBitState());
        }
        if (NULL != mDAT0) {
            i++;
            dataData.AddBit(mDAT0->GetBitState());
        }

        // 获取bit结束位
        mClock->AdvanceToNextEdge();
        dataStartSmpNum = mClock->GetSampleNumber();

        clkCurrentSmpNum = mClock->GetSampleOfNextEdge(); //下一个数据读取边沿
        if (clkCurrentSmpNum >= cmdStartSmpNum) {
            if (mCmd->GetBitState() == BIT_HIGH) {
                mCmd->AdvanceToNextEdge();
                mCmd->AdvanceToNextEdge();
                cmdStartSmpNum = mCmd->GetSampleNumber();
            } else {
                quit = true;
                if (!mSettings->mDoNotGenerateDatFrames) {
                    if (i == 8) {
                        Frame frame;
                        frame.mType = DATA;
                        frame.mFlags = 0;
                        frame.mData1 = dataValue;
                        frame.mStartingSampleInclusive = startSmp;
                        frame.mEndingSampleInclusive = dataStartSmpNum - 1;
                        mResults->AddFrame(frame);
                    }
                }
                break;
            }
        }
        mClock->AdvanceToNextEdge();
        if (NULL != mDAT0) {
            mDAT0->AdvanceToAbsPosition(clkCurrentSmpNum);
        }
        if (NULL != mDAT1) {
            mDAT1->AdvanceToAbsPosition(clkCurrentSmpNum);
        }
        if (NULL != mDAT2) {
            mDAT2->AdvanceToAbsPosition(clkCurrentSmpNum);
        }
        if (NULL != mDAT3) {
            mDAT3->AdvanceToAbsPosition(clkCurrentSmpNum);
        }
    }
    if (quit) {
        return false;
    }

    if (!mSettings->mDoNotGenerateDatFrames) {
        Frame frame;
        frame.mType = DATA;
        frame.mFlags = 0;
        frame.mData1 = dataValue;
        frame.mStartingSampleInclusive = startSmp;
        frame.mEndingSampleInclusive = dataStartSmpNum - 1;
        mResults->AddFrame(frame);
        mResults->CommitResults();
        ReportProgress(frame.mEndingSampleInclusive);
    }
    if (dataNum == 514) { //CRC16
        dataNum = 0;
        if (!mSettings->mDoNotGenerateDatFrames) {
//            if (mDAT0->GetBitState()) {
//                mResults->AddMarker(clkCurrentSmpNum, AnalyzerResults::Stop, mSettings->mClockChannel);
//            }
            mResults->CommitPacketAndStartNewPacket();
        }
        U64 smp = mDAT0->GetSampleOfNextEdge();
        if (smp >= cmdStartSmpNum) {
            return false;
        }
        mClock->AdvanceToAbsPosition(smp);
        if (mSettings->mSampleRead == RISING_EDGE) {
            if (mClock->GetBitState()) {
                mClock->AdvanceToNextEdge();
            }
        } else {
            if (mClock->GetBitState() == BIT_LOW) {
                mClock->AdvanceToNextEdge();
            }
        }
        mClock->AdvanceToNextEdge();
//        if (!mSettings->mDoNotGenerateDatFrames) {
//            mResults->AddMarker(mClock->GetSampleNumber(), AnalyzerResults::Start, mSettings->mClockChannel);
//        }
        mClock->AdvanceToNextEdge();
        dataStartSmpNum = mClock->GetSampleNumber();

        mClock->AdvanceToNextEdge();
        smp = mClock->GetSampleNumber();
        if (NULL != mDAT0) {
            mDAT0->AdvanceToAbsPosition(smp);
        }
        if (NULL != mDAT1) {
            mDAT1->AdvanceToAbsPosition(smp);
        }
        if (NULL != mDAT2) {
            mDAT2->AdvanceToAbsPosition(smp);
        }
        if (NULL != mDAT3) {
            mDAT3->AdvanceToAbsPosition(smp);
        }
    }
    ReportProgress(mClock->GetSampleNumber());
    return true;
}

// SD卡专用ACMD,其他ACMD命令为制造商定义,暂不解析
//ACMD6、ACMD13、ACMD17-25、ACMD38-49、ACMD51。SD存储卡专用保留
bool SDIOAnalyzer::isACMD(U32 cmd1, U64 cmd2)
{
    if (cmd1 == 55 && cmd2 != 55) {
        if (cmd2 == 6 || cmd2 == 13 || cmd2 == 51) {
            return true;
        }
        if (cmd2 >= 17 && cmd2 <= 25) {
            return true;
        }
        if (cmd2 >= 38 && cmd2 <= 49) {
            return true;
        }
    }
    return false;
}

bool SDIOAnalyzer::NeedsRerun()
{
    return false;
}

U32 SDIOAnalyzer::GenerateSimulationData(U64 minimum_sample_index, U32 device_sample_rate, SimulationChannelDescriptor **simulation_channels)
{
    if (mSimulationInitilized == false) {
        mSimulationDataGenerator.Initialize(GetSimulationSampleRate(), mSettings.get());
        mSimulationInitilized = true;
    }

    return mSimulationDataGenerator.GenerateSimulationData(minimum_sample_index, device_sample_rate, simulation_channels);
}

U32 SDIOAnalyzer::GetMinimumSampleRateHz()
{
    return 1000000;
}

const char *SDIOAnalyzer::GetAnalyzerName() const
{
    return "SDIO";
}

const char *GetAnalyzerName()
{
    return "SDIO";
}

Analyzer *CreateAnalyzer()
{
    return new SDIOAnalyzer();
}

void DestroyAnalyzer(Analyzer *analyzer)
{
    delete analyzer;
}
