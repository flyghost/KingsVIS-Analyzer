#include "SDIOAnalyzerResults.h"
#include <AnalyzerHelpers.h>
#include "SDIOAnalyzer.h"
#include "SDIOAnalyzerSettings.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <stdio.h>
#include <string.h>

struct CMD_Packet {
    U8 cmd;
    std::string description;
    std::string resp;
};

struct ACMD_Packet {
    U8 cmd;
    std::string description;
};

CMD_Packet Commands_packets[] = {
    { 0, "GO_IDLE_STATE", "" },
    { 1, "", "" },
    { 2, "ALL_SEND_CID", "R2" },
    { 3, "SEND_RELATIVE_ADDR", "R6" },
    { 4, "SET_DSR", "" },
    { 5, "IO_SEND_OP_COND", "R4" },
    { 6, "SWITCH_FUNC", "" }, //respone DAT
    { 7, "SELECT/DESELECT_CARD", "R1b" },
    { 8, "SEND_IF_COND", "R7"}, //V2.0
    { 9, "SEND_CSD", "R2" },
    { 10, "SEND_CID", "R2" },
    { 11, "VOLTAGE_SWITCH", "R1" }, //??
    { 12, "STOP_TRANSMISSION", "R1b" },
    { 13, "SEND_STATUS", "R1" },
    { 14, "", "" },
    { 15, "GO_INACTIVE_STATE", "" },
    { 16, "SET_BLOCKLEN", "R1" },
    { 17, "READ_SINGLE_BLOCK", "R1"},
    { 18, "READ_MULTIPLE_BLOCK", "R1"},
    { 19, "", "" },
    { 20, "", "" },
    { 21, "", "" },
    { 22, "", "" },
    { 23, "", "" },
    { 24, "WRITE_BLOCK", "R1" },
    { 25, "WRITE_MULTIPLE_BLOCK", "R1" },
    { 26, "", ""},
    { 27, "PROGRAM_CSD", "R1" },
    { 28, "SET_WRITE_PROT", "R1b"},
    { 29, "CLR_WRITE_PROT", "R1b" },
    { 30, "SEND_WRITE_PROT", "R1" },
    { 31, "", "" },
    { 32, "ERASE_WR_BLK_START", "R1" },
    { 33, "ERASE_WR_BLK_END", "R1"},
    { 34, "", ""},
    { 35, "", "" },
    { 36, "", "" },
    { 37, "", "" },
    { 38, "ERASE", "R1b" },
    { 39, "", "" },
    { 40, "", "" },
    { 41, "", "" },
    { 42, "LOCK_UNLOCK", "R1"},
    { 43, "", "" },
    { 44, "", ""},
    { 45, "", "" },
    { 46, "", "" },
    { 47, "", "" },
    { 48, "", "" },
    { 49, "", "" },
    { 50, "", "" },
    { 51, "", "" },
    { 52, "IO_RW_DIRECT", "R5" },
    { 53, "IO_RW_EXTENDED", "" }, //SDIO协议，代替SD协议CMD17,18,24,25
    { 54, "", "" },
    { 55, "APP_CMD", "R1" },
    { 56, "GEN_CMD", "R1" },
};

//先发送一条普通的不带参数的CMD55命令
ACMD_Packet ACMD_packets[] = {
    { 6, "SET_BUS_WIDTH" },
    { 13, "SD_STATUS" },
    { 22, "SEND_NUM_WR_BLOCKS" },
    { 23, "SET_WR_BLK_ERASE_COUNT" },
    { 41, "SD_SEND_OP_COND" }, //或 SD_APP_OP_COND
    { 42, "SET_CLR_CARD_DETECT" },
    { 51, "SEND_SCR" },
};
U8 commandsPacketCount = (U8)(sizeof(Commands_packets) / sizeof(Commands_packets[0]));
U8 ACMDPacketCount = (U8)(sizeof(ACMD_packets) / sizeof(ACMD_packets[0]));

// --------------------------------------------------------------------------------------------------
SDIOAnalyzerResults::SDIOAnalyzerResults(SDIOAnalyzer *analyzer, SDIOAnalyzerSettings *settings)
    :   AnalyzerResults(),
        mSettings(settings),
        mAnalyzer(analyzer)
{
}

SDIOAnalyzerResults::~SDIOAnalyzerResults()
{
}

void SDIOAnalyzerResults::GenerateBubbleText(U64 frame_index, Channel &channel, DisplayBase display_base)
{
    ClearResultStrings();
    Frame frame = GetFrame(frame_index);

    char number_str[128];
    if (channel == mSettings->mDAT0Channel) {
        if (frame.mType == SDIOAnalyzer::DATA) {
            AddResultString("D");
            AnalyzerHelpers::GetNumberString(frame.mData1, display_base, 8, number_str, 128);
            AddResultString(number_str);
        }
    } else {
        if (frame.mType == SDIOAnalyzer::FRAME_DIR) {
            AddResultString("D");
            AddResultString("DIR");
            if (frame.mData1) {
                AddResultString("DIR: Host  ---> Slave");
            } else {
                AddResultString("DIR: Slave ---> Host");
            }
        } else if (frame.mType == SDIOAnalyzer::FRAME_CMD) {
            AddResultString("C");
            AddResultString("CMD");
            AnalyzerHelpers::GetNumberString(frame.mData1, Decimal, 6, number_str, 128);
            AddResultString("CMD", number_str);
            std::string str = getCmdDescription((U8)frame.mData1);
            AddResultString("CMD", number_str, ": ", str.c_str());
        } else if (frame.mType == SDIOAnalyzer::FRAME_ACMD) {
            AddResultString("A");
            AddResultString("ACMD");
            AnalyzerHelpers::GetNumberString(frame.mData1, Decimal, 6, number_str, 128);
            AddResultString("ACMD", number_str);
            std::string str = getAcmdDescription((U8)frame.mData1);
            AddResultString("ACMD", number_str, ": ", str.c_str());
        } else if (frame.mType == SDIOAnalyzer::FRAME_RESP) {
            AddResultString("R");
            AnalyzerHelpers::GetNumberString(frame.mData1, Decimal, 6, number_str, 128);
            if (frame.mFlags&0x80) { //ACMD的Response
                if ((frame.mFlags&0x3F) == 41) {
                    AddResultString("R3");
                    AddResultString("R3-", number_str);
                } else {
                    AddResultString("R1");
                    AddResultString("R1-", number_str);
                }
            } else { //CMD的Response
                std::string str = getCmdResponse((U8)frame.mFlags);
                AddResultString(str.c_str());
                AddResultString(str.c_str(), "-", number_str);
            }
        } else if (frame.mType == SDIOAnalyzer::FRAME_ARG) {
            AddResultString("A");
            AddResultString("ARG");
            AnalyzerHelpers::GetNumberString(frame.mData1, display_base, 32, number_str, 128);
            AddResultString("ARG:", number_str);
        } else if (frame.mType == SDIOAnalyzer::FRAME_LONG_ARG) {
            AddResultString("L");
            AddResultString("LONG");
            if (display_base == Decimal) {
                AnalyzerHelpers::GetNumberString((frame.mData1>>32), Decimal, 32, number_str, 128);
                U32 len = strlen(number_str);
                number_str[len] = '-';

                AnalyzerHelpers::GetNumberString(frame.mData1&0x0FFFFFFFF, Decimal, 32, &number_str[len+1], 127-len);
                len = strlen(number_str);
                number_str[len] = '-';

                AnalyzerHelpers::GetNumberString((frame.mData2>>32), Decimal, 32, &number_str[len+1], 127-len);
                len = strlen(number_str);
                number_str[len] = '-';

                AnalyzerHelpers::GetNumberString(frame.mData2&0x0FFFFFFFF, Decimal, 32, &number_str[len+1], 127-len);

                AddResultString("LONG_ARG: ", number_str);
            } else if (display_base == ASCII || display_base == AsciiHex) {
                AnalyzerHelpers::GetNumberString((frame.mData1>>32), ASCII, 32, number_str, 128);
                U32 strLen = strlen(number_str);

                AnalyzerHelpers::GetNumberString(frame.mData1&0x0FFFFFFFF, ASCII, 32, &number_str[strLen], 128-strLen);
                strLen = strlen(number_str);

                AnalyzerHelpers::GetNumberString((frame.mData2>>32), ASCII, 32, &number_str[strLen], 128-strLen);
                strLen = strlen(number_str);

                AnalyzerHelpers::GetNumberString(frame.mData2&0x0FFFFFFFF, ASCII, 32, &number_str[strLen], 128-strLen);

                if (display_base == ASCII) {
                    AddResultString("LONG_ARG: ", number_str);
                } else {
                    char number_str1[128];
                    char number_str2[128];
                    AnalyzerHelpers::GetNumberString(frame.mData1, Hexadecimal, 64, number_str1, 128);
                    AnalyzerHelpers::GetNumberString(frame.mData2, Hexadecimal, 64, number_str2, 128);
                    AddResultString("LONG_ARG: ", number_str, "(", number_str1, &number_str2[2], ")");
                }
            } else {
                char number_str2[128];
                AnalyzerHelpers::GetNumberString(frame.mData1, display_base, 64, number_str, 128);
                AnalyzerHelpers::GetNumberString(frame.mData2, display_base, 64, number_str2, 128);
                AddResultString("LONG_ARG: ", number_str, &number_str2[2]);
            }
        } else if (frame.mType == SDIOAnalyzer::FRAME_CRC) {
            AddResultString("CRC");
            AnalyzerHelpers::GetNumberString(frame.mData1, display_base, 7, number_str, 128);
            AddResultString("CRC:", number_str);
        } else if (frame.mType == SDIOAnalyzer::FRAME_START) {
            AddResultString("S");
            AddResultString("START");
        } else if (frame.mType == SDIOAnalyzer::FRAME_STOP) {
            AddResultString("E");
            AddResultString("END");
        }
    }
}

void SDIOAnalyzerResults::GenerateExportFile(const char *file, DisplayBase display_base, U32 export_type_user_id)
{
    void *f = AnalyzerHelpers::StartFile(file);
    std::stringstream ss;
    if (!mSettings->mDoNotGenerateDatFrames) {
        ss << "Time [s],Type,Direction,CmdIndex,Argument/Data,CRC" << std::endl;
    } else {
        ss << "Time [s],Direction,CmdIndex,Argument/Data,CRC" << std::endl;
    }

    char number_str[128];
    U64 trigger_sample = mAnalyzer->GetTriggerSample();
    U32 sample_rate = mAnalyzer->GetSampleRate();
    U64 num_packets = GetNumPackets();
    for (U64 i = 0; i < num_packets; i++) {
        U64 first_frame_id = 0;
        U64 last_frame_id = 0;
        GetFramesContainedInPacket(i, &first_frame_id, &last_frame_id);
        Frame frame = GetFrame(first_frame_id);
        // -- Packet起始时间
        char time_str[128];
        AnalyzerHelpers::GetTimeString(frame.mStartingSampleInclusive, trigger_sample, sample_rate, time_str, 128);
        ss << time_str;
        // -- Packet类型
        if (!mSettings->mDoNotGenerateDatFrames) {
            if (frame.mType == SDIOAnalyzer::DATA) {
                ss << "," << "DATA";
            } else {
                ss << "," << "CMD";
            }
        }
        // ---
        for (U64 index = first_frame_id; index <= last_frame_id; index++) {
            frame = GetFrame(index);
            if (frame.mType == SDIOAnalyzer::FRAME_DIR) {
                if (frame.mData1) {
                    ss << "," << "Host";
                } else {
                    ss << "," << "Slave";
                }
            } else if (frame.mType == SDIOAnalyzer::FRAME_CMD) {
                AnalyzerHelpers::GetNumberString(frame.mData1, Decimal, 6, number_str, 128);
                std::string str = getCmdDescription((U8)frame.mData1);
                ss << "," << "CMD" << number_str << "(" << str.c_str() << ")";
            } else if (frame.mType == SDIOAnalyzer::FRAME_ACMD) {
                AnalyzerHelpers::GetNumberString(frame.mData1, Decimal, 6, number_str, 128);
                std::string str = getAcmdDescription((U8)frame.mData1);
                ss << "," << "ACMD" << number_str << "(" << str.c_str() << ")";
            } else if (frame.mType == SDIOAnalyzer::FRAME_RESP) {
                AnalyzerHelpers::GetNumberString(frame.mData1, Decimal, 6, number_str, 128);
                if (frame.mFlags&0x80) { //ACMD的Response
                    if ((frame.mFlags&0x3F) == 41) {
                        ss << "," << "R3-" << number_str;
                    } else {
                        ss << "," << "R1-" << number_str;
                    }
                } else { //CMD的Response
                    std::string str = getCmdResponse((U8)frame.mFlags);
                    ss << "," << str.c_str() << "-" << number_str;
                }
            } else if (frame.mType == SDIOAnalyzer::FRAME_ARG) {
                AnalyzerHelpers::GetNumberString(frame.mData1, display_base, 32, number_str, 128);
                ss << "," << number_str;
            } else if (frame.mType == SDIOAnalyzer::FRAME_LONG_ARG) {
                if (display_base == Decimal) {
                    AnalyzerHelpers::GetNumberString((frame.mData1>>32), Decimal, 32, number_str, 128);
                    ss << "," << number_str;

                    AnalyzerHelpers::GetNumberString(frame.mData1&0x0FFFFFFFF, Decimal, 32, number_str, 128);
                    ss << "-" << number_str;

                    AnalyzerHelpers::GetNumberString((frame.mData2>>32), Decimal, 32, number_str, 128);
                    ss << "-" << number_str;

                    AnalyzerHelpers::GetNumberString(frame.mData2&0x0FFFFFFFF, Decimal, 32, number_str, 128);
                    ss << "-" << number_str;
                } else if (display_base == ASCII || display_base == AsciiHex) {
                    AnalyzerHelpers::GetNumberString((frame.mData1>>32), ASCII, 32, number_str, 128);
                    ss << "," << number_str;

                    AnalyzerHelpers::GetNumberString(frame.mData1&0x0FFFFFFFF, ASCII, 32, number_str, 128);
                    ss << "-" << number_str;

                    AnalyzerHelpers::GetNumberString((frame.mData2>>32), ASCII, 32, number_str, 128);
                    ss << "-" << number_str;

                    AnalyzerHelpers::GetNumberString(frame.mData2&0x0FFFFFFFF, ASCII, 32, number_str, 128);
                    ss << "-" << number_str;

                    if (display_base == AsciiHex) {
                        char number_str1[128];
                        char number_str2[128];
                        AnalyzerHelpers::GetNumberString(frame.mData1, Hexadecimal, 64, number_str1, 128);
                        AnalyzerHelpers::GetNumberString(frame.mData2, Hexadecimal, 64, number_str2, 128);
                        ss << "(" << number_str1 << &number_str2[2] << ")";
                    }
                } else {
                    char number_str2[128];
                    AnalyzerHelpers::GetNumberString(frame.mData1, display_base, 64, number_str, 128);
                    AnalyzerHelpers::GetNumberString(frame.mData2, display_base, 64, number_str2, 128);
                    ss << "," << number_str << &number_str2[2];
                }
            } else if (frame.mType == SDIOAnalyzer::FRAME_CRC) {
                AnalyzerHelpers::GetNumberString(frame.mData1, display_base, 7, number_str, 128);
                ss << "," << number_str;
            } else if (frame.mType == SDIOAnalyzer::DATA) {
                AnalyzerHelpers::GetNumberString(frame.mData1, display_base, 8, number_str, 128);
                if ( index == first_frame_id) {
                    ss << ",,," << number_str;
                } else {
                    ss << " " << number_str;
                }
            }
        }
        if (frame.mType == SDIOAnalyzer::DATA) {
            ss << ","; //填充"CRC"
        }
        ss << std::endl;
        AnalyzerHelpers::AppendToFile((U8 *)ss.str().c_str(), ss.str().length(), f);
        ss.str(std::string());
        if (UpdateExportProgressAndCheckForCancel(i, num_packets) == true) {
            AnalyzerHelpers::EndFile(f);
            return;
        }
    }
    U64 num_frames = GetNumFrames();
    UpdateExportProgressAndCheckForCancel(num_frames, num_frames);
    AnalyzerHelpers::EndFile(f);
}

/**
 * 解析数据：用于右下角显示解析出来的格式.
 * 
 * \param frame_index
 * \param display_base
 */
void SDIOAnalyzerResults::GenerateFrameTabularText(U64 frame_index, DisplayBase display_base)
{
    ClearTabularText();
    Frame frame = GetFrame(frame_index);

    char number_str[128];
    if (frame.mType == SDIOAnalyzer::FRAME_DIR) 
    {
        if (frame.mData1) {
            AddTabularText("DIR: Host  ---> Slave");
        } else {
            AddTabularText("DIR: Slave ---> Host");
        }
    } 
    else if (frame.mType == SDIOAnalyzer::FRAME_CMD) 
    {
        AnalyzerHelpers::GetNumberString(frame.mData1, Decimal, 6, number_str, 128);
        std::string str = getCmdDescription((U8)frame.mData1);
        AddTabularText("CMD", number_str, ": ", str.c_str());
    } 
    else if (frame.mType == SDIOAnalyzer::FRAME_ACMD) 
    {
        AnalyzerHelpers::GetNumberString(frame.mData1, Decimal, 6, number_str, 128);
        std::string str = getAcmdDescription((U8)frame.mData1);
        AddTabularText("ACMD", number_str, ": ", str.c_str());
    } 
    else if (frame.mType == SDIOAnalyzer::FRAME_RESP) 
    {
        AnalyzerHelpers::GetNumberString(frame.mData1, Decimal, 6, number_str, 128);
        if (frame.mFlags&0x80) { //ACMD的Response
            if ((frame.mFlags&0x3F) == 41) {
                AddTabularText("R3-", number_str);
            } else {
                AddTabularText("R1-", number_str);
            }
        } else { //CMD的Response
            std::string str = getCmdResponse((U8)frame.mFlags);
            AddTabularText(str.c_str(), "-", number_str);
        }
    } 
    else if (frame.mType == SDIOAnalyzer::FRAME_ARG) 
    {
        AnalyzerHelpers::GetNumberString(frame.mData1, display_base, 32, number_str, 128);
        AddTabularText("ARG: ", number_str);
    } 
    else if (frame.mType == SDIOAnalyzer::FRAME_LONG_ARG) 
    {
        if (display_base == Decimal) {
            AnalyzerHelpers::GetNumberString((frame.mData1>>32), Decimal, 32, number_str, 128);
            U32 strLen = strlen(number_str);
            number_str[strLen] = '-';

            AnalyzerHelpers::GetNumberString(frame.mData1&0x0FFFFFFFF, Decimal, 32, &number_str[strLen+1], 127-strLen);
            strLen = strlen(number_str);
            number_str[strLen] = '-';

            AnalyzerHelpers::GetNumberString((frame.mData2>>32), Decimal, 32, &number_str[strLen+1], 127-strLen);
            strLen = strlen(number_str);
            number_str[strLen] = '-';

            AnalyzerHelpers::GetNumberString(frame.mData2&0x0FFFFFFFF, Decimal, 32, &number_str[strLen+1], 127-strLen);

            AddTabularText("LONG_ARG: ", number_str);
        } else if (display_base == ASCII || display_base == AsciiHex) {
            AnalyzerHelpers::GetNumberString((frame.mData1>>32), ASCII, 32, number_str, 128);
            U32 len = strlen(number_str);

            AnalyzerHelpers::GetNumberString(frame.mData1&0x0FFFFFFFF, ASCII, 32, &number_str[len], 128-len);
            len = strlen(number_str);

            AnalyzerHelpers::GetNumberString((frame.mData2>>32), ASCII, 32, &number_str[len], 128-len);
            len = strlen(number_str);

            AnalyzerHelpers::GetNumberString(frame.mData2&0x0FFFFFFFF, ASCII, 32, &number_str[len], 128-len);

            if (display_base == ASCII) {
                AddTabularText("LONG_ARG: ", number_str);
            } else {
                char number_str1[128];
                char number_str2[128];
                AnalyzerHelpers::GetNumberString(frame.mData1, Hexadecimal, 64, number_str1, 128);
                AnalyzerHelpers::GetNumberString(frame.mData2, Hexadecimal, 64, number_str2, 128);
                AddTabularText("LONG_ARG: ", number_str, "(", number_str1, &number_str2[2], ")");
            }
        } else {
            char number_str2[128];
            AnalyzerHelpers::GetNumberString(frame.mData1, display_base, 64, number_str, 128);
            AnalyzerHelpers::GetNumberString(frame.mData2, display_base, 64, number_str2, 128);
            AddTabularText("LONG_ARG: ", number_str, &number_str2[2]);
        }
    } else if (frame.mType == SDIOAnalyzer::FRAME_CRC) {
        AnalyzerHelpers::GetNumberString(frame.mData1, display_base, 7, number_str, 128);
        AddTabularText("CRC: ", number_str);
    } else if (frame.mType == SDIOAnalyzer::DATA) {
        AnalyzerHelpers::GetNumberString(frame.mData1, display_base, 8, number_str, 128);
        AddTabularText(number_str);
    } else if (frame.mType == SDIOAnalyzer::FRAME_START) {
        AddTabularText("========== START ==========");
    } else if (frame.mType == SDIOAnalyzer::FRAME_STOP) {
        AddTabularText("==========  END  ==========");
    }
}

void SDIOAnalyzerResults::GeneratePacketTabularText(U64 packet_id, DisplayBase display_base)
{
    ClearResultStrings();
    AddResultString("not supported");
}

void SDIOAnalyzerResults::GenerateTransactionTabularText(U64 transaction_id, DisplayBase display_base)
{
    ClearResultStrings();
    AddResultString("not supported");
}

/**
 * 获取该CMD的名称.
 * 
 * \param index
 * \return 
 */
std::string SDIOAnalyzerResults::getCmdDescription(U8 index)
{
    if (index < commandsPacketCount) {
        if (Commands_packets[index].description.size()) {
            return Commands_packets[index].description;
        }
    }
    return "Unknown";
}

std::string SDIOAnalyzerResults::getCmdResponse(U8 index)
{
    if (index < commandsPacketCount) {
        if (Commands_packets[index].resp.size()) {
            return Commands_packets[index].resp;
        }
    }
    return "R1";
}

std::string SDIOAnalyzerResults::getAcmdDescription(U8 index)
{
    for (U32 i = 0; i < ACMDPacketCount; i++) {
        if (ACMD_packets[i].cmd == index) {
            if (ACMD_packets[i].description.size()) {
                return ACMD_packets[i].description;
            }
            break;
        }
    }
    return "Unknown";
}

