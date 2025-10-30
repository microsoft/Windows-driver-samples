#pragma once

class SineWave
{
    struct wave
    {
        float amp{}, freq{}, phase{}, dc{};
    };
public:
    SineWave() {}

    template<typename _ty1, typename _ty2>
    SineWave(_ty1 numChannels, _ty2 samplingRate) : _channel(unsigned(numChannels)), _sr(float(samplingRate)) {}

    template<typename _ty1, typename _ty2>
    inline void Initialize(_ty1 numChannels, _ty2 samplingRate)
    {
        _channel.resize(unsigned(numChannels));
        _sr = unsigned(samplingRate);
    }

    template<typename _ty1>
    inline void SetChannel(_ty1 channel, float freq, float amplitude = 0.f, float dc = 0.f, float phase = 0.f)
    {
        _channel[unsigned(channel)].amp = amplitude;
        _channel[unsigned(channel)].freq = freq;
        _channel[unsigned(channel)].phase = phase;
        _channel[unsigned(channel)].dc = dc;
    }


    template<typename _ty1>
    void FillFrames(void* buffer, _ty1 numFrames)
    {
        auto fb = reinterpret_cast<float*>(buffer);
        for (unsigned i = 0; i < unsigned(numFrames); ++i)
        {
            for (unsigned ch = 0; ch < _channel.size(); ++ch)
            {
                fb[i * _channel.size() + ch] = Calculate(ch);
            }
        }
    }

protected:
    float Calculate(unsigned ch)
    {
        auto& w = _channel[ch];
        float sample = w.amp * sinf(w.phase) + w.dc;

        w.phase += 2 * float(M_PI) * w.freq / _sr;
        w.phase = fmodf(w.phase, 2 * float(M_PI));

        return sample;
    }

private:
    std::vector<wave> _channel;
    float _sr{};
};