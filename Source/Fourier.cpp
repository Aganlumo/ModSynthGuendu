/*
  ==============================================================================

    Fourier.cpp
    Created: 1 Sep 2020 11:18:06am
    Author:  Argo

  ==============================================================================
*/

#include "Fourier.h"

Fourier::Fourier() : forwardFFT(fftOrder), spectogramImage(juce::Image::RGB, 512, 512, true) {
    setOpaque(true);
    setAudioChannels(2, 0);
    startTimerHz(60);
    setSize(700, 500);
}

Fourier::~Fourier() {
    shutdownAudio();
}

void Fourier::prepareToPlay(int, double) {}

void Fourier::getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill) {
    if (bufferToFill.buffer->getNumChannels() > 0) {
        auto* channelData = bufferToFill.buffer->getReadPointer(0, bufferToFill.startSample);

        for (auto i = 0; i < bufferToFill.numSamples; ++i)
            pushNextSampleIntoFifo(channelData[i]);
    }
}

void Fourier::paint(juce::Graphics& g) {
    g.fillAll(juce::Colours::black);

    g.setOpacity(1.0f);
    g.drawImage(spectogramImage, getLocalBounds().toFloat());
}

void Fourier::timerCallback() {
    if (nextFFTBlockReady) {
        drawNextLineOfSpectogram();
        nextFFTBlockReady = false;
        repaint();
    }
}

void Fourier::pushNextSampleIntoFifo(float sample) noexcept {
    if (fifoIndex == fftSize) { // fifo must contain enough data 
        if (!nextFFTBlockReady) {
            std::fill(fftData.begin(), fftData.end(), 0.0f);
            std::copy(fifo.begin(), fifo.end(), fftData.begin());
            nextFFTBlockReady = true;
        }

        fifoIndex = 0;
    }

    fifo[(size_t)fifoIndex++] = sample;
}

void Fourier::drawNextLineOfSpectogram() {
    auto rightHandEdge = spectogramImage.getWidth() - 1;
    auto imageHeight = spectogramImage.getHeight();

    spectogramImage.moveImageSection(0, 0, 1, 0, rightHandEdge, imageHeight);

    forwardFFT.performFrequencyOnlyForwardTransform(fftData.data());

    auto maxlevel = juce::FloatVectorOperations::findMinAndMax(fftData.data(), fftSize / 2);

    for (auto y = 1; y < imageHeight; ++y) {
        auto skewedProportionY = 1.0f - std::exp(std::log((float)y / (float)imageHeight) * 0.2f);
        auto fftDataIndex = (size_t)juce::jlimit(0, fftSize / 2, (int)(skewedProportionY * fftSize / 2));
        auto level = juce::jmap(fftData[fftDataIndex], 0.0f, juce::jmax(maxlevel.getEnd(), 1e-5f), 0.0f, 1.0f); // mapped amplitude from 0 to 1

        spectogramImage.setPixelAt(rightHandEdge, y, juce::Colour::fromHSV(level, 1.0f, level, 1.0f)); // level is amplitude
    }
}