// TempTerminator - single-file implementation with:
// - startup explanation & user confirmation
// - safe deletion of files & folders from %TEMP%
// - skips undeletable items (logs reason)
// - detailed operation log in the folder where the .exe runs (TempTerminator_Log.txt)
// - crash logging to Desktop\Crashlog.txt
// - animated progress dots, summary counts
// Build: g++ -std=c++17 -O2 -static -s -o TempTerminator.exe TempTerminator.cpp
// (If static build fails, remove -static)

#include <windows.h>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <chrono>
#include <thread>
#include <ctime>
#include <iomanip>
#include <sstream>

namespace fs = std::filesystem;

static std::string nowTimestamp() {
    auto t = std::time(nullptr);
    std::ostringstream ss;
    ss << std::put_time(std::localtime(&t), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

static std::string getEnvVar(const char* name) {
    const char* v = std::getenv(name);
    return v ? std::string(v) : std::string();
}

// Get the directory where the running exe is located
static std::string getExeFolder() {
    char path[MAX_PATH];
    DWORD len = GetModuleFileNameA(NULL, path, MAX_PATH);
    if (len == 0 || len == MAX_PATH) {
        // fallback to current path
        return fs::current_path().string();
    }
    std::string p(path, path + len);
    size_t pos = p.find_last_of("\\/");
    if (pos == std::string::npos) return ".";
    return p.substr(0, pos);
}

static void animateDots(const std::string& message, int count = 3, int delayMs = 300) {
    std::cout << message;
    for (int i = 0; i < count; ++i) {
        std::cout << ".";
        std::cout.flush();
        std::this_thread::sleep_for(std::chrono::milliseconds(delayMs));
    }
    std::cout << "\n\n";
}

int main() {
    try {
        // Intro / friendly explanation
        std::cout << "=== TempTerminator ===\n\n";
        std::cout << "This tool will safely clean your Windows TEMP folder.\n";
        std::cout << "- It attempts to delete temporary files and folders.\n";
        std::cout << "- Files or folders that are in use or protected will be skipped (and logged).\n";
        std::cout << "- A detailed log of operations will be saved next to this program (TempTerminator_Log.txt).\n";
        std::cout << "- If the program crashes, a Crashlog.txt will be created on your Desktop.\n\n";
        std::cout << "Press Enter to start the cleanup (or Ctrl+C to cancel) ...";
        std::cin.get();

        // Resolve paths
        std::string tempPath = getEnvVar("TEMP");
        std::string exeFolder = getExeFolder();
        std::string logPath = exeFolder + "\\TempTerminator_Log.txt";

        // Open main log file (append so previous logs are preserved)
        std::ofstream log(logPath, std::ios::out | std::ios::app);
        if (!log.is_open()) {
            std::cerr << "Warning: cannot open log file at " << logPath << "\nProceeding without file log.\n";
        }

        auto logWrite = [&](const std::string& s) {
            std::string line = "[" + nowTimestamp() + "] " + s;
            std::cout << line << std::endl;
            if (log.is_open()) {
                log << line << std::endl;
            }
        };

        // Header for this run
        logWrite("=== Starting Temp Termination ===");

        if (tempPath.empty()) {
            logWrite("Error: %TEMP% environment variable not found. Aborting.");
            if (log.is_open()) log.close();
            return 1;
        }

        if (!fs::exists(tempPath)) {
            logWrite(std::string("Error: Temp folder not found: ") + tempPath);
            if (log.is_open()) log.close();
            return 1;
        }

        // Animated intro
        std::cout << "\nPreparing to delete files...\n\n";
        animateDots("Working");

        // Counters
        size_t filesDeleted = 0, filesSkipped = 0;
        size_t foldersDeleted = 0, foldersSkipped = 0;

        // First pass: delete regular files in the temp root
        for (const auto& entry : fs::directory_iterator(tempPath)) {
            try {
                if (fs::is_regular_file(entry.path())) {
                    bool removed = fs::remove(entry.path());
                    if (removed) {
                        logWrite(std::string("Deleted file: ") + entry.path().string());
                        ++filesDeleted;
                    } else {
                        logWrite(std::string("Could not delete file (unknown reason): ") + entry.path().string());
                        ++filesSkipped;
                    }
                }
            } catch (const fs::filesystem_error& e) {
                logWrite(std::string("Skipped file (cannot delete): ") + entry.path().string() + " | Reason: " + e.what());
                ++filesSkipped;
            } catch (const std::exception& e) {
                logWrite(std::string("Skipped file (unexpected error): ") + entry.path().string() + " | Reason: " + e.what());
                ++filesSkipped;
            }
        }

        std::cout << "\nFiles pass finished.\n\n";
        std::cout << "Preparing to delete subfolders...\n\n";
        animateDots("Working");

        // Second pass: delete directories (use remove_all, skip on errors)
        for (const auto& entry : fs::directory_iterator(tempPath)) {
            try {
                if (fs::is_directory(entry.path())) {
                    // attempt remove_all
                    std::uintmax_t removedCount = fs::remove_all(entry.path()); // returns number removed
                    if (removedCount > 0) {
                        logWrite(std::string("Deleted folder: ") + entry.path().string() + " (items removed: " + std::to_string(removedCount) + ")");
                        ++foldersDeleted;
                    } else {
                        logWrite(std::string("Could not delete folder (unknown reason): ") + entry.path().string());
                        ++foldersSkipped;
                    }
                }
            } catch (const fs::filesystem_error& e) {
                logWrite(std::string("Skipped folder (cannot delete): ") + entry.path().string() + " | Reason: " + e.what());
                ++foldersSkipped;
            } catch (const std::exception& e) {
                logWrite(std::string("Skipped folder (unexpected error): ") + entry.path().string() + " | Reason: " + e.what());
                ++foldersSkipped;
            }
        }

        // Summary
        if (log.is_open()) {
            log << "\n--- Summary (" << nowTimestamp() << ") ---\n";
            log << "Files deleted: " << filesDeleted << "\n";
            log << "Files skipped: " << filesSkipped << "\n";
            log << "Folders deleted: " << foldersDeleted << "\n";
            log << "Folders skipped: " << foldersSkipped << "\n";
            log << "=== End ===\n\n";
            log.flush();
            log.close();
        }

        // Also echo summary to console
        std::cout << "\n--- Summary ---\n";
        std::cout << "Files deleted: " << filesDeleted << "\n";
        std::cout << "Files skipped: " << filesSkipped << "\n";
        std::cout << "Folders deleted: " << foldersDeleted << "\n";
        std::cout << "Folders skipped: " << foldersSkipped << "\n\n";

        std::cout << "Temp folder cleanup finished. Detailed log saved at: " << logPath << "\n";

        std::cout << "\nPress Enter to exit...";
        std::cin.get();

        return 0;
    }
    catch (const std::exception& e) {
        // Write crash info to Desktop\Crashlog.txt
        try {
            std::string userProfile = getEnvVar("USERPROFILE");
            std::string crashPath;
            if (!userProfile.empty()) crashPath = userProfile + "\\Desktop\\Crashlog.txt";
            else crashPath = "Crashlog.txt"; // fallback

            std::ofstream crash(crashPath, std::ios::out | std::ios::app);
            if (crash.is_open()) {
                crash << "=== Crash at " << nowTimestamp() << " ===\n";
                crash << "Exception: " << e.what() << "\n\n";
                crash.close();
            }
        } catch (...) {
            // If crash logging fails, nothing more we can do
        }

        std::cerr << "\nTempTerminator encountered an unexpected error and must exit.\n";
        std::cerr << "A crash log has been written to your Desktop (Crashlog.txt) if possible.\n";
        std::cerr << "Press Enter to exit...";
        std::cin.get();
        return 1;
    }
}
