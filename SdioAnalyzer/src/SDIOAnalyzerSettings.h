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


    Channel mClockChannel;
    Channel mCmdChannel;
    Channel mDAT0Channel;
    Channel mDAT1Channel;
    Channel mDAT2Channel;
    Channel mDAT3Channel;
    bool mDoNotGenerateDatFrames;
    bool mDoNotGenerateStartFrames;
    enum SampleEdge mSampleRead;

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
