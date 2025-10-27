#include <algorithm>
#include <atomic>
#include <chrono>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include <ftxui/component/captured_mouse.hpp>
#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>

using namespace ftxui;

class RpiSoundUI {
public:
    RpiSoundUI()
        : selectedDeviceIndex_(0),
          selectedSampleIndex_(0),
          velocityValue_(100),
          statusMessage_("Ready - Select a device to start"),
          lastTriggeredSound_(""),
          deviceSelected_(false) {

        // Dummy data for testing UI
        deviceEntries_ = {
            "0: USB Audio Device (Card: 0, Device: 0)",
            "1: HDMI Audio Output (Card: 1, Device: 0)",
            "2: Built-in Audio (Card: 2, Device: 0)",
            "3: Bluetooth Speaker (Card: 3, Device: 0)",
        };

        sampleEntries_ = {
            "0: kick",
            "1: snare",
            "2: hi_hat_closed",
            "3: hi_hat_open",
            "4: tom_low",
            "5: tom_mid",
            "6: tom_high",
            "7: crash",
            "8: ride",
            "9: rim",
        };
    }

    Component CreateUI() {
        // Device selection list
        auto deviceMenu = Menu(&deviceEntries_, &selectedDeviceIndex_);

        // Sample list
        auto sampleMenu = Menu(&sampleEntries_, &selectedSampleIndex_);

        // Velocity slider
        auto velocitySlider = Slider("Velocity: ", &velocityValue_, 0, 127, 1);

        // Buttons
        auto selectDeviceButton = Button("Select Device", [this] {
            deviceSelected_ = true;
            statusMessage_ = "✓ Device selected: " + deviceEntries_[selectedDeviceIndex_];
        });

        auto triggerSoundButton = Button("Trigger Sound", [this] {
            if (!deviceSelected_) {
                statusMessage_ = "❌ Please select a device first";
                return;
            }

            lastTriggeredSound_ = sampleEntries_[selectedSampleIndex_];
            statusMessage_ =
                "♪ Playing: " + lastTriggeredSound_ + " (velocity: " + std::to_string(velocityValue_) + ")";
        });

        auto quitButton = Button("Quit", [this] { shouldExit_ = true; });

        // Layout containers
        auto deviceContainer = Container::Vertical({
                                   Renderer([&] { return text("Audio Devices") | bold | center; }),
                                   Renderer([] { return separator(); }),
                                   deviceMenu | flex,
                                   Renderer([] { return separator(); }),
                                   selectDeviceButton,
                               }) |
                               border | size(WIDTH, EQUAL, 40);

        auto sampleContainer = Container::Vertical({
                                   Renderer([&] { return text("Available Samples") | bold | center; }),
                                   Renderer([] { return separator(); }),
                                   sampleMenu | flex,
                                   Renderer([] { return separator(); }),
                                   triggerSoundButton,
                               }) |
                               border | flex;

        auto controlsContainer =
            Container::Vertical({
                Renderer([&] { return text("Controls") | bold | center; }),
                Renderer([] { return separator(); }),
                Renderer([this] {
                    return hbox({
                        text("Velocity: "),
                        text(std::to_string(velocityValue_)) | bold | color(Color::Yellow),
                    });
                }),
                velocitySlider,
                Renderer([] { return separator(); }),
                Renderer([this] { return text("Device Status:") | bold; }),
                Renderer([this] {
                    return text(deviceSelected_ ? "✓ Connected" : "✗ Not connected") |
                           color(deviceSelected_ ? Color::Green : Color::Red);
                }),
                Renderer([] { return separator(); }),
                Renderer([this] { return text("Last Played:") | bold; }),
                Renderer([this] {
                    return text(lastTriggeredSound_.empty() ? "-" : lastTriggeredSound_) | color(Color::Magenta);
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
                    text(statusMessage_) | bold |
                        color(statusMessage_.find("❌") != std::string::npos  ? Color::Red
                              : statusMessage_.find("✓") != std::string::npos ? Color::Green
                              : statusMessage_.find("♪") != std::string::npos ? Color::Yellow
                                                                              : Color::White),
                }) | border |
                    color(Color::Cyan),
            });
        });

        // component = component | CatchEvent([this](Event event) {
        //                 if (event == Event::Character('q') || event == Event::Character('Q') || shouldExit_) {
        //                     shouldExit_ = true;
        //                     return true;
        //                 }
        //                 return false;
        //             });

        return component;
    }

    void Run() {
        auto screen = ScreenInteractive::Fullscreen();
        auto component = CreateUI();

        std::atomic<bool> refresh_ui_continue = true;
        std::thread refresh_ui([&] {
            while (refresh_ui_continue) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                screen.PostEvent(Event::Custom);
                if (shouldExit_) {
                    screen.ExitLoopClosure()();
                    break;
                }
            }
        });

        screen.Loop(component);
        refresh_ui_continue = false;
        refresh_ui.join();
    }
    bool shouldExit_ = false;

private:
    int selectedDeviceIndex_;
    int selectedSampleIndex_;
    int velocityValue_;
    std::string statusMessage_;
    std::string lastTriggeredSound_;
    bool deviceSelected_;

    std::vector<std::string> deviceEntries_;
    std::vector<std::string> sampleEntries_;
};

int main() {
    // RpiSoundUI ui;
    // ui.Run();
    auto screen = ScreenInteractive::Fullscreen();
    auto ui = RpiSoundUI();
    auto app = ui.CreateUI();

    app |= CatchEvent([&](Event event) {
        if (event == Event::Character('q') || event == Event::Character('Q') || ui.shouldExit_) {
            screen.ExitLoopClosure()();
            return true;
        }
        return false;
    });

    screen.Loop(app);
    return 0;
}