#ifndef ANALYZER_SETTING_INTERFACE
#define ANALYZER_SETTING_INTERFACE

#include <vector>
#include <string>
#include "LogicPublicTypes.h"

enum AnalyzerInterfaceTypeId {
    INTERFACE_BASE,
    INTERFACE_CHANNEL,
    INTERFACE_NUMBER_LIST,
    INTERFACE_INTEGER,
    INTERFACE_TEXT,
    INTERFACE_BOOL
};

//note: we don't need to have virtual functions to prevent code copies becuase they is only one copy of the code now -- in the DLL.
//we need one virtual function, to return the class type; we're not using rtti

//using PIMPL; struct can change, non-virtual functions can be added to the end;
//Can not add/remove virtual functions

struct AnalyzerSettingInterfaceData;
class LOGICAPI AnalyzerSettingInterface
{
public:
    AnalyzerSettingInterface();
    virtual ~AnalyzerSettingInterface();

    static void operator delete (void *p);
    static void *operator new (size_t size);
    virtual AnalyzerInterfaceTypeId GetType();

    const char *GetToolTip();
    const char *GetTitle();
    bool IsDisabled();

    // 设置接口名和提示信息
    void SetTitleAndTooltip(const char *title, const char *tooltip);

    //todo:  position, group, spacers, visual attributes

protected:
    struct AnalyzerSettingInterfaceData *mData;

};

struct AnalyzerSettingInterfaceChannelData;
// 数据通道接口
class LOGICAPI AnalyzerSettingInterfaceChannel : public AnalyzerSettingInterface
{
public:
    AnalyzerSettingInterfaceChannel();
    virtual ~AnalyzerSettingInterfaceChannel();
    virtual AnalyzerInterfaceTypeId GetType();

    // 获取数据通道
    Channel GetChannel();

    // 设置数据通道
    void SetChannel(const Channel &channel);
    bool GetSelectionOfNoneIsAllowed();

    // 设置通道是否可以设置为None，这就表示“Enable”可以不与逻辑分析仪通道相连接
    void SetSelectionOfNoneIsAllowed(bool is_allowed);

protected:
    struct AnalyzerSettingInterfaceChannelData *mChannelData;
};

struct AnalyzerSettingInterfaceNumberListData;
// 下拉列表
class LOGICAPI AnalyzerSettingInterfaceNumberList : public AnalyzerSettingInterface
{
public:
    AnalyzerSettingInterfaceNumberList();
    virtual ~AnalyzerSettingInterfaceNumberList();
    virtual AnalyzerInterfaceTypeId GetType();

    double GetNumber();
    void SetNumber(double number);

    U32 GetListboxNumbersCount();
    double GetListboxNumber(U32 index);

    U32 GetListboxStringsCount();
    const char *GetListboxString(U32 index);

    U32 GetListboxTooltipsCount();
    const char *GetListboxTooltip(U32 index);

    void AddNumber(double number, const char *str, const char *tooltip);
    void ClearNumbers();

protected:
    struct AnalyzerSettingInterfaceNumberListData *mNumberListData;

};

struct AnalyzerSettingInterfaceIntegerData;
// 整数输入框
class LOGICAPI AnalyzerSettingInterfaceInteger : public AnalyzerSettingInterface
{
public:
    AnalyzerSettingInterfaceInteger();
    virtual ~AnalyzerSettingInterfaceInteger();
    virtual AnalyzerInterfaceTypeId GetType();

    int GetInteger();
    void SetInteger(int integer);

    int GetMax();
    int GetMin();

    void SetMax(int max);
    void SetMin(int min);

protected:
    struct AnalyzerSettingInterfaceIntegerData *mIntegerData;

};

struct AnalyzerSettingInterfaceTextData;
// 文本框
class LOGICAPI AnalyzerSettingInterfaceText : public AnalyzerSettingInterface
{
public:
    AnalyzerSettingInterfaceText();
    virtual ~AnalyzerSettingInterfaceText();

    AnalyzerInterfaceTypeId GetType();
    const char *GetText();
    void SetText(const char *text);

    enum TextType { NormalText, FilePath, FolderPath };
    TextType GetTextType();
    void SetTextType(TextType text_type);


protected:
    struct AnalyzerSettingInterfaceTextData *mTextData;

};

struct AnalyzerSettingInterfaceBoolData;
// 单选框
class LOGICAPI AnalyzerSettingInterfaceBool : public AnalyzerSettingInterface
{
public:
    AnalyzerSettingInterfaceBool();
    virtual ~AnalyzerSettingInterfaceBool();
    virtual AnalyzerInterfaceTypeId GetType();

    bool GetValue();

    // 设置单选框是否选中
    void SetValue(bool value);
    const char *GetCheckBoxText();

    // 设置单选框文本
    void SetCheckBoxText(const char *text);

protected:
    struct AnalyzerSettingInterfaceBoolData *mBoolData;
};

#endif //ANALYZER_SETTING_INTERFACE
