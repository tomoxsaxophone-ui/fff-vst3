#pragma once
#include "PluginProcessor.h"
#include <juce_gui_extra/juce_gui_extra.h>

class FffAudioProcessorEditor : public juce::AudioProcessorEditor, private juce::Timer {
public:
    explicit FffAudioProcessorEditor (FffAudioProcessor&);
    ~FffAudioProcessorEditor() override = default;

    void paint (juce::Graphics&) override;
    void resized() override {}
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;

private:
    void timerCallback() override { repaint(); }
    void drawFffLogo (juce::Graphics& g, juce::Rectangle<float> bounds);
    void drawRotaryKnob (juce::Graphics& g, juce::Rectangle<int> r, const juce::String& label, int val, bool pinkAccent);

    FffAudioProcessor& proc;
    int activeTab = 0; // 0:PATTERN, 1:GRID, 2:SOUND, 3:PRESET
    int quantMode = 0; // 0:NORM, 1:DOT, 2:TRIP
    int selectedBaseQuant = 3; // 0:1/2 .. 4:1/32
    bool freeQuantActive = false;
    int freeNoteIdx = 7;
    bool showAboutModal = false;
    bool showClearAllModal = false;
    int dragKnobType = 0; // 0:none, 1:globalSwing, 2:trackSwing
    int dragStartY = 0;
    int dragStartVal = 50;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FffAudioProcessorEditor)
};
