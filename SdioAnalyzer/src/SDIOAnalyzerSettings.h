#ifndef SDIO_ANALYZER_SETTINGS
#define SDIO_ANALYZER_SETTINGS

#include <AnalyzerSettings.h>
#include <AnalyzerTypes.h>

enum SampleEdge { FALLING_EDGE, RISING_EDGE };

class SDIOAnalyzerSettings : public AnalyzerSettings
{
public:
    SDIOAnalyzerSettings();
    virtual ~SDIOAnalyzerSettings();

    virtual bool SetSettingsFromInterfaces();
    void UpdateInterfacesFromSettings();
    virtual void LoadSettings(const char *settings);
    virtual const char *SaveSettings();


    Channel mClockChannel;      // 时钟通道
    Channel mCmdChannel;        // 命令通道
    Channel mDAT0Channel;       // DATA0
    Channel mDAT1Channel;       // DATA1
    Channel mDAT2Channel;       // DATA2
    Channel mDAT3Channel;       // DATA3
    bool mDoNotGenerateDatFrames;       // 不解析 DATA0~DATA3 数据
    bool mDoNotGenerateStartFrames;     // 不解析 Start 和 End 字符串
    enum SampleEdge mSampleRead;        // 采样边沿：上升沿 or 下降沿

protected:
    std::auto_ptr< AnalyzerSettingInterfaceChannel >    mClockChannelInterface;
    std::auto_ptr< AnalyzerSettingInterfaceChannel >    mCmdChannelInterface;
    std::auto_ptr< AnalyzerSettingInterfaceChannel >    mDAT0ChannelInterface;
    std::auto_ptr< AnalyzerSettingInterfaceChannel >    mDAT1ChannelInterface;
    std::auto_ptr< AnalyzerSettingInterfaceChannel >    mDAT2ChannelInterface;
    std::auto_ptr< AnalyzerSettingInterfaceChannel >    mDAT3ChannelInterface;
    std::auto_ptr< AnalyzerSettingInterfaceNumberList > mSampleReadInterface;
    std::auto_ptr<AnalyzerSettingInterfaceBool> mDoNotGenerateDatFramesInterface;
    std::auto_ptr<AnalyzerSettingInterfaceBool> mDoNotGenerateStartFramesInterface;

};

#endif //SDIO_ANALYZER_SETTINGS
