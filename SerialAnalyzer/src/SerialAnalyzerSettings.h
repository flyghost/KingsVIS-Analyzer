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

    Channel mInputChannel;                      // ͨ�����������߼������ǵ��ĸ�ͨ������
    U32 mBitRate;                               // ������
    U32 mBitsPerTransfer;                       // ʹ���źŵĸߵ͵�ƽ
    AnalyzerEnums::ShiftOrder mShiftOrder;      // MSB ���� LSB
    double mStopBits;                           // ֹͣλ
    AnalyzerEnums::Parity mParity;              // ��żУ��
    bool mInverted;                             // ���򣬵ߵ�
    bool mUseAutobaud;                          // �Ƿ��Զ������ʼ��
    SerialAnalyzerEnums::Mode mSerialMode;

protected:
    std::auto_ptr< AnalyzerSettingInterfaceChannel >    mInputChannelInterface;     // ͨ���ӿ�
    std::auto_ptr< AnalyzerSettingInterfaceInteger >    mBitRateInterface;          // ��������򣺲�����
    std::auto_ptr< AnalyzerSettingInterfaceNumberList > mBitsPerTransferInterface;  // �����б��ṩ�û�����ѡ�������ѡ���б�
    std::auto_ptr< AnalyzerSettingInterfaceNumberList > mShiftOrderInterface;       // �����б�MSB or LSB
    std::auto_ptr< AnalyzerSettingInterfaceNumberList > mStopBitsInterface;         // �����б�ֹͣλ
    std::auto_ptr< AnalyzerSettingInterfaceNumberList > mParityInterface;           // �����б���żУ��
    std::auto_ptr< AnalyzerSettingInterfaceBool >   mInvertedInterface;             // ��ѡ���Ƿ��򣨽�������RS232��
    std::auto_ptr< AnalyzerSettingInterfaceBool >   mUseAutobaudInterface;          // ��ѡ���Ƿ��Զ�������
    std::auto_ptr< AnalyzerSettingInterfaceNumberList > mSerialModeInterface;       // �����б�
};

#endif //SERIAL_ANALYZER_SETTINGS
