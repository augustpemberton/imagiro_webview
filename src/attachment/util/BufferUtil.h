//
// Created by August Pemberton on 17/06/2023.
//

#pragma once
#include <choc/audio/choc_SampleBuffers.h>
#include <choc/audio/choc_SampleBufferUtilities.h>
#include <juce_core/juce_core.h>

static choc::buffer::ChannelArrayView<float> getWriteViewForJuceBuffer(juce::AudioSampleBuffer& b) {
    return choc::buffer::createChannelArrayView(b.getArrayOfWritePointers(),
                                                b.getNumChannels(),
                                                b.getNumSamples());
}

static choc::buffer::ChannelArrayView<const float> getReadViewForJuceBuffer(juce::AudioSampleBuffer& b) {
    return choc::buffer::createChannelArrayView(b.getArrayOfReadPointers(),
                                                b.getNumChannels(),
                                                b.getNumSamples());
}

static auto downsample (
        choc::buffer::ChannelArrayView<const float> in, unsigned int outputSamples) {

    jassert(outputSamples < in.getNumFrames());

    auto factor = in.getNumFrames() / (float)outputSamples;

    return choc::buffer::createChannelArrayBuffer(
            in.getNumChannels(), outputSamples,
            [&](auto channel, auto frame) -> float {
                int inStart = frame * factor;
                int inEnd = (frame + 1) * factor;

                auto max = 0.f;
                for (auto s = inStart; s < inEnd; s++) {
                    auto sample = in.getSample(channel, s);
                    if (abs(sample) > abs(max)) max = sample;
                }

                return max;
            }
    );
}

struct VisualizerData {
    choc::buffer::ChannelArrayBuffer<float> min;
    choc::buffer::ChannelArrayBuffer<float> max;
    float mag;
    bool micro = false;
};

static VisualizerData getVisualizerDataMicro (
        choc::buffer::ChannelArrayView<const float> in) {

    VisualizerData v;
    v.min = choc::buffer::createChannelArrayBuffer(in.getNumChannels(), in.getNumFrames(),
                                                   [](){return 0.f;});
    v.max = choc::buffer::createChannelArrayBuffer(in.getNumChannels(), in.getNumFrames(),
                                                   [](){return 0.f;});

    float mag = 0.f;
    for (auto c=0; c<in.getNumChannels(); c++) {
        for (auto s = 0; s < in.getNumFrames(); s++) {
            auto value = in.getSample(c, s);
            if (abs(value) > abs(mag)) mag = value;

            v.min.getSample(c, s) = value;
            v.max.getSample(c, s) = value;
        }
    }

    v.mag = mag;
    v.micro = true;
    return v;
}

static VisualizerData getVisualizerDataMacro (
        choc::buffer::ChannelArrayView<const float> in, int outputSamples) {

    auto factor = in.getNumFrames() / (float)outputSamples;

    VisualizerData v;
    v.min = choc::buffer::createChannelArrayBuffer(in.getNumChannels(), outputSamples,
                                                   [](){return 0.f;});
    v.max = choc::buffer::createChannelArrayBuffer(in.getNumChannels(), outputSamples,
                                                   [](){return 0.f;});

    float mag = 0.f;
    for (auto c=0; c<in.getNumChannels(); c++) {
        for (auto s=0; s<outputSamples; s++) {
            int inStart = s * factor - 1;
            int inEnd = (s + 1) * factor + 1;

            inStart = std::max(0, inStart);
            inEnd = std::min((int)in.getNumFrames()-1, inEnd);

            auto max = -999.f;
            auto min = 999.f;

            // TODO this is pretty inaccurate
            auto interval = std::max(1, (inEnd - inStart) / 300);

            for (auto s2 = inStart; s2 < inEnd; s2+=interval) {
                auto sample = in.getSample(c, s2);
                if (sample > max) max = sample;
                if (sample < min) min = sample;
                if (abs(sample) > abs(mag)) mag = sample;
            }

            v.min.getSample(c, s) = min;
            v.max.getSample(c, s) = max;
        }
    }

    v.mag = mag;
    return v;
}

static auto getVisualizerData (
        choc::buffer::ChannelArrayView<const float> in, int outputSamples) {
    if (in.getNumFrames() >= outputSamples/2) {
        return getVisualizerDataMacro(in, outputSamples);
    } else {
        return getVisualizerDataMicro(in);
    }
}

static choc::value::Value getVisualizerDataForBuffer(juce::AudioSampleBuffer& buffer, int startSample,
                                                     int endSample, int numPoints) {

    auto view = choc::buffer::createChannelArrayView(buffer.getArrayOfReadPointers(),
                                                     (unsigned int) buffer.getNumChannels(),
                                                     (unsigned int) buffer.getNumSamples());

    choc::buffer::FrameRange range;
    range.start = (unsigned int) startSample;
    range.end = (unsigned int) std::min(endSample, buffer.getNumSamples());
    view = view.getFrameRange(range);

    auto visualizerData = getVisualizerData(view, numPoints);

    choc::buffer::InterleavingScratchBuffer<float> ibMin;
    auto interleavedMin = ibMin.interleave(visualizerData.min);
    choc::buffer::InterleavingScratchBuffer<float> ibMax;
    auto interleavedMax = ibMax.interleave(visualizerData.max);

    auto val = choc::value::createObject("VisualizerData");
    val.setMember("min", choc::buffer::createValueViewFromBuffer(interleavedMin));
    val.setMember("max", choc::buffer::createValueViewFromBuffer(interleavedMax));
    val.setMember("mag", visualizerData.mag);
    val.setMember("micro", visualizerData.micro);
    return val;
}
