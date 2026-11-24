// Slice 1 bring-up application: self-tests, the report and the serial console (spec 001).
// Uses only src/platform, lib/core and lib/ui headers (constitution VI).
//
// Stage 1 (tasks.md Phases 1-3 and 7): silence, backlight full on, storage, report. No display,
// touch, Wi-Fi or stress yet; those self-tests are reported as skipped.
#pragma once

namespace app {

// Prints the `audio-off: ...` line on its own (plan startup step 2).
void print_silence_line();

// Runs the remaining start-up steps and prints the report (FR-026). Call after audio_off() and
// the console have started.
void start();

// Call repeatedly from the UI task (src/main.cpp, core 1).
void loop();

// FR-001a: stress mode, Wi-Fi setup and Wi-Fi join refuse to start while the silence step is
// not acknowledged, until a restart passes it.
bool can_use_radio_and_load();

}  // namespace app
