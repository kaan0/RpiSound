#include <chrono>
#include <functional>
#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>
#include <thread>
#include <unordered_map>

#include <spdlog/spdlog.h>
#include <argparse/argparse.hpp>

#include "alsa/alsa_device_enumerator.hpp"
#include "alsa/alsa_driver.hpp"
#include "rpi_sound/audio_device_factory.hpp"
#include "rpi_sound/audio_device_manager.hpp"
#include "rpi_sound/pcm_loader.hpp"
#include "rpi_sound/sound_manager.hpp"

// Command context to pass state to commands
struct CommandContext {
    SoundManager& soundManager;
    std::vector<types::AudioDeviceInfo>& availableDevices;
    std::vector<std::string>& samples;
    uint32_t& velocity;
    bool& running;
    int& selectedDeviceIndex;
    std::string& statusMessage;
    std::string& lastTriggeredSound;
};

// Command handler type
using CommandHandler = std::function<void(CommandContext&, const std::string&)>;

// UI Manager for rendering the interface
class UIManager {
public:
    static void clearScreen() { std::cout << "\033[2J\033[H" << std::flush; }

    static void renderUI(const CommandContext& ctx) {
        clearScreen();

        // Header
        std::cout << "╔════════════════════════════════════════════════════════════════╗\n";
        std::cout << "║              🎵 Raspberry Pi Sound System 🎵                   ║\n";
        std::cout << "╚════════════════════════════════════════════════════════════════╝\n\n";

        // Selected Device Info
        std::cout << "┌─ Audio Device ─────────────────────────────────────────────────┐\n";
        if (ctx.selectedDeviceIndex >= 0 && ctx.selectedDeviceIndex < static_cast<int>(ctx.availableDevices.size())) {
            const auto& device = ctx.availableDevices[ctx.selectedDeviceIndex];
            std::cout << "│ ✓ " << device.description << "│\n";
            std::cout << "│   Card: " << std::setw(3) << device.cardId << " Device: " << std::setw(3) << device.deviceId
                      << std::setw(40) << "" << "│\n";
        } else {
            std::cout << "│ ✗ No device selected" << std::setw(42) << "" << "│\n";
        }
        std::cout << "└────────────────────────────────────────────────────────────────┘\n\n";

        // Velocity Info
        std::cout << "┌─ Settings ─────────────────────────────────────────────────────┐\n";
        std::cout << "│ Velocity: " << std::setw(52) << ctx.velocity << "│\n";
        std::cout << "└────────────────────────────────────────────────────────────────┘\n\n";

        // Available Samples
        std::cout << "┌─ Available Samples ────────────────────────────────────────────┐\n";
        int maxSamplesToShow = 8;
        for (int i = 0; i < std::min(maxSamplesToShow, static_cast<int>(ctx.samples.size())); ++i) {
            std::string marker = (ctx.samples[i] == ctx.lastTriggeredSound) ? "♪" : " ";
            std::cout << "│ " << marker << " [" << i << "] " << std::left << std::setw(54) << ctx.samples[i] << "│\n";
        }
        if (ctx.samples.size() > maxSamplesToShow) {
            std::cout << "│   ... and " << (ctx.samples.size() - maxSamplesToShow) << " more (use 'l' to list all)"
                      << std::setw(24) << "" << "│\n";
        }
        std::cout << "└────────────────────────────────────────────────────────────────┘\n\n";

        // Status Message
        std::cout << "┌─ Status ───────────────────────────────────────────────────────┐\n";
        std::cout << "│ " << std::left << std::setw(61) << ctx.statusMessage << "│\n";
        std::cout << "└────────────────────────────────────────────────────────────────┘\n\n";

        // Commands
        std::cout << "┌─ Commands ─────────────────────────────────────────────────────┐\n";
        std::cout << "│ [0-9] Trigger sample  │ [l] List all    │ [d] Change device  │\n";
        std::cout << "│ [v] Set velocity      │ [h] Help        │ [q] Quit           │\n";
        std::cout << "└────────────────────────────────────────────────────────────────┘\n";

        std::cout << "\n> ";
        std::cout.flush();
    }
};

// Command registry
class CommandRegistry {
public:
    struct Command {
        std::vector<std::string> aliases;
        std::string description;
        CommandHandler handler;
    };

    void registerCommand(const std::vector<std::string>& aliases,
                         const std::string& description,
                         CommandHandler handler) {
        Command cmd{aliases, description, handler};
        for (const auto& alias : aliases) {
            m_commands[alias] = cmd;
        }
    }

    bool execute(const std::string& input, CommandContext& context) const {
        auto it = m_commands.find(input);
        if (it != m_commands.end()) {
            it->second.handler(context, input);
            return true;
        }
        return false;
    }

    void printHelp(const CommandContext& ctx) const {
        UIManager::clearScreen();
        std::cout << "╔════════════════════════════════════════════════════════════════╗\n";
        std::cout << "║                          Help Menu                             ║\n";
        std::cout << "╚════════════════════════════════════════════════════════════════╝\n\n";

        std::unordered_map<std::string, const Command*> uniqueCommands;
        for (const auto& [alias, cmd] : m_commands) {
            if (uniqueCommands.find(cmd.aliases[0]) == uniqueCommands.end()) {
                uniqueCommands[cmd.aliases[0]] = &cmd;
            }
        }

        for (const auto& [_, cmd] : uniqueCommands) {
            std::cout << "  ";
            for (size_t i = 0; i < cmd->aliases.size(); ++i) {
                std::cout << cmd->aliases[i];
                if (i < cmd->aliases.size() - 1)
                    std::cout << ", ";
            }
            std::cout << " - " << cmd->description << "\n";
        }

        std::cout << "\n  [0-9...] - Trigger sound sample by number\n";
        std::cout << "\nPress Enter to continue...";
        std::cin.ignore();
        std::cin.get();
    }

private:
    std::unordered_map<std::string, Command> m_commands;
};

void showDeviceSelectionMenu(const CommandContext& ctx) {
    UIManager::clearScreen();
    std::cout << "╔════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║                     Select Audio Device                        ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════════╝\n\n";

    for (size_t i = 0; i < ctx.availableDevices.size(); ++i) {
        std::string marker = (i == static_cast<size_t>(ctx.selectedDeviceIndex)) ? "✓" : " ";
        std::cout << " " << marker << " [" << i << "] " << ctx.availableDevices[i].description
                  << " (Card: " << ctx.availableDevices[i].cardId << ", Device: " << ctx.availableDevices[i].deviceId
                  << ")\n";
    }

    std::cout << "\nEnter device number: ";
}

void showSampleList(const CommandContext& ctx) {
    UIManager::clearScreen();
    std::cout << "╔════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║                     Available Samples                          ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════════╝\n\n";

    for (size_t i = 0; i < ctx.samples.size(); ++i) {
        std::string marker = (ctx.samples[i] == ctx.lastTriggeredSound) ? "♪" : " ";
        std::cout << " " << marker << " [" << i << "] " << ctx.samples[i] << "\n";
    }

    std::cout << "\nPress Enter to continue...";
    std::cin.ignore();
    std::cin.get();
}

CommandRegistry setupCommands() {
    CommandRegistry registry;

    // Quit command
    registry.registerCommand({"q", "quit", "exit"}, "Quit the program", [](CommandContext& ctx, const std::string&) {
        ctx.running = false;
    });

    // List samples command
    registry.registerCommand({"l", "list"}, "List all available samples", [](CommandContext& ctx, const std::string&) {
        showSampleList(ctx);
    });

    // List devices command
    registry.registerCommand(
        {"d", "devices"}, "List and select audio device", [](CommandContext& ctx, const std::string&) {
            showDeviceSelectionMenu(ctx);

            int selection;

            if (!(std::cin >> selection) ||
                (selection < 0 || selection >= static_cast<int>(ctx.availableDevices.size()))) {
                std::cin.clear();
                ctx.statusMessage = "❌ Invalid device selection";
                return;
            }

            if (ctx.soundManager.selectAudioDevice(ctx.availableDevices[selection])) {
                ctx.selectedDeviceIndex = selection;
                ctx.statusMessage = "✓ Device selected: " + ctx.availableDevices[selection].description;
            } else {
                ctx.statusMessage = "❌ Failed to select device";
            }
        });

    // Help command
    registry.registerCommand({"h", "help", "?"},
                             "Show this help message",
                             [&registry](CommandContext& ctx, const std::string&) { registry.printHelp(ctx); });

    // Set velocity command
    registry.registerCommand({"v", "velocity"}, "Set velocity (0-127)", [](CommandContext& ctx, const std::string&) {
        UIManager::clearScreen();
        std::cout << "Current velocity: " << ctx.velocity << "\n";
        std::cout << "Enter new velocity (0-127): ";

        int newVelocity;
        std::cin >> newVelocity;

        if (newVelocity >= 0 && newVelocity <= 127) {
            ctx.velocity = newVelocity;
            ctx.statusMessage = "✓ Velocity set to: " + std::to_string(ctx.velocity);
        } else {
            ctx.statusMessage = "❌ Invalid velocity (must be 0-127)";
        }
    });

    return registry;
}

int main(int argc, char* argv[]) {
    argparse::ArgumentParser program("RpiSound", "1.0");

    program.add_argument("--instrument", "-i")
        .help("Instrument folder to load samples from")
        .default_value(std::string("demo"));

    program.add_argument("--velocity", "-v")
        .help("Default velocity for triggering sounds (0-127)")
        .default_value(100U)
        .scan<'u', uint32_t>();

    program.add_argument("--log-level", "-l")
        .help("Set log level (trace, debug, info, warn, error)")
        .default_value(std::string("info"));

    try {
        program.parse_args(argc, argv);
    } catch (const std::exception& err) {
        spdlog::error("Error parsing command line arguments. Error: {}", err.what());
        return 1;
    }

    // Set log level
    std::string logLevel = program.get<std::string>("--log-level");
    if (logLevel == "trace") {
        spdlog::set_level(spdlog::level::trace);
    } else if (logLevel == "debug") {
        spdlog::set_level(spdlog::level::debug);
    } else if (logLevel == "info") {
        spdlog::set_level(spdlog::level::info);
    } else if (logLevel == "warn") {
        spdlog::set_level(spdlog::level::warn);
    } else if (logLevel == "error") {
        spdlog::set_level(spdlog::level::err);
    }

    // Initialize the ALSA driver
    AlsaDriver alsaDriver;
    AlsaDeviceEnumerator alsaEnumerator;
    AudioDeviceFactory deviceFactory;

    auto audioDeviceManager = AudioDeviceManager(deviceFactory, alsaEnumerator, alsaDriver);

    if (!audioDeviceManager.isInitialized()) {
        std::cerr << "Failed to initialize Audio Device Manager." << std::endl;
        return -1;
    }

    SoundManager soundManager(std::make_unique<PcmLoader>(), audioDeviceManager);

    if (!soundManager.initialize()) {
        std::cerr << "Failed to initialize Sound Manager." << std::endl;
        return -1;
    }

    auto availableDevices = soundManager.getAvailableAudioDevices();
    if (availableDevices.empty()) {
        std::cerr << "No audio devices available." << std::endl;
        return -1;
    }

    if (!soundManager.load(program.get<std::string>("--instrument"))) {
        std::cerr << "Failed to load instrument samples." << std::endl;
        return -1;
    }

    auto samples = soundManager.getAvailableSamples();
    uint32_t velocity = program.get<uint32_t>("--velocity");
    bool running = true;
    int selectedDeviceIndex = -1;
    std::string statusMessage = "Ready";
    std::string lastTriggeredSound = "";

    CommandRegistry commandRegistry = setupCommands();
    CommandContext context{soundManager,
                           availableDevices,
                           samples,
                           velocity,
                           running,
                           selectedDeviceIndex,
                           statusMessage,
                           lastTriggeredSound};

    std::string input = "d";  // Start with device selection
    while (running) {
        UIManager::renderUI(context);

        // Try to execute as a command first
        if (commandRegistry.execute(input, context)) {
            input = "";
            continue;
        }

        // Clear input buffer
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

        std::cin >> input;

        // Otherwise, try to parse as sample number
        try {
            int sampleIndex = std::stoi(input);

            if (sampleIndex < 0 || sampleIndex >= static_cast<int>(samples.size())) {
                context.statusMessage = "❌ Invalid sample number";
                continue;
            }

            const auto& sampleName = samples[sampleIndex];

            if (soundManager.triggerSound(sampleName, velocity)) {
                context.lastTriggeredSound = sampleName;
                context.statusMessage = "♪ Playing: " + sampleName + " (velocity: " + std::to_string(velocity) + ")";
            } else {
                context.statusMessage = "❌ Failed to trigger: " + sampleName;
            }

        } catch (const std::exception& e) {
            context.statusMessage = "❌ Invalid input - use number or command";
        }
    }

    UIManager::clearScreen();
    std::cout << "Rpi Sound terminated.\n";
    return 0;
}