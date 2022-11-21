#ifndef ANALYZERCHANNELDATA
#define ANALYZERCHANNELDATA

#include "LogicPublicTypes.h"

struct AnalyzerChannelDataData;
class ChannelData;

/**
 * @brief 通过 AnalyzerChannelData 类，我们可以访问输入的数据。
 * 我们只能按照先后顺序访问数据，而不能进行随机访问。该类提供的函数，可以使的解析更加方便，在解析的时候要充分使用此类中的函数
 * 
 */
class LOGICAPI AnalyzerChannelData
{
public:
    AnalyzerChannelData(ChannelData *channel_data);
    ~AnalyzerChannelData();

    //State
    // 若不确定当前数据的位置，以及输入是高是低，我们可以调用一下两个接口
    U64 GetSampleNumber();
    BitState GetBitState();

    //Basic:
    // 要遍历数据，可以使用三种方式
    // 该函数可以向前移动特定数量的采样点数，本函数将返回移动期间电平翻转的次数（由高变低或由低变高）。
    U32 Advance(U32 num_samples); //move forward the specified number of samples. Returns the number of times the bit changed state during the move.
    // 若要向前移动到特定的绝对位置，我们可以使用该函数，它也会返回移动期间电平的变化次数
    U32 AdvanceToAbsPosition(U64 sample_number); //move forward to the specified sample number. Returns the number of times the bit changed state during the move.
    // 该函数向前移动到状态改变处，即下个信号边沿所在处，调用该函数之后，你可以调用 GetSampleNumber 函数来获取该处的采样点数
    void AdvanceToNextEdge(); //move forward until the bit state changes from what it is now.

    //Fancier
    // 高级遍历（向前无移动）：在开发协议解析器时，特定任务可能会进行更加复杂的遍历，下面是一些处理方法
    // 该函数不会移动数据中的位置，函数返回下个边沿的采样点数
    U64 GetSampleOfNextEdge(); //without moving, get the sample of the next transition.
    // 该函数不会移动数据中的位置，函数返回移动一定数量的采样数据时是否会引起位状态的变化（由高变低或由低变高）
    bool WouldAdvancingCauseTransition(U32 num_samples); //if we advanced, would we encounter any transitions?
    // 该函数不会移动数据中的位置，函数返回移动到特定位置时是否会引起位状态的变化（由高变低或由低变高）。
    bool WouldAdvancingToAbsPositionCauseTransition(U64 sample_number); //if we advanced, would we encounter any transitions?

    //minimum pulse tracking.  The serial analyzer uses this for auto-baud
    void TrackMinimumPulseWidth(); //normally this is not enabled.
    U64 GetMinimumPulseWidthSoFar();

    //Fancier, part II
    bool DoMoreTransitionsExistInCurrentData(); //use this when you have a situation where you have multiple lines, and you need to handle the case where one or the other of them may never change again, and you don't know which.

protected:
    AnalyzerChannelDataData *mData;
};

#endif //ANALYZERCHANNELDATA