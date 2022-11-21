#ifndef SIMULATION_CHANNEL_DESCRIPTOR
#define SIMULATION_CHANNEL_DESCRIPTOR

#include "LogicPublicTypes.h"

//PIMPL implementation can be changed, non-virtual functions can be added at the end, no vars can be added/removed/reordered/changed.
// PIMPL实现可以更改，可以在末尾添加非虚函数，不能添加/删除/重新排序/更改变量。
struct SimulationChannelDescriptorData;
class LOGICAPI SimulationChannelDescriptor
{
public:
    /**
     * @brief 翻转通道，即把通道由低电平（BIT_LOW）变成高电平（BIT_HIGH）或者由高电平 （BIT_HIGH）变成低电平（BIT_LOW）。
     * 当前的采样数量的电平状态（BitState）会变成的新 的 BitState（BIT_LOW 或 BIT_HIGH），
     * 在我们再次翻转之前，其后的采样数量的 BitState 也 会是新的 BitState。
     * 
     */
    void Transition();

    /**
     * @brief 把传入的 bit_state 与当前的 BitState 比较，
     * 如果两者的 状态一样，则不用改变通道当前的 BitState，反之，则需要将当前的 BitState 翻转。
     * 
     * @param bit_state 
     */
    void TransitionIfNeeded(BitState bit_state);

    /**
     * @brief 利用该函数向前移动模拟波形，即增加采样数量，
     * 例如开始时为采样数量 0，在调用 Advance(10)函数 3 次后，采样数量就变为 30
     * 
     * @param num_samples_to_advance 
     */
    void Advance(U32 num_samples_to_advance);

    // 查询当前的 BitState，即当前的电平状态
    BitState GetCurrentBitState();

    // 获取当前的 SampleNumber，即采样数量
    U64 GetCurrentSampleNumber();

public:  //don't use
    SimulationChannelDescriptor();
    SimulationChannelDescriptor(const SimulationChannelDescriptor &other);
    ~SimulationChannelDescriptor();
    SimulationChannelDescriptor &operator=(const SimulationChannelDescriptor &other);

    void SetChannel(Channel &channel);
    void SetSampleRate(U32 sample_rate_hz);
    void SetInitialBitState(BitState intial_bit_state);

    Channel GetChannel();
    U32 GetSampleRate();
    BitState GetInitialBitState();
    void *GetData();

protected:
    struct SimulationChannelDescriptorData *mData;
};


struct SimulationChannelDescriptorGroupData;
class LOGICAPI SimulationChannelDescriptorGroup
{
public:
    SimulationChannelDescriptorGroup();
    ~SimulationChannelDescriptorGroup();

    SimulationChannelDescriptor *Add(Channel &channel, U32 sample_rate, BitState intial_bit_state);   //do not delete this pointer

    void AdvanceAll(U32 num_samples_to_advance);

public:
    SimulationChannelDescriptor *GetArray();
    U32 GetCount();

protected:
    struct SimulationChannelDescriptorGroupData *mData;
};

#endif //SIMULATION_CHANNEL_DESCRIPTOR