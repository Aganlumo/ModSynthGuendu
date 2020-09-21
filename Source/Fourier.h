/*
  ==============================================================================

    Fourier.h
    Created: 1 Sep 2020 11:17:41am
    Author:  Argo

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

class Fourier : public juce::AudioAppComponent, private juce::Timer {
public:
    Fourier();

    ~Fourier() override;

    void prepareToPlay(int, double) override;
    void releaseResources() override {};

    void paint(juce::Graphics& g) override;

    void timerCallback() override;

    void getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill) override;

    void pushNextSampleIntoFifo(float sample) noexcept;

    void drawNextLineOfSpectogram();

    static constexpr auto fftOrder = 10; // Order designates the size of the fft window. 2^order
    static constexpr auto fftSize = 1 << fftOrder; // Bit shift operator to produce binary number

private:
    juce::dsp::FFT forwardFFT; // object to perform forward fft
    juce::Image spectogramImage;

    std::array<float, fftSize> fifo; // fifo  float array of fftSize cointains incoming audio in samples
    std::array<float, fftSize * 2> fftData; // data array with results of Fourier analysis 
    int fifoIndex = 0; // indez leeps count of amount of samples in fifo
    bool nextFFTBlockReady = false; // temporary boolean

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Fourier)
};