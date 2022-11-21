#ifndef ANALYZER_HELPERS_H
#define ANALYZER_HELPERS_H

#include "Analyzer.h"

class LOGICAPI AnalyzerHelpers
{
public:
    // 偶校验判断
    static bool IsEven(U64 value);

    // 奇校验判断
    static bool IsOdd(U64 value);

    // 获取数据位数
    static U32 GetOnesCount(U64 value);

    // 获取两个数的差（a-b）
    static U32 Diff32(U32 a, U32 b);

    /**
     * @brief 将 number 转成指定格式的字符串
     * 
     * 根据实际可用的显示空间，显示信息可以是不同长度的字符串。应该生成多种结果字符串，
     * 最简单的可能只需表示内容的类型（比如“D”代表数据），长些的可能会表示整个数据（比如“0xFF01”），
     * 也可能会非常长（比如“Left Channel Audio Data: 0xFF01”）。
     * 设置多个字符串是为了当软件显示解析结果时，对界面上的波形进行缩放后，会呈现出不同的解析信息，
     * 当显示区域短时显示短的字符串，长的时候显示长的字符串
     * 
     * @param number 要转化的数
     * @param display_base 要转换成的格式
     * @param num_data_bits 数的实际位数
     * @param result_string 转换成的字符串
     * @param result_string_max_length 字符串的最大长度
     */
    static void GetNumberString(U64 number, DisplayBase display_base, U32 num_data_bits, char *result_string, U32 result_string_max_length);
    
    /**
     * @brief 将与结果相关联的时间转换成字符串
     * 
     * @param sample 当前的采样点数
     * @param trigger_sample 为触发位置的采样点数
     * @param sample_rate_hz 采样频率
     * @param result_string 转换成的 字符串指针
     * @param result_string_max_length 字符串的最大长度
     */
    static void GetTimeString(U64 sample, U64 trigger_sample, U32 sample_rate_hz, char *result_string, U32 result_string_max_length);

    static void Assert(const char *message);
    static U64 AdjustSimulationTargetSample(U64 target_sample, U32 sample_rate, U32 simulation_sample_rate);

    static bool DoChannelsOverlap(const Channel *channel_array, U32 num_channels);
    static void SaveFile(const char *file_name, const U8 *data, U32 data_length, bool is_binary = false);

    static S64 ConvertToSignedNumber(U64 number, U32 num_bits);

    //These save functions should not be used with SaveFile, above. They are a better way to export data (don't waste memory), and should be used from now on.
    static void *StartFile(const char *file_name, bool is_binary = false);
    static void AppendToFile(const U8 *data, U32 data_length, void *file);
    static void EndFile(void *file);
};


struct ClockGeneratorData;
class LOGICAPI ClockGenerator
{
public:
    ClockGenerator();
    ~ClockGenerator();

    /**
     * @brief 初始化
     * 
     * @param target_frequency 想要模拟数据的频率（单位 Hz），比如 Serail 协议的波特 率、SPI 时钟的波特率等
     * @param sample_rate_hz 这个参数是生成数据的采样率，就是通过软件界面 上设置的采样率
     */
    void Init(double target_frequency, U32 sample_rate_hz);

    /**
     * @brief 返回移动半个周期所需的采样点数，以半个周期为基本单位。例如，要移动一个 周期时需要输入 2.0
     * 
     * @param multiple 
     * @return U32 
     */
    U32 AdvanceByHalfPeriod(double multiple = 1.0);

    /**
     * @brief 返回移动 time_s 时间所需的采样点数，时间单位为秒，输入 1e-6 表示 1 微妙
     * 
     * @param time_s 
     * @return U32 
     */
    U32 AdvanceByTimeS(double time_s);

protected:
    struct ClockGeneratorData *mData;
};


struct BitExtractorData;
class LOGICAPI BitExtractor
{
public:
    BitExtractor(U64 data, AnalyzerEnums::ShiftOrder shift_order, U32 num_bits);
    ~BitExtractor();

    BitState GetNextBit();

protected:
    struct BitExtractorData *mData;
};


struct DataBuilderData;
class LOGICAPI DataBuilder
{
public:
    DataBuilder();
    ~DataBuilder();

    void Reset(U64 *data, AnalyzerEnums::ShiftOrder shift_order, U32 num_bits);
    void AddBit(BitState bit);

protected:
    struct DataBuilderData *mData;
};


struct SimpleArchiveData;
class LOGICAPI SimpleArchive
{
public:
    SimpleArchive();
    ~SimpleArchive();

    void SetString(const char *archive_string);
    const char *GetString();

    bool operator<<(U64 data);
    bool operator<<(U32 data);
    bool operator<<(S64 data);
    bool operator<<(S32 data);
    bool operator<<(double data);
    bool operator<<(bool data);
    bool operator<<(const char *data);
    bool operator<<(Channel &data);

    bool operator>>(U64 &data);
    bool operator>>(U32 &data);
    bool operator>>(S64 &data);
    bool operator>>(S32 &data);
    bool operator>>(double &data);
    bool operator>>(bool &data);
    bool operator>>(char const **data);
    bool operator>>(Channel &data);

protected:
    struct SimpleArchiveData *mData;
};

#endif //ANALYZER_HELPERS_H
