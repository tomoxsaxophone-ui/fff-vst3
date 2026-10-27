#include "PluginProcessor.h"
#include "PluginEditor.h"

static const float kVelGains[8] = { 0.14f, 0.26f, 0.38f, 0.52f, 0.66f, 0.80f, 0.92f, 1.00f };
static const int   kVelMidis[8] = { 16, 32, 48, 64, 80, 96, 112, 127 };

FffAudioProcessor::FffAudioProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)) {
    const char* names[8] = { "KICK", "SNARE", "CLAP", "PERC", "HIHAT CL", "HIHAT OP", "TOM", "CYMBAL" };
    for (int i = 0; i < 8; ++i) {
        tracks[i].name = names[i];
        tracks[i].midiNote = 60 + i; // C3..G3
        tracks[i].steps = 16;
        tracks[i].swing = 50;
        tracks[i].choke = (i == 4 || i == 5);
        tracks[i].mono = (i == 4 || i == 5);
        tracks[i].events.clear(); // Empty sequence on startup
    }
}

void FffAudioProcessor::prepareToPlay (double sampleRate, int) {
    sr = sampleRate > 0.0 ? sampleRate : 44100.0;
}

int FffAudioProcessor::getLongestTrackSteps() const {
    int m = 1;
    for (const auto& t : tracks) m = std::max (m, t.steps);
    return m;
}

int FffAudioProcessor::getLongestTrackTicks() const {
    return getLongestTrackSteps() * 96;
}

int FffAudioProcessor::applySwingToTick (double rawTick, int effSwing, int cycleLimit) const {
    int limit = std::max (1, cycleLimit);
    if (std::abs (effSwing - 50) == 0)
        return ((int) std::round (rawTick)) % limit;

    double norm = (effSwing - 50.0) / 50.0;
    int beatIdx = (int) std::floor (rawTick / 384.0);
    double pos = rawTick - beatIdx * 384.0;
    double x1 = 96.0 + norm * 48.0;
    double x2 = 192.0 + norm * 16.0;
    double x3 = 288.0 + norm * 48.0;
    double warped = 0.0;
    if (pos <= 96.0) warped = (pos / 96.0) * x1;
    else if (pos <= 192.0) warped = x1 + ((pos - 96.0) / 96.0) * (x2 - x1);
    else if (pos <= 288.0) warped = x2 + ((pos - 192.0) / 96.0) * (x3 - x2);
    else warped = x3 + ((pos - 288.0) / 96.0) * (384.0 - x3);

    int res = (int) std::round (beatIdx * 384.0 + warped);
    return ((res % limit) + limit) % limit;
}

void FffAudioProcessor::triggerTrack (int trackIdx, int velIdx) {
    if (trackIdx < 0 || trackIdx >= 8) return;
    auto& tr = tracks[trackIdx];
    bool anyTrackSolo = false;
    for (const auto& t : tracks) if (t.solo) anyTrackSolo = true;
    if (tr.mute || (anyTrackSolo && !tr.solo)) return;

    if (tr.choke) {
        for (auto& v : voices)
            if (v.active && tracks[v.trackIdx].choke) v.active = false;
    } else if (tr.mono) {
        for (auto& v : voices)
            if (v.active && v.trackIdx == trackIdx) v.active = false;
    }

    float velGain = kVelGains[juce::jlimit (0, 7, velIdx)];
    for (auto& v : voices) {
        if (!v.active) {
            v.active = true;
            v.trackIdx = trackIdx;
            v.phase = 0.0f;
            v.ageSec = 0.0f;
            v.maxDurSec = juce::jlimit (0.04f, 1.2f, tr.samples[0].attack + tr.samples[0].decay);
            v.gain = juce::jlimit (0.05f, 1.2f, tr.samples[0].volume * velGain);
            v.pan = tr.samples[0].pan;
            v.pitchRatio = std::pow (2.0f, tr.samples[0].pitch / 12.0f);
            break;
        }
    }
}

void FffAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) {
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();

    if (auto* ph = getPlayHead()) {
        if (auto pos = ph->getPosition()) {
            if (auto hostBpm = pos->getBpm()) bpm = (int) std::round (*hostBpm);
        }
    }

    const int numSamples = buffer.getNumSamples();
    const double ticksPerSample = ((bpm / 60.0) * 384.0) / sr;
    const int maxCycleTicks = getLongestTrackTicks();

    for (int s = 0; s < numSamples; ++s) {
        if (isPlaying) {
            int prevTick = (int) std::floor (currentGlobalTick);
            currentGlobalTick += ticksPerSample;
            int nowTick = (int) std::floor (currentGlobalTick);

            if (nowTick > prevTick) {
                for (int tIdx = 0; tIdx < 8; ++tIdx) {
                    auto& tr = tracks[tIdx];
                    int trTicks = std::max (96, tr.steps * 96);
                    int effSwing = juce::jlimit (0, 100, 50 + (globalSwing - 50) + (tr.swing - 50));

                    if (!polyStretchMode) {
                        int localTick = nowTick % trTicks;
                        for (const auto& ev : tr.events) {
                            if (applySwingToTick (ev.tick, effSwing, trTicks) == localTick) {
                                triggerTrack (tIdx, ev.velIdx);
                                midiMessages.addEvent (juce::MidiMessage::noteOn (1, tr.midiNote, (juce::uint8) kVelMidis[ev.velIdx]), s);
                            }
                        }
                    } else {
                        int cyclePos = nowTick % maxCycleTicks;
                        for (const auto& ev : tr.events) {
                            double scaled = (ev.tick * (double) maxCycleTicks) / trTicks;
                            if (applySwingToTick (scaled, effSwing, maxCycleTicks) == cyclePos) {
                                triggerTrack (tIdx, ev.velIdx);
                                midiMessages.addEvent (juce::MidiMessage::noteOn (1, tr.midiNote, (juce::uint8) kVelMidis[ev.velIdx]), s);
                            }
                        }
                    }
                }
            }
        }

        float left = 0.0f, right = 0.0f;
        const float dt = 1.0f / (float) sr;
        for (auto& v : voices) {
            if (!v.active) continue;
            v.ageSec += dt;
            if (v.ageSec >= v.maxDurSec) { v.active = false; continue; }

            float env = std::exp (-v.ageSec * 12.0f) * v.gain;
            float sig = 0.0f;
            float t = v.ageSec;
            if (v.trackIdx == 0 || v.trackIdx == 6) {
                float freq = (v.trackIdx == 0 ? 52.0f : 95.0f) * v.pitchRatio + 130.0f * std::exp (-t * 32.0f);
                v.phase += freq * dt;
                sig = std::sin (v.phase * 6.2831853f) * env;
            } else if (v.trackIdx == 1 || v.trackIdx == 2) {
                float noise = (rng.nextFloat() * 2.0f - 1.0f);
                v.phase += 180.0f * v.pitchRatio * dt;
                sig = (std::sin (v.phase * 6.2831853f) * 0.45f + noise * 0.55f) * env;
            } else {
                float noise = (rng.nextFloat() * 2.0f - 1.0f);
                sig = noise * std::exp (-t * (v.trackIdx == 4 ? 35.0f : 8.0f)) * v.gain;
            }
            left  += sig * std::min (1.0f, 1.0f - v.pan);
            right += sig * std::min (1.0f, 1.0f + v.pan);
        }

        if (buffer.getNumChannels() > 0) buffer.setSample (0, s, juce::jlimit (-1.0f, 1.0f, left));
        if (buffer.getNumChannels() > 1) buffer.setSample (1, s, juce::jlimit (-1.0f, 1.0f, right));
    }
}

void FffAudioProcessor::getStateInformation (juce::MemoryBlock& destData) {
    juce::ValueTree vt ("FFF_STATE");
    vt.setProperty ("bpm", bpm, nullptr);
    vt.setProperty ("globalSwing", globalSwing, nullptr);
    vt.setProperty ("polyStretch", polyStretchMode, nullptr);
    std::unique_ptr<juce::XmlElement> xml (vt.createXml());
    copyXmlToBinary (*xml, destData);
}

void FffAudioProcessor::setStateInformation (const void* data, int sizeInBytes) {
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));
    if (xml && xml->hasTagName ("FFF_STATE")) {
        auto vt = juce::ValueTree::fromXml (*xml);
        bpm = vt.getProperty ("bpm", 120);
        globalSwing = vt.getProperty ("globalSwing", 50);
        polyStretchMode = vt.getProperty ("polyStretch", false);
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new FffAudioProcessor();
}
