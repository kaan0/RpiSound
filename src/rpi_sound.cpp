#include <algorithm>
#include <atomic>
#include <chrono>
#include <concepts>
#include <format>
#include <iostream>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <thread>
#include <vector>

#include <ftxui/component/captured_mouse.hpp>
#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>

#if defined(USE_COREAUDIO)
#include "core_audio/core_audio_device_enumerator.hpp"
#include "core_audio/core_audio_driver.hpp"
#else
#include "alsa/alsa_device_enumerator.hpp"
#include "alsa/alsa_driver.hpp"
#endif

#include "rpi_sound/audio_device_factory.hpp"
#include "rpi_sound/audio_device_manager.hpp"
#include "rpi_sound/audio_engine.hpp"
#include "rpi_sound/pcm_loader.hpp"
#include "rpi_sound/sound_manager.hpp"

using namespace ftxui;

// ---------------------------------------------------------------------------
// Strong types
// ---------------------------------------------------------------------------

struct MidiVelocity {
    explicit MidiVelocity(int v) noexcept : value(std::clamp(v, 0, 127)) {}
    int value;
};

using DeviceIndex = std::size_t;
using SampleIndex = std::size_t;

// ---------------------------------------------------------------------------
// RAII mouse reporting guard
//
// Emits the three escape sequences that enable SGR extended mouse reporting
// on construction, and cleanly disables them on destruction — even if the
// stack unwinds via an exception.  The guard is move-only so it can be stored
// in a std::optional and released early if needed.
//
// Terminal support:
//   ?1000h  – button-event tracking (press + release)
//   ?1002h  – button-event + drag tracking
//   ?1006h  – SGR extended coordinate mode (required by FTXUI for wide terminals)
//
// Note: Terminal.app on macOS does not implement ?1006h and will not produce
// mouse events regardless.  Use iTerm2, Kitty, or WezTerm for full support.
// ---------------------------------------------------------------------------

class MouseReportingGuard {
public:
    MouseReportingGuard() {
        std::cout << "\033[?1000h"  // button events
                  << "\033[?1002h"  // button + drag
                  << "\033[?1006h"  // SGR extended mode
                  << std::flush;
    }

    ~MouseReportingGuard() { disable(); }

    // Move-only — copying would double-disable on destruction.
    MouseReportingGuard(const MouseReportingGuard&) = delete;
    MouseReportingGuard& operator=(const MouseReportingGuard&) = delete;

    MouseReportingGuard(MouseReportingGuard&& other) noexcept : m_active(std::exchange(other.m_active, false)) {}

    MouseReportingGuard& operator=(MouseReportingGuard&& other) noexcept {
        if (this != &other) {
            disable();
            m_active = std::exchange(other.m_active, false);
        }
        return *this;
    }

    // Allow explicit early release (e.g. before spawning a child process).
    void release() noexcept { disable(); }

private:
    void disable() noexcept {
        if (!m_active)
            return;
        std::cout << "\033[?1006l"  // disable SGR extended mode
                  << "\033[?1002l"  // disable drag tracking
                  << "\033[?1000l"  // disable button events
                  << std::flush;
        m_active = false;
    }

    bool m_active = true;
};

// ---------------------------------------------------------------------------
// SoundUIBackend concept
//
// Decouples the UI from SoundManager entirely.  Any type satisfying this
// concept can back the UI — no std::function overhead, no nullable callbacks,
// violations caught at compile time.
// ---------------------------------------------------------------------------

template <typename T>
concept SoundUIBackend = requires(T & t, DeviceIndex di, SampleIndex si, MidiVelocity v) {
    {t.selectDevice(di)}->std::same_as<bool>;
    {t.triggerSound(si, v)}->std::same_as<bool>;
};

// ---------------------------------------------------------------------------
// RpiSoundUI
// ---------------------------------------------------------------------------

template <SoundUIBackend Backend>
class RpiSoundUI {
public:
    RpiSoundUI(Backend& backend, std::span<const std::string> devices, std::span<const std::string> samples)
        : m_backend(backend),
          m_deviceEntries(devices.begin(), devices.end()),
          m_sampleEntries(samples.begin(), samples.end()),
          m_selectedDeviceIndex(0),
          m_selectedSampleIndex(0),
          m_velocity(100),
          m_activeDeviceIndex(std::nullopt),
          m_statusMessage("Ready — select a device to start"),
          m_lastTriggeredSound() {}

    Component CreateUI(ScreenInteractive& screen) {
        return Renderer(Container::Vertical({
                            MakeDevicePanel(),
                            MakeSamplePanel(),
                            MakeMidiPanel(),
                            MakeControlsPanel(screen),
                        }),
                        [this](Component& root) {
                            // Expose the horizontal split through the renderer
                            auto devicePanel = MakeDevicePanel();
                            auto samplePanel = MakeSamplePanel();
                            auto midiPanel = MakeMidiPanel();
                            auto controlPanel = MakeControlsPanel_Renderer();

                            return vbox({
                                MakeHeader(),
                                hbox({
                                    devicePanel->Render(),
                                    samplePanel->Render(),
                                    midiPanel->Render(),
                                    controlPanel->Render(),
                                }) | flex,
                                MakeStatusBar(),
                            });
                        });
    }

    // Public entry point that wires components and runs the layout correctly.
    Component Build(ScreenInteractive& screen) {
        auto devicePanel = MakeDevicePanel();
        auto samplePanel = MakeSamplePanel();
        auto midiPanel = MakeMidiPanel();
        auto controlPanel = MakeControlsPanel(screen);

        auto mainContainer = Container::Horizontal({
            devicePanel,
            samplePanel,
            midiPanel,
            controlPanel,
        });

        auto root = Renderer(mainContainer, [this, mainContainer] {
            return vbox({
                MakeHeader(),
                mainContainer->Render() | flex,
                MakeStatusBar(),
            });
        });

        root |= CatchEvent([&screen](Event event) {
            if (event == Event::Character('q') || event == Event::Character('Q')) {
                screen.ExitLoopClosure()();
                return true;
            }
            return false;
        });

        return root;
    }

private:
    // -- Data ----------------------------------------------------------------

    Backend& m_backend;
    std::vector<std::string> m_deviceEntries;
    std::vector<std::string> m_sampleEntries;

    int m_selectedDeviceIndex;
    int m_selectedSampleIndex;
    int m_velocity;

    std::optional<DeviceIndex> m_activeDeviceIndex;  // nullopt = no device selected
    std::string m_statusMessage;
    std::string m_lastTriggeredSound;

    // -- Helpers -------------------------------------------------------------

    bool isDeviceSelected() const noexcept { return m_activeDeviceIndex.has_value(); }

    void onSelectDevice() {
        const auto idx = static_cast<DeviceIndex>(m_selectedDeviceIndex);
        if (m_backend.selectDevice(idx)) {
            m_activeDeviceIndex = idx;
            m_statusMessage = std::format("✓ Device selected: {}", m_deviceEntries[idx]);
        } else {
            m_activeDeviceIndex = std::nullopt;
            m_statusMessage = std::format("❌ Failed to select device: {}", m_deviceEntries[idx]);
        }
    }

    void onTriggerSound() {
        if (!isDeviceSelected()) {
            m_statusMessage = "❌ Please select a device first";
            return;
        }
        const auto idx = static_cast<SampleIndex>(m_selectedSampleIndex);
        const auto vel = MidiVelocity(m_velocity);
        if (m_backend.triggerSound(idx, vel)) {
            m_lastTriggeredSound = m_sampleEntries[idx];
            m_statusMessage = std::format("♪ Played sample: {}", m_lastTriggeredSound);
        } else {
            m_statusMessage = std::format("❌ Failed to play sample: {}", m_sampleEntries[idx]);
        }
    }

    // -- Panel factories -----------------------------------------------------

    Component MakeDevicePanel() {
        auto menu = Menu(&m_deviceEntries, &m_selectedDeviceIndex);
        auto selectBtn = Button("Select Device", [this] { onSelectDevice(); });

        return Container::Vertical({
                   Renderer([] { return text("Audio Devices") | ftxui::bold | center; }),
                   Renderer([] { return separator(); }),
                   menu | flex,
                   Renderer([] { return separator(); }),
                   selectBtn,
               }) |
               border | size(WIDTH, EQUAL, 40);
    }

    Component MakeSamplePanel() {
        auto menu = Menu(&m_sampleEntries, &m_selectedSampleIndex);
        auto triggerBtn = Button("Trigger Sound", [this] { onTriggerSound(); });

        return Container::Vertical({
                   Renderer([] { return text("Available Samples") | ftxui::bold | center; }),
                   Renderer([] { return separator(); }),
                   Renderer(menu, [menu] { return menu->Render() | vscroll_indicator | frame; }) | flex,
                   Renderer([] { return separator(); }),
                   triggerBtn,
               }) |
               border | flex;
    }

    Component MakeMidiPanel() {
        return Container::Vertical({
                   Renderer([] { return text("MIDI Mappings") | ftxui::bold | center; }),
                   Renderer([] { return separator(); }),
                   Renderer([] { return text("MIDI mapping UI coming soon...") | color(Color::GrayDark); }) | flex,
               }) |
               border | flex;
    }

    Component MakeControlsPanel(ScreenInteractive& screen) {
        // +/- buttons replace the sticky Slider — they release mouse focus correctly.
        auto decFast = Button("-10", [this] { m_velocity = std::max(0, m_velocity - 10); });
        auto dec = Button(" -  ", [this] { m_velocity = std::max(0, m_velocity - 1); });
        auto inc = Button(" +  ", [this] { m_velocity = std::min(127, m_velocity + 1); });
        auto incFast = Button("+10", [this] { m_velocity = std::min(127, m_velocity + 10); });
        auto quitBtn = Button("Quit", [&screen] { screen.ExitLoopClosure()(); });

        return Container::Vertical({
                   Renderer([] { return text("Controls") | ftxui::bold | center; }),
                   Renderer([] { return separator(); }),

                   // Velocity row
                   Container::Horizontal({decFast, dec, inc, incFast}),
                   Renderer([this] {
                       return hbox({
                           text("Velocity: "),
                           text(std::format("{:3}", m_velocity)) | ftxui::bold | color(Color::Yellow),
                       });
                   }),

                   Renderer([] { return separator(); }),
                   Renderer([this] { return text("Device:") | ftxui::bold; }),
                   Renderer([this] {
                       const bool connected = isDeviceSelected();
                       return text(connected ? "✓ Connected" : "✗ Not connected") |
                              color(connected ? Color::Green : Color::Red);
                   }),

                   Renderer([] { return separator(); }),
                   Renderer([this] { return text("Last played:") | ftxui::bold; }),
                   Renderer([this] {
                       const auto& label = m_lastTriggeredSound.empty() ? "-" : m_lastTriggeredSound;
                       return text(label) | color(Color::Magenta);
                   }),

                   Renderer([] { return vbox({}) | flex; }),
                   Renderer([] { return separator(); }),
                   quitBtn,
               }) |
               border | size(WIDTH, EQUAL, 30);
    }

    // Stateless renderer variant used inside the Renderer lambda of Build().
    Component MakeControlsPanel_Renderer() {
        // Thin wrapper — returns a non-interactive renderer for layout only.
        // Interactive variant is wired in Build().
        return Renderer([this] {
            return vbox({
                text("Controls") | ftxui::bold | center,
                separator(),
                hbox({
                    text("Velocity: "),
                    text(std::format("{:3}", m_velocity)) | ftxui::bold | color(Color::Yellow),
                }),
                separator(),
                text("Device:") | ftxui::bold,
                text(isDeviceSelected() ? "✓ Connected" : "✗ Not connected") |
                    color(isDeviceSelected() ? Color::Green : Color::Red),
            });
        });
    }

    // -- Shared render helpers -----------------------------------------------

    Element MakeHeader() const {
        return hbox({text("🎵 Raspberry Pi Sound System 🎵") | ftxui::bold | center}) | border | color(Color::Cyan);
    }

    Element MakeStatusBar() const {
        const bool isError = m_statusMessage.contains("❌");
        const bool isSuccess = m_statusMessage.contains("✓");
        const bool isPlaying = m_statusMessage.contains("♪");

        const Color msgColor = isError     ? Color::Red
                               : isSuccess ? Color::Green
                               : isPlaying ? Color::Yellow
                                           : Color::White;

        return hbox({text(" Status: "), text(m_statusMessage) | ftxui::bold | color(msgColor)}) | border |
               color(Color::Cyan);
    }
};

class SoundManagerBackend {
public:
    SoundManagerBackend(SoundManager& mgr,
                        std::span<const types::AudioDeviceInfo> devices,
                        std::span<const std::string> samples)
        : m_manager(mgr), m_devices(devices.begin(), devices.end()), m_samples(samples.begin(), samples.end()) {}

    bool selectDevice(DeviceIndex idx) {
        if (idx >= m_devices.size())
            return false;
        return m_manager.selectAudioDevice(m_devices[idx]);
    }

    bool triggerSound(SampleIndex idx, MidiVelocity velocity) {
        if (idx >= m_samples.size())
            return false;
        return m_manager.triggerSound(m_samples[idx], static_cast<uint32_t>(velocity.value));
    }

private:
    SoundManager& m_manager;
    std::vector<types::AudioDeviceInfo> m_devices;
    std::vector<std::string> m_samples;
};

static_assert(SoundUIBackend<SoundManagerBackend>);

struct AudioBackends {
#if defined(USE_COREAUDIO)
    CoreAudioDriver driver;
    CoreAudioDeviceEnumerator enumerator;
#else
    AlsaDriver driver;
    AlsaDeviceEnumerator enumerator;
#endif
};

int main() {
    const MouseReportingGuard mouseGuard;

    AudioBackends backends;
    AudioDeviceFactory deviceFactory;

    auto audioDeviceManager = AudioDeviceManager(deviceFactory, backends.enumerator, backends.driver);

    if (!audioDeviceManager.isInitialized()) {
        std::cerr << "Failed to initialize Audio Device Manager.\n";
        return -1;
    }

    SoundManager soundManager(std::make_unique<PcmLoader>(), audioDeviceManager, createAudioEngine(512));

    if (!soundManager.initialize()) {
        std::cerr << "Failed to initialize Sound Manager.\n";
        return -1;
    }

    const auto availableDevices = soundManager.getAvailableAudioDevices(types::AudioDeviceInfo::DeviceType::kPlayback);

    const auto availableDeviceDescriptions =
        soundManager.getAvailableAudioDeviceDescriptions(types::AudioDeviceInfo::DeviceType::kPlayback);

    if (availableDevices.empty()) {
        std::cerr << "No audio devices available.\n";
        return -1;
    }

    if (!soundManager.load("demo")) {
        std::cerr << "Failed to load instrument samples.\n";
        return -1;
    }

    const auto samples = soundManager.getAvailableSamples();

    SoundManagerBackend backend(soundManager, availableDevices, samples);

    auto screen = ScreenInteractive::Fullscreen();
    auto app = RpiSoundUI(backend, availableDeviceDescriptions, samples);
    auto ui = app.Build(screen);

    screen.Loop(ui);
    return 0;
}