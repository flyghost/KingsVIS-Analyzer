#ifndef SERIAL_ANALYZER_SETTINGS
#define SERIAL_ANALYZER_SETTINGS

#include <AnalyzerSettings.h>
#include <AnalyzerTypes.h>

namespace SerialAnalyzerEnums
{
    enum Mode { Normal, MpModeMsbZeroMeansAddress, MpModeMsbOneMeansAddress };
};

class SerialAnalyzerSettings : public AnalyzerSettings

{
public:
    SerialAnalyzerSettings();
    virtual ~SerialAnalyzerSettings();

    virtual bool SetSettingsFromInterfaces();
    void UpdateInterfacesFromSettings();
    virtual void LoadSettings(const char *settings);
    virtual const char *SaveSettings();

    Channel mInputChannel;                      // 通道，表明与逻辑分析仪的哪个通道相连
    U32 mBitRate;                               // 波特率
    U32 mBitsPerTransfer;                       // 使能信号的高低电平
    AnalyzerEnums::ShiftOrder mShiftOrder;      // MSB 或者 LSB
    double mStopBits;                           // 停止位
    AnalyzerEnums::Parity mParity;              // 奇偶校验
    bool mInverted;                             // 反向，颠倒
    bool mUseAutobaud;                          // 是否自动波特率检测
    SerialAnalyzerEnums::Mode mSerialMode;

protected:
    std::auto_ptr< AnalyzerSettingInterfaceChannel >    mInputChannelInterface;     // 通道接口
    std::auto_ptr< AnalyzerSettingInterfaceInteger >    mBitRateInterface;          // 整数输入框：波特率
    std::auto_ptr< AnalyzerSettingInterfaceNumberList > mBitsPerTransferInterface;  // 下拉列表：提供用户可以选择的数字选项列表
    std::auto_ptr< AnalyzerSettingInterfaceNumberList > mShiftOrderInterface;       // 下拉列表：MSB or LSB
    std::auto_ptr< AnalyzerSettingInterfaceNumberList > mStopBitsInterface;         // 下拉列表：停止位
    std::auto_ptr< AnalyzerSettingInterfaceNumberList > mParityInterface;           // 下拉列表：奇偶校验
    std::auto_ptr< AnalyzerSettingInterfaceBool >   mInvertedInterface;             // 单选框：是否反向（仅适用于RS232）
    std::auto_ptr< AnalyzerSettingInterfaceBool >   mUseAutobaudInterface;          // 单选框：是否自动波特率
    std::auto_ptr< AnalyzerSettingInterfaceNumberList > mSerialModeInterface;       // 下拉列表
};

#endif //SERIAL_ANALYZER_SETTINGS
