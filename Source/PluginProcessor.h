#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <array>
#include <vector>
#include <cmath>

struct NoteEvent {
    int tick = 0;
    int velIdx = 4; // 0:ppp .. 7:fff
    int dur = 96;
};

struct SampleLayer {
    float volume = 0.8f;
    float pan = 0.0f;
    int pitch = 0;
    float attack = 0.002f;
    float decay = 0.25f;
    bool mute = false;
    bool solo = false;
};

struct DrumTrack {
    juce::String name;
    int midiNote = 60;
    int steps = 16;
    int swing = 50; // 0..100 (50 = Even)
    bool mute = false;
    bool solo = false;
    int playMode = 0; // 0:Stack, 1:Velocity, 2:Round
    bool mono = false;
    bool choke = false;
    int rrCounter = 0;
    std::vector<NoteEvent> events;
    std::array<SampleLayer, 3> samples;
};

struct ActiveVoice {
    bool active = false;
    int trackIdx = 0;
    float phase = 0.0f;
    float ageSec = 0.0f;
    float maxDurSec = 0.35f;
    float gain = 0.5f;
    float pan = 0.0f;
    float pitchRatio = 1.0f;
};

class FffAudioProcessor : public juce::AudioProcessor {
public:
    FffAudioProcessor();
    ~FffAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "#fff"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return true; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 32; }
    int getCurrentProgram() override { return currentPreset; }
    void setCurrentProgram (int index) override { currentPreset = juce::jlimit (0, 31, index); }
    const juce::String getProgramName (int index) override { return "Preset #" + juce::String (index + 1); }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    void triggerTrack (int trackIdx, int velIdx);
    int getLongestTrackSteps() const;
    int getLongestTrackTicks() const;
    int applySwingToTick (double rawTick, int effSwing, int cycleLimit) const;

    std::array<DrumTrack, 8> tracks;
    int selectedTrack = 0;
    int cursorTick = 0;
    int bpm = 120;
    int globalSwing = 50; // 0..100 (50 = Even)
    bool isPlaying = false;
    bool polyStretchMode = false; // false: LOOP, true: STRETCH
    bool darkMode = false;
    int currentPreset = 0;
    double currentGlobalTick = 0.0;

private:
    double sr = 44100.0;
    std::array<ActiveVoice, 32> voices;
    juce::Random rng;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FffAudioProcessor)
};
