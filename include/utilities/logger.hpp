#pragma once

#include <fmt/chrono.h>
#include <fmt/format.h>
#include <atomic>
#include <condition_variable>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <mutex>
#include <queue>
#include <sstream>
#include <streambuf>
#include <string>
#include <thread>

namespace logging {

enum class Level { kDebug, kInfo, kWarning, kError };

inline const char* to_string(Level lvl) noexcept {
    switch (lvl) {
        case Level::kDebug:
            return "DEBUG:  ";
        case Level::kInfo:
            return "INFO:   ";
        case Level::kWarning:
            return "WARNING:";
        case Level::kError:
            return "ERROR:  ";
    }
    return "UNKNOWN";
}

struct LogMessage {
    Level level;
    std::string text;
    std::chrono::system_clock::time_point timestamp;
};

class Logger {
public:
    struct Config {
        bool toStdOut = true;
        bool toFile = false;
        std::string filename = "log.txt";
        std::size_t maxFileSize = 5 * 1024 * 1024;
        Level level = Level::kDebug;
    };

    static Logger& getInstance(const Config& cfg) {
        static std::once_flag initialized;
        static std::unique_ptr<Logger> instance;

        std::call_once(initialized, [&cfg]() { instance = std::unique_ptr<Logger>(new Logger(cfg)); });
        return *instance;
    }

    static Logger& getInstance() {
        static Config default_cfg;
        return getInstance(default_cfg);
    }

    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    template <typename... Args>
    void log(Level lvl, std::string_view fmt_str, Args&&... args) {
        if (lvl < m_config.level)
            return;
        auto msg = fmt::format(fmt::runtime(fmt_str), std::forward<Args>(args)...);
        enqueue({lvl, std::move(msg), std::chrono::system_clock::now()});
    }

    void debug(std::string_view fmt_str) { log(Level::kDebug, fmt_str); }
    template <typename... Args>
    void debug(std::string_view fmt_str, Args&&... args) {
        log(Level::kDebug, fmt_str, std::forward<Args>(args)...);
    }
    void info(std::string_view fmt_str) { log(Level::kInfo, fmt_str); }
    template <typename... Args>
    void info(std::string_view fmt_str, Args&&... args) {
        log(Level::kInfo, fmt_str, std::forward<Args>(args)...);
    }
    void warning(std::string_view fmt_str) { log(Level::kWarning, fmt_str); }
    template <typename... Args>
    void warning(std::string_view fmt_str, Args&&... args) {
        log(Level::kWarning, fmt_str, std::forward<Args>(args)...);
    }
    void error(std::string_view fmt_str) { log(Level::kError, fmt_str); }
    template <typename... Args>
    void error(std::string_view fmt_str, Args&&... args) {
        log(Level::kError, fmt_str, std::forward<Args>(args)...);
    }

    // Method to write directly without going through logger formatting
    void writeRaw(Level lvl, const std::string& text) {
        if (lvl < m_config.level)
            return;
        enqueue({lvl, text, std::chrono::system_clock::now()});
    }

    ~Logger() {
        {
            std::lock_guard<std::mutex> lock(m_queueMutex);
            m_exitFlag = true;
        }
        m_cv.notify_one();
        if (m_worker.joinable())
            m_worker.join();

        // Restore original stream buffers
        if (m_originalCoutBuf)
            std::cout.rdbuf(m_originalCoutBuf);
        if (m_originalCerrBuf)
            std::cerr.rdbuf(m_originalCerrBuf);
    }

private:
    explicit Logger(const Config& cfg) : m_config(cfg), m_exitFlag(false) {
        // Store original stream buffers BEFORE starting worker thread
        m_originalCoutBuf = std::cout.rdbuf();
        m_originalCerrBuf = std::cerr.rdbuf();

        if (m_config.toFile)
            open_new_file();
        m_worker = std::thread(&Logger::processQueue, this);
    }

    void enqueue(LogMessage msg) {
        {
            std::lock_guard<std::mutex> lock(m_queueMutex);
            m_queue.push(std::move(msg));
        }
        m_cv.notify_one();
    }

    void processQueue() {
        std::unique_lock<std::mutex> lock(m_queueMutex);
        while (!m_exitFlag || !m_queue.empty()) {
            m_cv.wait(lock, [&] { return m_exitFlag || !m_queue.empty(); });
            while (!m_queue.empty()) {
                auto msg = std::move(m_queue.front());
                m_queue.pop();
                lock.unlock();
                write(msg);
                lock.lock();
            }
        }
    }

    void write(const LogMessage& msg) {
        // timestamp
        auto timestamp = fmt::format("{:%Y-%m-%d %H:%M:%S}", msg.timestamp);
        auto line = fmt::format("\r[{}] {} {}\n", timestamp, to_string(msg.level), msg.text);

        if (m_config.toStdOut && m_originalCoutBuf) {
            const char* color = "\033[0m";
            switch (msg.level) {
                case Level::kError:
                    color = "\033[31m";
                    break;
                case Level::kWarning:
                    color = "\033[33m";
                    break;
                case Level::kInfo:
                    color = "\033[0m";
                    break;
                case Level::kDebug:
                    color = "\033[90m";
                    break;
            }
            // Write directly to original cout buffer to avoid recursion
            m_originalCoutBuf->sputn(color, std::strlen(color));
            m_originalCoutBuf->sputn(line.c_str(), line.length());
            m_originalCoutBuf->sputn("\033[0m", 4);
            m_originalCoutBuf->pubsync();
        }
        if (m_config.toFile && m_fileStream) {
            m_fileStream << line;
            m_fileStream.flush();
            rotate_if_needed(line.length());
        }
    }

    void open_new_file() {
        if (m_fileStream.is_open())
            m_fileStream.close();
        auto t0 = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        std::tm tm = *std::localtime(&t0);
        char buf[20];
        std::strftime(buf, sizeof(buf), "%Y%m%d_%H%M%S", &tm);

        auto path = std::filesystem::path(m_config.filename);
        auto stem = path.stem().string();
        auto ext = path.extension().string();
        auto name = fmt::format("{}_{:s}{}", stem, buf, ext);
        m_fileStream.open(name, std::ios::out | std::ios::trunc);
        m_currentSize = 0;
    }

    void rotate_if_needed(size_t bytesWritten) {
        m_currentSize += bytesWritten;
        if (m_currentSize >= m_config.maxFileSize)
            open_new_file();
    }

    Config m_config;
    std::atomic<bool> m_exitFlag;
    std::mutex m_queueMutex;
    std::condition_variable m_cv;
    std::queue<LogMessage> m_queue;
    std::thread m_worker;
    std::ofstream m_fileStream;
    std::size_t m_currentSize = 0;

    // Store original stream buffers to avoid recursion
    std::streambuf* m_originalCoutBuf = nullptr;
    std::streambuf* m_originalCerrBuf = nullptr;

    friend class LoggerStreamBuf;
};

// StreamBuf to forward std::cout/cerr to Logger
class LoggerStreamBuf : public std::streambuf {
public:
    explicit LoggerStreamBuf(Level lvl) : m_level(lvl) {}

protected:
    std::streamsize xsputn(const char* s, std::streamsize n) override {
        for (std::streamsize i = 0; i < n; ++i) {
            overflow(s[i]);
        }
        return n;
    }

    int overflow(int c) override {
        if (c == EOF) {
            return EOF;
        }

        char ch = static_cast<char>(c);

        if (ch == '\n') {
            // End of line - flush the buffer
            if (!m_buffer.empty()) {
                Logger::getInstance().writeRaw(m_level, m_buffer);
                m_buffer.clear();
            }
        } else if (ch != '\r') {
            // Accumulate characters (ignore carriage returns)
            m_buffer += ch;
        }

        return c;
    }

    int sync() override {
        // Flush any remaining buffer content without newline
        if (!m_buffer.empty()) {
            Logger::getInstance().writeRaw(m_level, m_buffer);
            m_buffer.clear();
        }
        return 0;
    }

private:
    Level m_level;
    std::string m_buffer;
};

inline void redirectStdOut() {
    static LoggerStreamBuf buf(Level::kInfo);
    std::cout.rdbuf(&buf);
}

inline void redirectStdErr() {
    static LoggerStreamBuf buf(Level::kError);
    std::cerr.rdbuf(&buf);
}

inline void redirectStdStreams() {
    redirectStdOut();
    redirectStdErr();
}

}  // namespace logging

namespace utilities {
inline logging::Logger& log = logging::Logger::getInstance();
}  // namespace utilities
