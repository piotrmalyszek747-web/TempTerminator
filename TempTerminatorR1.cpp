#include <iostream>
#include <filesystem>
#include <fstream>
#include <chrono>
#include <thread>

namespace fs = std::filesystem;

// Returns a timestamp string for logs
std::string timestamp() {
    auto now = std::chrono::system_clock::now();
    std::time_t t_now = std::chrono::system_clock::to_time_t(now);
    std::tm tm_now;
    localtime_s(&tm_now, &t_now);
    char buf[20];
    std::strftime(buf, sizeof(buf), "[%Y-%m-%d %H:%M:%S]", &tm_now);
    return std::string(buf);
}

int main() {
    try {
        // Paths
        std::string tempPath = std::getenv("TEMP");
        std::string exeFolder = fs::current_path().string();
        std::string logFile = exeFolder + "\\TempTerminator_Log.txt";

        // Open log file
        std::ofstream log(logFile, std::ios::out | std::ios::app);
        log << timestamp() << " === Starting Temp Termination ===\n";

        // Startup message
        std::cout << "=== TempTerminatorBETAV2 ===\n";
        std::cout << "This tool will safely clean your Windows TEMP folder.\n";
        std::cout << "- Files/folders in use or protected will be skipped (and logged).\n";
        std::cout << "- Detailed log will be saved here: " << logFile << "\n";
        std::cout << "- If the program crashes, a Crashlog.txt will be created on your Desktop.\n";
        std::cout << "\nPress Enter to start the cleanup (or Ctrl+C to cancel) ...";
        std::cin.get();

        // Safety check
        if (!fs::exists(tempPath)) {
            std::cout << "Error 404 - Temp folder not found: " << tempPath << std::endl;
            log << timestamp() << " Error 404 - Temp folder not found: " << tempPath << std::endl;
            return 1;
        }

        std::cout << "\nPreparing to delete files...\n" << std::endl;
        size_t filesDeleted = 0, filesSkipped = 0, foldersDeleted = 0, foldersSkipped = 0;

        // Delete files
        for (const auto& entry : fs::directory_iterator(tempPath)) {
            try {
                if (fs::is_regular_file(entry.path())) {
                    fs::remove(entry.path());
                    log << timestamp() << " Deleted file: " << entry.path() << std::endl;
                    filesDeleted++;
                }
            } catch (const fs::filesystem_error& e) {
                log << timestamp() << " Skipped file (cannot delete): " << entry.path()
                    << " | Reason: " << e.what() << std::endl;
                filesSkipped++;
            }
        }

        std::cout << "Files pass finished.\n\nPreparing to delete subfolders...\n" << std::endl;

        // Delete folders
        for (const auto& entry : fs::directory_iterator(tempPath)) {
            try {
                if (fs::is_directory(entry.path())) {
                    size_t items = std::distance(fs::recursive_directory_iterator(entry), fs::recursive_directory_iterator{});
                    fs::remove_all(entry.path());
                    log << timestamp() << " Deleted folder: " << entry.path() << " (items removed: " << items << ")\n";
                    foldersDeleted++;
                }
            } catch (const fs::filesystem_error& e) {
                log << timestamp() << " Skipped folder (cannot delete): " << entry.path()
                    << " | Reason: " << e.what() << std::endl;
                foldersSkipped++;
            }
        }

        // Summary
        log << "\n--- Summary ---\n";
        log << "Files deleted: " << filesDeleted << "\n";
        log << "Files skipped: " << filesSkipped << "\n";
        log << "Folders deleted: " << foldersDeleted << "\n";
        log << "Folders skipped: " << foldersSkipped << "\n\n";
        log.close();

        std::cout << "--- Summary ---\n";
        std::cout << "Files deleted: " << filesDeleted << "\n";
        std::cout << "Files skipped: " << filesSkipped << "\n";
        std::cout << "Folders deleted: " << foldersDeleted << "\n";
        std::cout << "Folders skipped: " << foldersSkipped << "\n\n";
        std::cout << "Temp folder cleanup finished. Detailed log saved at: " << logFile << std::endl;

        std::cout << "\nPress Enter to exit...";
        std::cin.get();

    } catch (const std::exception& e) {
        // Crash log
        std::string desktop = std::getenv("USERPROFILE");
        desktop += "\\Desktop\\Crashlog.txt";
        std::ofstream crash(desktop, std::ios::app);
        crash << timestamp() << " TempTerminatorBETAV2 crashed\n";
        crash << "Exception: " << e.what() << "\n-----------------------------\n";
        crash.close();
        std::cout << "Oops! A crash occurred. Details logged to Desktop\\Crashlog.txt" << std::endl;
        return 1;
    }

    return 0;
}
