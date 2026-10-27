#include "PluginProcessor.h"
#include "PluginEditor.h"

static const int kBaseQuantTicks[5] = { 768, 384, 192, 96, 48 };
static const char* kBaseQuantNames[5] = { "1/2", "1/4", "1/8", "1/16", "1/32" };
static const char* kVelNames[8] = { "ppp", "pp", "p", "mp", "mf", "f", "ff", "fff" };
static const juce::uint32 kVelColors[8] = {
    0xffe8eef5, 0xffd8ecff, 0xffbce0fd, 0xff99d1fc,
    0xffffdce8, 0xffffc2d8, 0xffffa6c6, 0xffff85b1
};

FffAudioProcessorEditor::FffAudioProcessorEditor (FffAudioProcessor& p)
    : AudioProcessorEditor (&p), proc (p) {
    setSize (1040, 680);
    startTimerHz (25);
}

void FffAudioProcessorEditor::drawFffLogo (juce::Graphics& g, juce::Rectangle<float> b) {
    g.setColour (juce::Colours::white);
    g.fillRoundedRectangle (b, 8.0f);
    g.setColour (juce::Colour (0xffff9ebb));
    g.drawRoundedRectangle (b, 8.0f, 2.0f);

    juce::Path p;
    auto sx = [&](float x) { return b.getX() + (x / 1000.0f) * b.getWidth(); };
    auto sy = [&](float y) { return b.getY() + (y / 360.0f) * b.getHeight(); };

    p.startNewSubPath (sx (182), sy (55)); p.lineTo (sx (98), sy (292));
    p.startNewSubPath (sx (250), sy (38)); p.lineTo (sx (165), sy (325));
    p.startNewSubPath (sx (25), sy (110)); p.lineTo (sx (320), sy (75));
    p.startNewSubPath (sx (26), sy (205)); p.lineTo (sx (270), sy (180));

    p.startNewSubPath (sx (530), sy (50)); p.quadraticTo (sx (440), sy (55), sx (410), sy (290));
    p.startNewSubPath (sx (345), sy (172)); p.lineTo (sx (500), sy (172));

    p.startNewSubPath (sx (745), sy (75)); p.quadraticTo (sx (590), sy (90), sx (575), sy (325));
    p.startNewSubPath (sx (528), sy (225)); p.lineTo (sx (648), sy (218));

    p.startNewSubPath (sx (970), sy (30)); p.quadraticTo (sx (860), sy (40), sx (865), sy (275));
    p.startNewSubPath (sx (808), sy (190)); p.lineTo (sx (975), sy (145));

    g.setColour (juce::Colour (0xff3155d4));
    g.strokePath (p, juce::PathStrokeType (2.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    g.setColour (juce::Colour (0xffef3b4c));
    const float dots[6][2] = { {381,111}, {506,110}, {545,184}, {622,179}, {813,141}, {915,126} };
    float r = std::max (2.0f, b.getHeight() * 0.045f);
    for (int i = 0; i < 6; ++i)
        g.fillEllipse (sx (dots[i][0]) - r, sy (dots[i][1]) - r, r * 2.0f, r * 2.0f);
}

void FffAudioProcessorEditor::drawRotaryKnob (juce::Graphics& g, juce::Rectangle<int> r, const juce::String& label, int val, bool pinkAccent) {
    g.setColour (proc.darkMode ? juce::Colour (0xff2b3140) : juce::Colours::white);
    g.fillRoundedRectangle (r.toFloat(), 8.0f);
    g.setColour (juce::Colour (0xffcbd5e1));
    g.drawRoundedRectangle (r.toFloat(), 8.0f, 1.2f);

    float cx = (float) r.getX() + 18.0f;
    float cy = (float) r.getCentreY();
    float rad = 11.0f;
    g.setColour (juce::Colour (pinkAccent ? 0xffec6b94 : 0xff4da3e8));
    g.drawEllipse (cx - rad, cy - rad, rad * 2.0f, rad * 2.0f, 2.0f);

    float angle = ((val - 50.0f) / 50.0f) * 2.35f;
    float nx = cx + std::sin (angle) * (rad - 2.0f);
    float ny = cy - std::cos (angle) * (rad - 2.0f);
    g.drawLine (cx, cy, nx, ny, 2.4f);

    g.setColour (proc.darkMode ? juce::Colours::white : juce::Colour (0xff262b36));
    g.setFont (10.0f);
    g.drawText (label, r.getX() + 34, r.getY() + 4, r.getWidth() - 36, 12, juce::Justification::left);
    g.setFont (12.0f);
    g.drawText (juce::String (val) + "%", r.getX() + 34, r.getY() + 17, r.getWidth() - 36, 14, juce::Justification::left);
}

void FffAudioProcessorEditor::paint (juce::Graphics& g) {
    auto bgPage = proc.darkMode ? juce::Colour (0xff161920) : juce::Colour (0xfff1f4f9);
    auto bgBoard = proc.darkMode ? juce::Colour (0xff212631) : juce::Colour (0xffe2e8f0);
    auto bgPanel = proc.darkMode ? juce::Colour (0xff2b3140) : juce::Colours::white;
    auto textMain = proc.darkMode ? juce::Colours::white : juce::Colour (0xff262b36);

    g.fillAll (bgPage);

    // Top Header Bar
    g.setColour (bgBoard);
    g.fillRoundedRectangle (12.0f, 10.0f, (float) getWidth() - 24.0f, 48.0f, 10.0f);

    drawFffLogo (g, juce::Rectangle<float> (20.0f, 16.0f, 84.0f, 36.0f));

    auto drawBtn = [&](juce::Rectangle<int> rect, const juce::String& text, juce::Colour bg, juce::Colour fg) {
        g.setColour (bg);
        g.fillRoundedRectangle (rect.toFloat(), 7.0f);
        g.setColour (fg);
        g.setFont (12.0f);
        g.drawText (text, rect, juce::Justification::centred);
    };

    drawBtn ({ 112, 16, 76, 36 }, proc.isPlaying ? "STOP" : "PLAY",
             juce::Colour (proc.isPlaying ? 0xffff9ebb : 0xff8ecae6), juce::Colour (0xff1e232e));
    drawBtn ({ 194, 16, 66, 36 }, "CLEAR", juce::Colour (0xffffe4ed), juce::Colour (0xff262b36));
    drawBtn ({ 266, 16, 82, 36 }, "BPM " + juce::String (proc.bpm), bgPanel, textMain);

    drawRotaryKnob (g, { 354, 16, 92, 36 }, "SWING", proc.globalSwing, true);

    drawBtn ({ 452, 16, 106, 36 }, proc.polyStretchMode ? "POLY: STRETCH" : "POLY: LOOP",
             juce::Colour (proc.polyStretchMode ? 0xffff9ebb : 0xffdff1ff), juce::Colour (0xff1e232e));

    const char* tabNames[4] = { "PATTERN", "GRID", "SOUND", "PRESET" };
    for (int i = 0; i < 4; ++i) {
        drawBtn ({ 580 + i * 88, 16, 82, 36 }, tabNames[i],
                 activeTab == i ? juce::Colour (0xffff9ebb) : bgPanel,
                 activeTab == i ? juce::Colour (0xff1e232e) : textMain);
    }
    drawBtn ({ 936, 16, 82, 36 }, proc.darkMode ? "LIGHT" : "DARK", bgPanel, textMain);

    auto& tr = proc.tracks[proc.selectedTrack];

    // Counter & Track Info Row
    g.setColour (bgBoard);
    g.fillRoundedRectangle (12.0f, 68.0f, (float) getWidth() - 24.0f, 54.0f, 10.0f);

    int dispTick = proc.isPlaying ? ((int) proc.currentGlobalTick % std::max (96, tr.steps * 96)) : proc.cursorTick;
    int bar = (dispTick / 384) / 4 + 1;
    int beat = (dispTick / 384) % 4 + 1;
    int clk = std::min (127, (int) std::round (((dispTick % 384) * 128.0) / 384.0));
    juce::String counterStr = juce::String::formatted ("%02d:%02d:%03d", bar, beat, clk);

    g.setColour (textMain);
    g.setFont (22.0f);
    g.drawText (counterStr, 26, 74, 210, 42, juce::Justification::centredLeft);

    g.setFont (14.0f);
    g.drawText ("TR" + juce::String (proc.selectedTrack + 1) + ": " + tr.name + " (C3+" + juce::String (proc.selectedTrack) + ")",
                250, 76, 220, 38, juce::Justification::centredLeft);

    drawBtn ({ 480, 77, 32, 36 }, "-", bgPanel, textMain);
    drawBtn ({ 516, 77, 90, 36 }, "STEPS: " + juce::String (tr.steps), bgPanel, textMain);
    drawBtn ({ 610, 77, 32, 36 }, "+", bgPanel, textMain);

    drawRotaryKnob (g, { 654, 77, 102, 36 }, "TR SWING", tr.swing, false);

    // 1-Lane Mini Step Strip
    g.setColour (bgBoard);
    g.fillRoundedRectangle (12.0f, 130.0f, (float) getWidth() - 24.0f, 44.0f, 8.0f);
    int stepW = (getWidth() - 48) / std::max (1, tr.steps);
    int curStep = (dispTick / 96) % std::max (1, tr.steps);
    for (int s = 0; s < tr.steps; ++s) {
        juce::Rectangle<int> srRect (24 + s * stepW, 138, std::max (8, stepW - 3), 28);
        int sTick = s * 96;
        int foundVel = -1;
        for (const auto& ev : tr.events) if (ev.tick >= sTick && ev.tick < sTick + 96) { foundVel = ev.velIdx; break; }
        g.setColour (foundVel >= 0 ? juce::Colour (kVelColors[foundVel]) : bgPanel);
        g.fillRoundedRectangle (srRect.toFloat(), 5.0f);
        if (s == curStep) {
            g.setColour (juce::Colour (0xff3b97e3));
            g.drawRoundedRectangle (srRect.toFloat(), 5.0f, 2.2f);
        }
        g.setColour (juce::Colour (0xff262b36));
        g.setFont (10.0f);
        g.drawText (foundVel >= 0 ? kVelNames[foundVel] : juce::String (s + 1), srRect, juce::Justification::centred);
    }

    // Left Track Sidebar (Tracks 1-8)
    for (int t = 0; t < 8; ++t) {
        juce::Rectangle<int> trRect (16, 186 + t * 56, 155, 50);
        drawBtn (trRect, juce::String (t + 1) + ". " + proc.tracks[t].name + " (" + juce::String (proc.tracks[t].steps) + "st)",
                 t == proc.selectedTrack ? juce::Colour (0xffff9ebb) : bgPanel,
                 t == proc.selectedTrack ? juce::Colour (0xff1e232e) : textMain);
    }

    // Main Right Area (Quantize + Accent Velocity Grid)
    g.setColour (bgBoard);
    g.fillRoundedRectangle (184.0f, 186.0f, (float) getWidth() - 196.0f, 476.0f, 12.0f);

    const char* qModeNames[3] = { "MODE: NORM", "MODE: DOT .", "MODE: TRIP 3" };
    drawBtn ({ 200, 204, 110, 60 }, qModeNames[quantMode], juce::Colour (0xffdff1ff), juce::Colour (0xff1e232e));
    for (int q = 0; q < 5; ++q) {
        drawBtn ({ 318 + q * 114, 204, 106, 60 }, kBaseQuantNames[q],
                 (!freeQuantActive && selectedBaseQuant == q) ? juce::Colour (0xffff9ebb) : bgPanel,
                 (!freeQuantActive && selectedBaseQuant == q) ? juce::Colour (0xff1e232e) : textMain);
    }
    drawBtn ({ 888, 204, 120, 60 }, "FREE", freeQuantActive ? juce::Colour (0xffff9ebb) : juce::Colour (0xffffe4ed), juce::Colour (0xff1e232e));

    // REST + 8 Velocity Tiles (ppp..fff)
    drawBtn ({ 200, 286, 120, 176 }, "REST", juce::Colour (0xff8ecae6), juce::Colour (0xff17212b));
    for (int v = 0; v < 8; ++v) {
        int col = v % 4;
        int row = v / 4;
        juce::Rectangle<int> vr (332 + col * 165, 286 + row * 92, 155, 84);
        drawBtn (vr, kVelNames[v], juce::Colour (kVelColors[v]), juce::Colour (0xff1e232e));
    }

    // Bottom Transport Cursor Buttons
    const char* botOps[6] = { "< NOTE", "NOTE >", "< QUANT", "QUANT >", "DELETE", "CHANGE" };
    for (int b = 0; b < 6; ++b) {
        drawBtn ({ 200 + b * 136, 488, 128, 52 }, botOps[b], bgPanel, textMain);
    }

    // About #fff Modal
    if (showAboutModal) {
        g.fillAll (juce::Colours::black.withAlpha (0.6f));
        juce::Rectangle<float> mb (250.0f, 150.0f, 540.0f, 360.0f);
        g.setColour (bgPanel);
        g.fillRoundedRectangle (mb, 14.0f);
        drawFffLogo (g, { 310.0f, 185.0f, 420.0f, 155.0f });
        g.setColour (textMain);
        g.setFont (15.0f);
        g.drawFittedText ("#fff means \"Faded, Fit, and Fast\". For some people, it's a nostalgic sequencer (?) that feels comfortable to use and lets you enter notes quickly...",
                          280, 360, 480, 70, juce::Justification::centred, 3);
        drawBtn ({ 460, 445, 120, 40 }, "CLOSE", juce::Colour (0xff8ecae6), juce::Colour (0xff1e232e));
    }

    // Double-Click Clear All Confirmation Modal
    if (showClearAllModal) {
        g.fillAll (juce::Colours::black.withAlpha (0.6f));
        g.setColour (bgPanel);
        g.fillRoundedRectangle (320.0f, 230.0f, 400.0f, 200.0f, 14.0f);
        g.setColour (textMain);
        g.setFont (18.0f);
        g.drawText ("Do you want to delete all tracks?", 340, 260, 360, 40, juce::Justification::centred);
        drawBtn ({ 390, 340, 110, 44 }, "Yes", juce::Colour (0xffff9ebb), juce::Colour (0xff1e232e));
        drawBtn ({ 540, 340, 110, 44 }, "Cancel", bgBoard, textMain);
    }
}

void FffAudioProcessorEditor::mouseDown (const juce::MouseEvent& e) {
    auto pt = e.getPosition();
    if (showAboutModal) { showAboutModal = false; repaint(); return; }
    if (showClearAllModal) {
        if (juce::Rectangle<int> (390, 340, 110, 44).contains (pt)) {
            for (auto& t : proc.tracks) t.events.clear();
            proc.cursorTick = 0;
        }
        showClearAllModal = false;
        repaint();
        return;
    }

    if (juce::Rectangle<int> (20, 16, 84, 36).contains (pt)) { showAboutModal = true; repaint(); return; }
    if (juce::Rectangle<int> (112, 16, 76, 36).contains (pt)) { proc.isPlaying = !proc.isPlaying; proc.currentGlobalTick = 0; repaint(); return; }
    if (juce::Rectangle<int> (194, 16, 66, 36).contains (pt)) { proc.tracks[proc.selectedTrack].events.clear(); proc.cursorTick = 0; repaint(); return; }
    if (juce::Rectangle<int> (354, 16, 92, 36).contains (pt)) { dragKnobType = 1; dragStartY = pt.y; dragStartVal = proc.globalSwing; return; }
    if (juce::Rectangle<int> (654, 77, 102, 36).contains (pt)) { dragKnobType = 2; dragStartY = pt.y; dragStartVal = proc.tracks[proc.selectedTrack].swing; return; }
    if (juce::Rectangle<int> (452, 16, 106, 36).contains (pt)) { proc.polyStretchMode = !proc.polyStretchMode; repaint(); return; }
    if (juce::Rectangle<int> (936, 16, 82, 36).contains (pt)) { proc.darkMode = !proc.darkMode; repaint(); return; }

    auto& tr = proc.tracks[proc.selectedTrack];
    if (juce::Rectangle<int> (480, 77, 32, 36).contains (pt)) { tr.steps = juce::jlimit (1, 64, tr.steps - 1); repaint(); return; }
    if (juce::Rectangle<int> (610, 77, 32, 36).contains (pt)) { tr.steps = juce::jlimit (1, 64, tr.steps + 1); repaint(); return; }

    for (int t = 0; t < 8; ++t) {
        if (juce::Rectangle<int> (16, 186 + t * 56, 155, 50).contains (pt)) {
            proc.selectedTrack = t;
            proc.triggerTrack (t, 5);
            repaint();
            return;
        }
    }

    if (juce::Rectangle<int> (200, 204, 110, 60).contains (pt)) { quantMode = (quantMode + 1) % 3; freeQuantActive = false; repaint(); return; }
    for (int q = 0; q < 5; ++q) {
        if (juce::Rectangle<int> (318 + q * 114, 204, 106, 60).contains (pt)) {
            selectedBaseQuant = q; freeQuantActive = false; repaint(); return;
        }
    }

    int qTicks = kBaseQuantTicks[selectedBaseQuant];
    if (quantMode == 1) qTicks = (qTicks * 3) / 2;
    else if (quantMode == 2) qTicks = (qTicks * 2) / 3;
    int maxTicks = std::max (96, tr.steps * 96);

    if (juce::Rectangle<int> (200, 286, 120, 176).contains (pt)) {
        proc.cursorTick = (proc.cursorTick + qTicks) % maxTicks;
        repaint();
        return;
    }

    for (int v = 0; v < 8; ++v) {
        int col = v % 4, row = v / 4;
        if (juce::Rectangle<int> (332 + col * 165, 286 + row * 92, 155, 84).contains (pt)) {
            int pos = proc.cursorTick % maxTicks;
            tr.events.erase (std::remove_if (tr.events.begin(), tr.events.end(), [&](const NoteEvent& e) { return std::abs (e.tick - pos) <= 4; }), tr.events.end());
            tr.events.push_back ({ pos, v, qTicks });
            proc.triggerTrack (proc.selectedTrack, v);
            proc.cursorTick = (pos + qTicks) % maxTicks;
            repaint();
            return;
        }
    }

    // Bottom buttons
    if (juce::Rectangle<int> (200 + 2 * 136, 488, 128, 52).contains (pt)) { proc.cursorTick = (proc.cursorTick - qTicks + maxTicks) % maxTicks; repaint(); return; }
    if (juce::Rectangle<int> (200 + 3 * 136, 488, 128, 52).contains (pt)) { proc.cursorTick = (proc.cursorTick + qTicks) % maxTicks; repaint(); return; }
    if (juce::Rectangle<int> (200 + 4 * 136, 488, 128, 52).contains (pt)) {
        tr.events.erase (std::remove_if (tr.events.begin(), tr.events.end(), [&](const NoteEvent& e) { return std::abs (e.tick - proc.cursorTick) < qTicks; }), tr.events.end());
        repaint();
        return;
    }
}

void FffAudioProcessorEditor::mouseDrag (const juce::MouseEvent& e) {
    if (dragKnobType == 1) {
        proc.globalSwing = juce::jlimit (0, 100, dragStartVal + (dragStartY - e.getPosition().y));
        repaint();
    } else if (dragKnobType == 2) {
        proc.tracks[proc.selectedTrack].swing = juce::jlimit (0, 100, dragStartVal + (dragStartY - e.getPosition().y));
        repaint();
    }
}

void FffAudioProcessorEditor::mouseDoubleClick (const juce::MouseEvent& e) {
    auto pt = e.getPosition();
    if (juce::Rectangle<int> (194, 16, 66, 36).contains (pt)) { showClearAllModal = true; repaint(); return; }
    if (juce::Rectangle<int> (354, 16, 92, 36).contains (pt)) { proc.globalSwing = 50; repaint(); return; }
    if (juce::Rectangle<int> (654, 77, 102, 36).contains (pt)) { proc.tracks[proc.selectedTrack].swing = 50; repaint(); return; }
}

juce::AudioProcessorEditor* FffAudioProcessor::createEditor() {
    return new FffAudioProcessorEditor (*this);
}
