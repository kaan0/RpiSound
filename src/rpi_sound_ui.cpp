#include <algorithm>
#include <atomic>
#include <chrono>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include <ftxui/component/captured_mouse.hpp>
#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>

#include "alsa/alsa_device_enumerator.hpp"
#include "alsa/alsa_driver.hpp"
#include "rpi_sound/audio_device_factory.hpp"
#include "rpi_sound/audio_device_manager.hpp"
#include "rpi_sound/audio_engine.hpp"
#include "rpi_sound/pcm_loader.hpp"
#include "rpi_sound/sound_manager.hpp"

using namespace ftxui;

struct UICallbacks {
    std::function<bool(int deviceIndex)> onDeviceSelected;
    std::function<bool(int sampleIndex, int velocity)> onSoundTriggered;
    std::function<void()> onQuit;
};

class RpiSoundUI {
public:
    RpiSoundUI(std::vector<std::string> deviceEntries, std::vector<std::string> sampleEntries, UICallbacks callbacks)
        : m_selectedDeviceIndex(0),
          m_selectedSampleIndex(0),
          m_velocityValue(100),
          m_statusMessage("Ready - Select a device to start"),
          m_lastTriggeredSound(""),
          m_deviceEntries(std::move(deviceEntries)),
          m_sampleEntries(std::move(sampleEntries)),
          m_callbacks(std::move(callbacks)),
          m_deviceSelected(false) {}

    Component CreateUI(ScreenInteractive& screen) {
        // Device selection list
        auto deviceMenu = Menu(&m_deviceEntries, &m_selectedDeviceIndex);

        // Sample list
        auto sampleMenu = Menu(&m_sampleEntries, &m_selectedSampleIndex);

        // Velocity slider
        auto velocitySlider = Slider("Velocity: ", &m_velocityValue, 0, 127, 1);

        // Buttons
        auto selectDeviceButton = Button("Select Device", [this] {
            if (m_callbacks.onDeviceSelected) {
                if (!m_callbacks.onDeviceSelected(m_selectedDeviceIndex)) {
                    m_statusMessage = "❌ Failed to select device: " + m_deviceEntries[m_selectedDeviceIndex];
                    return;
                } else {
                    m_deviceSelected = true;
                    m_statusMessage = "✓ Device selected: " + m_deviceEntries[m_selectedDeviceIndex];
                    return;
                }
            }
        });

        auto triggerSoundButton = Button("Trigger Sound", [this] {
            if (!m_deviceSelected) {
                m_statusMessage = "❌ Please select a device first";
                return;
            }

            if (m_callbacks.onSoundTriggered) {
                if (!m_callbacks.onSoundTriggered(m_selectedSampleIndex, m_velocityValue)) {
                    m_statusMessage = "❌ Failed to play sample: " + m_sampleEntries[m_selectedSampleIndex];
                    return;
                } else {
                    m_lastTriggeredSound = m_sampleEntries[m_selectedSampleIndex];
                    m_statusMessage = "♪ Played sample: " + m_lastTriggeredSound;
                    return;
                }
            }
        });

        auto quitButton = Button("Quit", [&screen] { screen.ExitLoopClosure()(); });

        // Layout containers
        auto deviceContainer = Container::Vertical({
                                   Renderer([&] { return text("Audio Devices") | bold | center; }),
                                   Renderer([] { return separator(); }),
                                   deviceMenu | flex,
                                   Renderer([] { return separator(); }),
                                   selectDeviceButton,
                               }) |
                               border | size(WIDTH, EQUAL, 40);

        auto sampleContainer =
            Container::Vertical({
                Renderer([&] { return text("Available Samples") | bold | center; }),
                Renderer([] { return separator(); }),
                Renderer(sampleMenu, [sampleMenu] { return sampleMenu->Render() | vscroll_indicator | frame; }) | flex,
                Renderer([] { return separator(); }),
                triggerSoundButton,
            }) |
            border | flex;

        auto midiMappingContainer =
            Container::Vertical({
                Renderer([&] { return text("MIDI Mappings") | bold | center; }),
                Renderer([] { return separator(); }),
                // Placeholder for future MIDI mapping UI elements
                Renderer([] { return text("MIDI mapping UI coming soon...") | color(Color::Grey0); }) | flex,
            }) |
            border | flex;

        auto controlsContainer =
            Container::Vertical({
                Renderer([&] { return text("Controls") | bold | center; }),
                Renderer([] { return separator(); }),
                Renderer([this] {
                    return hbox({
                        text("Velocity: "),
                        text(std::to_string(m_velocityValue)) | bold | color(Color::Yellow),
                    });
                }),
                velocitySlider,
                Renderer([] { return separator(); }),
                Renderer([this] { return text("Device Status:") | bold; }),
                Renderer([this] {
                    return text(m_deviceSelected ? "✓ Connected" : "✗ Not connected") |
                           color(m_deviceSelected ? Color::Green : Color::Red);
                }),
                Renderer([] { return separator(); }),
                Renderer([this] { return text("Last Played:") | bold; }),
                Renderer([this] {
                    return text(m_lastTriggeredSound.empty() ? "-" : m_lastTriggeredSound) | color(Color::Magenta);
                }),
                Renderer([] { return separator(); }),
                Renderer([] { return vbox({}) | flex; }),
                Renderer([] { return separator(); }),
                quitButton,
            }) |
            border | size(WIDTH, EQUAL, 30);

        auto mainContainer = Container::Horizontal({
            deviceContainer,
            sampleContainer,
            midiMappingContainer,
            controlsContainer,
        });

        // Main renderer
        auto component = Renderer(mainContainer, [this, mainContainer] {
            return vbox({
                // Header
                hbox({
                    text("🎵 Raspberry Pi Sound System 🎵") | bold | center,
                }) | border |
                    color(Color::Cyan),

                // Main content area - let the containers render themselves
                hbox({
                    mainContainer->Render() | flex,
                }) | flex,

                // Status bar
                hbox({
                    text(" Status: "),
                    text(m_statusMessage) | bold |
                        color(m_statusMessage.find("❌") != std::string::npos  ? Color::Red
                              : m_statusMessage.find("✓") != std::string::npos ? Color::Green
                              : m_statusMessage.find("♪") != std::string::npos ? Color::Yellow
                                                                               : Color::White),
                }) | border |
                    color(Color::Cyan),
            });
        });

        return component;
    }

private:
    int m_selectedDeviceIndex;
    int m_selectedSampleIndex;
    int m_velocityValue;
    std::string m_statusMessage;
    std::string m_lastTriggeredSound;
    bool m_deviceSelected;

    std::vector<std::string> m_deviceEntries;
    std::vector<std::string> m_sampleEntries;
    UICallbacks m_callbacks;
};

int main() {
    AlsaDriver alsaDriver;
    AlsaDeviceEnumerator alsaEnumerator;
    AudioDeviceFactory deviceFactory;

    auto audioDeviceManager = AudioDeviceManager(deviceFactory, alsaEnumerator, alsaDriver);

    if (!audioDeviceManager.isInitialized()) {
        std::cerr << "Failed to initialize Audio Device Manager." << std::endl;
        return -1;
    }

    SoundManager soundManager(std::make_unique<PcmLoader>(), audioDeviceManager, createAudioEngine(20971520, 2048));

    if (!soundManager.initialize()) {
        std::cerr << "Failed to initialize Sound Manager." << std::endl;
        return -1;
    }

    auto availableDevices = soundManager.getAvailableAudioDevices();
    auto availableDeviceDescriptions = soundManager.getAvailableAudioDeviceDescriptions();
    if (availableDevices.empty()) {
        std::cerr << "No audio devices available." << std::endl;
        return -1;
    }

    if (!soundManager.load("demo")) {
        std::cerr << "Failed to load instrument samples." << std::endl;
        return -1;
    }

    auto samples = soundManager.getAvailableSamples();

    UICallbacks uiCallbacks;
    uiCallbacks.onDeviceSelected = [&soundManager, &availableDevices](int deviceIndex) -> bool {
        if (deviceIndex >= 0 && deviceIndex < static_cast<int>(availableDevices.size())) {
            return soundManager.selectAudioDevice(availableDevices[deviceIndex]);
        }
        return false;
    };

    uiCallbacks.onSoundTriggered = [&soundManager, &samples](int sampleIndex, int velocity) -> bool {
        if (sampleIndex >= 0 && sampleIndex < static_cast<int>(samples.size())) {
            return soundManager.triggerSound(samples[sampleIndex], static_cast<uint32_t>(velocity));
        }
        return false;
    };

    auto screen = ScreenInteractive::Fullscreen();
    auto app = RpiSoundUI(availableDeviceDescriptions, samples, uiCallbacks);
    auto ui = app.CreateUI(screen);

    ui |= CatchEvent([&](Event event) {
        if (event == Event::Character('q') || event == Event::Character('Q')) {
            screen.ExitLoopClosure()();
            return true;
        }
        return false;
    });

    screen.Loop(ui);
    return 0;
}