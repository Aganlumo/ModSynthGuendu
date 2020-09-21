/*
  ==============================================================================

    WaveTableGenerator.cpp
    Created: 11 Feb 2020 6:19:54pm
    Author:  Daniel Schwartz

	This WaveTableGenerator generates and allocates memory for 4 different kinds
	of wavetables.  A single instance of a WaveTableGenerator can be created and
	passed around to avoid making unnecessary copies of the wave table.

	This creates sine, square, saw, and tri tables.  It would be cool to add
	more functionality to this class to create custom tables using the fourierTable
	function.

	This table generator uses a constant tablesize.  The table size could be decreased
	as the octave is increased for better performance.  However, using a constant table
	size decreases the likelyhood of indexing outside the bounds of the table.

	The tables are bandlimited with one table per octave, but it might be better
	to use more divisions at some point.

	The number of harmonics per octave is calcualated by:

		tableSize / 2^(octave + 1) - 1

  ==============================================================================
*/

#include "WaveTableGenerator.h"

WaveTableGenerator::WaveTableGenerator() : FFT(fftOrder) // Needs to create FFT object with fftOrder
{
	formatManager.registerBasicFormats(); // For registering formats and need to be executed only once

	generateSineTable();
	generateSquareTable();
	generateSawTable();
	generateTriTable();
	generateCustomTable();
}

// this returns a vector of bandlimited tables for a given wavetype
const std::vector<AudioSampleBuffer>& WaveTableGenerator::getTables(WaveType type) const
{
	switch (type)
	{
	case WaveType::SINE:
		return m_sineTable;
	case WaveType::SQUARE:
		return m_squareTable;
	case WaveType::SAW:
		return m_sawTable;
	case WaveType::TRI:
		return m_triTable;
	case WaveType::CUSTOM:
		return m_customTable;
	default:
		return m_sineTable;
	}
}

// this returns a single table 
// the table it returns is table[0] which contains the full amount of harmonic detail
const AudioSampleBuffer& WaveTableGenerator::getTable(WaveType type) const
{
	switch (type)
	{
	case WaveType::SINE:
		return m_sineTable[0];
	case WaveType::SQUARE:
		return m_squareTable[0];
	case WaveType::SAW:
		return m_sawTable[0];
	case WaveType::TRI:
		return m_triTable[0];
	case WaveType::CUSTOM:
		return m_customTable[0];
	default:
		return m_sineTable[0];
	}
}

const int WaveTableGenerator::getTableSize() const
{
	return m_tableSize;
}

//==============================================================================

// Private member functions

void WaveTableGenerator::generateSineTable()
{
	for (int octave = 0; octave < m_octDivision; ++octave)
	{
		m_sineTable[octave].setSize(1, m_tableSize + 1);
		m_sineTable[octave].clear();

		Array<float> amps;

		fourierTable(m_sineTable[octave], 1, 0.25, amps);
	}
}

void WaveTableGenerator::generateSquareTable()
{
	for (int octave = 0; octave < m_octDivision; ++octave)
	{
		m_squareTable[octave].setSize(1, m_tableSize + 1);
		m_squareTable[octave].clear();

		int harm = static_cast<int>(m_tableSize / std::pow(2, octave + 1) - 1);
		Array<float> amps;

		for (int i = 0; i < harm; i += 2)
			amps.insert(i, 1.0f / (i + 1.0f));

		fourierTable(m_squareTable[octave], harm, 0.25, amps);

		normalizeTable(m_squareTable[octave]);
	}
}

void WaveTableGenerator::generateSawTable()
{
	for (int octave = 0; octave < m_octDivision; ++octave)
	{
		m_sawTable[octave].setSize(1, m_tableSize + 1);
		m_sawTable[octave].clear();

		int harm = static_cast<int>(m_tableSize / std::pow(2, octave + 1) - 1);
		Array<float> amps;

		for (int i = 0; i < harm; ++i)
			amps.insert(i, 1.0f / (i + 1.0f));

		fourierTable(m_sawTable[octave], harm, 0.25, amps);

		normalizeTable(m_sawTable[octave]);
	}
}

void WaveTableGenerator::generateTriTable()
{
	for (int octave = 0; octave < m_octDivision; ++octave)
	{
		m_triTable[octave].setSize(1, m_tableSize + 1);
		m_triTable[octave].clear();

		int harm = static_cast<int>(m_tableSize / std::pow(2, octave + 1) - 1);
		Array<float> amps;

		for (int i = 0; i < harm; i += 2)
			amps.insert(i, 1.0f / ((i + 1.0f) * (i + 1.0f)));

		fourierTable(m_triTable[octave], harm, 0, amps);

		normalizeTable(m_triTable[octave]);
	}
}


// Public function for creation of custom table
// Needs to be called whenever user wants to change the custom table oscillator
void WaveTableGenerator::generateCustomTable()
{
	FileChooser chooser("select a wave file to play...", {}, "*.wav");
	if (chooser.browseForFileToOpen()) {
		auto file = chooser.getResult();
		//File file = "D:\\Freelance\\ModSynth_Guendu\\Cymatics_Growl_Tables\\Growl Table 11.wav";
		auto* reader = formatManager.createReaderFor(file);

		if (reader != nullptr) {
			std::unique_ptr<AudioFormatReaderSource> newSource(new AudioFormatReaderSource(reader, true)); // Sets source
			// Buffer will release resources, so needs to be returned if user wants to keep it
			AudioBuffer<float> buffer(reader->numChannels, reader->lengthInSamples); // Creates buffer for reading .wav data
			reader->read(&buffer, 0, buffer.getNumSamples(), 0, true, true); // Copy .wav data to buffer 

			for (int octave = 0; octave < m_octDivision; ++octave)
			{
				m_customTable[octave].setSize(1, m_tableSize + 1); // equal to previous functions
				m_customTable[octave].clear();

				// calc harmonics for each and every octave in order to avoid nooice in higher octaves
				int harm = static_cast<int>(fftData.size()/std::pow(2, octave + 1) -1);
				// Creates amplitude juce array
				Array<float> amps;
				//std::array<type, size>

				// loop to obtain amplitudes and harmonics of buffer (contains .wav data)
				for (auto m = 0; m < buffer.getNumSamples(); ++m) {
					// m is sampleIndex, depends on number of samples in buffer
					// function performs FFT frequency analysis of sample
					// sample is accesed via buffer.getSample(channel, sampleIndex)
					pushNextSampleIntoFIFO(buffer.getSample(0, m)); 
				}

				for (int i = 0; i < harm; i += 2) {
					int current_amp = fftData[i]; // gets amplitude for corresponding harmonic
					amps.insert(i, current_amp); // insert amplitude to amplitude juce array
				}

				// create table as previous functions
				// create table for each and every octave, with given harmonics, phase = 0 and amplitud juce array
				fourierTable(m_customTable[octave], harm, 0, amps); 

				// normalize amplitudes per octave
				normalizeTable(m_customTable[octave]);
			}
		}
	}
}

void WaveTableGenerator::pushNextSampleIntoFIFO(float sample) {
	if (fifoIndex == fftSize) {
		std::fill(fftData.begin(), fftData.end(), 0.0f);
		std::copy(fifo.begin(), fifo.end(), fftData.begin());
		FFT.performFrequencyOnlyForwardTransform(fftData.data());
		fifoIndex = 0;
	}
	fifo[(size_t)fifoIndex++] = sample; 
}

void WaveTableGenerator::fourierTable(AudioSampleBuffer& buffer, int harm, float phase, const Array<float>& amps)
{
	// buffer is std::vector<AudioSampleBuffer>
	float* samples = buffer.getWritePointer(0); //???
	phase *= MathConstants<float>::twoPi; // moves phase to desired phase from arguments
	// phase is given in radians

	// loop alongside harmonics
	for (int i = 0; i < harm; ++i)
	{
		double currentAngle = phase; // 
		double angleDelta = (i + 1) * (MathConstants<double>::twoPi / m_tableSize); // calcs how much is necessary to move along buffer?
		float amp = amps.isEmpty() ? 1.0f : amps[i]; // asserts amplitude isn't empty
		// should amps is empty, amp = 1.0f, else amp = amps at harmonic

		for (int n = 0; n < m_tableSize; ++n)
		{
			double sample = std::cos(currentAngle); // angle calculation with Fou
			samples[n] += (float)(sample * amp); // giving amplitude to the fou sample
			currentAngle += angleDelta;
		}
	}

	samples[m_tableSize] = samples[0]; //???
}

void WaveTableGenerator::normalizeTable(AudioSampleBuffer& buffer)
{
	float maxAmp{ 0.0f };
	float amp;

	float* samples = buffer.getWritePointer(0);

	for (int i = 0; i < m_tableSize; ++i)
	{
		amp = samples[i];
		if (amp > maxAmp)
			maxAmp = amp;
	}

	maxAmp = 1.0f / maxAmp;

	for (int i = 0; i < m_tableSize; ++i)
		samples[i] *= maxAmp;

	samples[m_tableSize] = samples[0];
}