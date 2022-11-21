#ifndef ANALYZER_RESULTS
#define ANALYZER_RESULTS

#include "LogicPublicTypes.h"
#include <string>

#define DISPLAY_AS_ERROR_FLAG   (1 << 7)
#define DISPLAY_AS_WARNING_FLAG (1 << 6)

// 表示一段时间内协议所传达的信息
class LOGICAPI Frame
{
public:
    Frame();
    Frame(const Frame &frame);
    ~Frame();

    // Frame的起始和结束的采样点数
    // Frame间不能重叠，不能公用一个采样点
    // 例如：若某 Frame 的结束点位于一个时钟边沿处，而新 Frame 要从此处开始，那么这个新 Frame 的 mStartingSampleInclusive 需要加 1。
    S64 mStartingSampleInclusive;   // Frame起始的采样点数
    S64 mEndingSampleInclusive;     // Frame结束的采样点数

    // Frame 中可以有两个 64 位的数据，即 mDta1 和 mData2，
    // 例如：对于 SPI，其中 一个数据用于 MISO，另一个则用于 MISO。一般情况下只需要一个数据
    U64 mData1;
    U64 mData2;

    // 用于保存自定义的枚举值，表示 Frame 的类型
    // 例如：CAN 具有许多种 Frame，如头、数据、CRC 等，异步串行数据则只有一种类型，因此不会使用这个成员变量
    U8 mType;

    // 提供适用于 Frame 的自定义标志。但请注意，这个标志不能是普通枚举类型的，而应该是可用于组合在一起的单个 bit
    // 例如，在异步串行协议中，有一个帧错误标志和一个奇偶校验错误标志。
    // #define FRAMING_ERROR_FLAG (1 << 0) 
    // #define PARITY_ERROR_FLAG (1 << 1)
    U8 mFlags;

    bool HasFlag(U8 flag);
};

#define INVALID_RESULT_INDEX 0xFFFFFFFFFFFFFFFFull

struct AnalyzerResultsData;
class LOGICAPI AnalyzerResults
{
public:
    // 可用的 Marker 类型
    enum MarkerType { Dot, ErrorDot, Square, ErrorSquare, UpArrow, DownArrow, X, ErrorX, Start, Stop, One, Zero };
    AnalyzerResults(); //you must call the base class contructor in your constructor
    virtual ~AnalyzerResults();

    //override:

    /**
     * @brief 用于生成显示在屏幕上的信息
     * 
     * @param frame_index 提供 Frame 自身的索引值
     * @param channel 显示解析结果的通道一般不会多余 1 个（SPI 是一个特例），这样需要显示结果信息的通道就在 channel 参数中指定
     * @param display_base 指定了数据值显示的格式（二进制 Bin / 十进制 Dec / 十六进制 Hex / ASCII / ASCII & Hex），可以使用辅助函数来处理这种情况
     */
    virtual void GenerateBubbleText(U64 frame_index, Channel &channel, DisplayBase display_base) = 0;

    /**
     * @brief 要将解析结果输出到文件中
     * 
     * @param file 是解析结果保存文件的完整路径
     * @param display_base 解析结果的数据格式
     * @param export_type_user_id 与用户选择的导出文件关联的 id。
     * 可以在 AnalyzerSettings 派生类的构造函数中指定这些选项（应该至少有一个）。如果只有一个导出选项，可以忽略此参数。
     */
    virtual void GenerateExportFile(const char *file, DisplayBase display_base, U32 export_type_user_id) = 0;

    /**
     * @brief 为软件界面右下方的解析结果列表生成所需的字符串
     * 
     * GenerateFrameTabularText 函数与 GenerateBubbleText 函数几乎相同，
     * 只不过 GenerateFrameTabularText 函数应该只生成一个字符串结果。理想情况下，字符串应该简洁，在正常（非错误）情况下应该尽量短
     * 
     * @param frame_index Frame 自身的索引值
     * @param display_base 指定了数据值显示的格式
     */
    virtual void GenerateFrameTabularText(U64 frame_index, DisplayBase display_base) = 0;

    /**
     * @brief 
     * 
     * @param packet_id 
     * @param display_base 
     */
    virtual void GeneratePacketTabularText(U64 packet_id, DisplayBase display_base) = 0;

    /**
     * @brief 
     * 
     * @param transaction_id 
     * @param display_base 
     */
    virtual void GenerateTransactionTabularText(U64 transaction_id, DisplayBase display_base) = 0;

public:  //adding/setting data

    /**
     * @brief Marker 是波形上的表示协议某些特性的标记，
     * 例如：在异步串行协议中，我们在每个数 据 bit 上添加了一个小白点，表示该 bit 的值取自当前位置上波形的电平值。
     * 也可以用 Marker 表示协议出错以及时钟信号的上升和下降沿等
     * 
     * 与添加 Frame 相同，你需要依次添加 Marker
     * Marker 只用作图形记号，不能用于产生显示信息以及输出文件等
     * 
     * @param sample_number 
     * @param marker_type 
     * @param channel 
     */
    void AddMarker(U64 sample_number, MarkerType marker_type, Channel &channel);

    U64 AddFrame(const Frame &frame);

    // Packet 为 Frame 的组合
    // 在添加 Frame 时，它会被自动添加到当前 Packet 中，在将所有所需 Frame 加到 Packet 中后，可以调用 CommitPacketAndStartNewPacket，
    // 有些情况下，特别是出错时，你 可能需要启动一个新的 packet，此时需要调用 CancelPacketAndStartNewPacket
    // 请注意 CommitPacketAndStartNewPacket 会返回一个 Packet ID
    // 目前，Packet 仅在输出数据到 text/csv 时使用。
    // 在输出文件使用 Packet ID 时，应该使 用 GetPacketContainingFrameSequential 函 数 ， 
    // 避 免 每 次 都 重 新 检 索 所 需 的 Packet ， GetPacketContainingFrame 会从头搜索，因此效率更低
    U64 CommitPacketAndStartNewPacket();
    void CancelPacketAndStartNewPacket();
    void AddPacketToTransaction(U64 transaction_id, U64 packet_id);
    void AddChannelBubblesWillAppearOn(const Channel &channel);

    void CommitResults();

public:  //data access
    U64 GetNumFrames();
    U64 GetNumPackets();
    Frame GetFrame(U64 frame_id);

    U64 GetPacketContainingFrame(U64 frame_id);
    U64 GetPacketContainingFrameSequential(U64 frame_id);
    void GetFramesContainedInPacket(U64 packet_id, U64 *first_frame_id, U64 *last_frame_id);

    U32 GetTransactionContainingPacket(U64 packet_id);
    void GetPacketsContainedInTransaction(U64 transaction_id, U64 **packet_id_array, U64 *packet_id_count);

public:  //text results setting and access:
    // 在增加新的字符串结果时，要调用 ClearResultStrings
    void ClearResultStrings();

    // 要将字符串提供给调用者，可以使用 AddStringResult 函数，要确保函数返回时字符串仍然存在。
    // 要合并多个字符串，可以给 AddStringResult 输入多个字符串
    void AddResultString(const char *str1, const char *str2 = NULL, const char *str3 = NULL, const char *str4 = NULL, const char *str5 = NULL, const char *str6 = NULL);   //multiple strings will be concatinated
    void GetResultStrings(char const ***result_string_array, U32 *num_strings);

protected:  //use these when exporting data.
    bool UpdateExportProgressAndCheckForCancel(U64 completed_frames, U64 total_frames);

public:  //don't use
    bool DoBubblesAppearOnChannel(Channel &channel);
    bool DoMarkersAppearOnChannel(Channel &channel);
    bool GetFramesInRange(S64 starting_sample_inclusive, S64 ending_sample_inclusive, U64 *first_frame_index, U64 *last_frame_index);
    bool GetMarkersInRange(Channel &channel, S64 starting_sample_inclusive, S64 ending_sample_inclusive, U64 *first_marker_index, U64 *last_marker_index);
    void GetMarker(Channel &channel, U64 marker_index, MarkerType *marker_type, U64 *marker_sample);
    U64 GetNumMarkers(Channel &channel);

    void CancelExport();
    double GetProgress();
    void StartExportThread(const char *file, DisplayBase display_base, U32 export_type_user_id);

protected:
    struct AnalyzerResultsData *mData;

public:
    void ClearTabularText();
    const char *BuildSearchData(U64 FrameID, DisplayBase disp_base, int channel_list_index, char *result);

    std::string GetStringForDisplayBase(U64 frame_id, Channel channel, DisplayBase disp_base);
    void AddTabularText(const char *str1, const char *str2 = NULL, const char *str3 = NULL, const char *str4 = NULL, const char *str5 = NULL, const char *str6 = NULL);
    std::string GetTabularTextString();
};

#endif  //ANALYZER_RESULTS
