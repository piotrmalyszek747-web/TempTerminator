#include <iostream>
#include <filesystem>
#include <fstream>
#include <chrono>
#include <thread>

namespace fs = std::filesystem;

int main() {
    std::string tempPath = std::getenv("TEMP");
    std::string logFile = tempPath + "\\temp_cleanup_log.txt";

    std::ofstream log(logFile, std::ios::out);
    log << "Starting Temp Termination at " << __DATE__ << " " << __TIME__ << std::endl;

    // Safety check
    if (!fs::exists(tempPath)) {
        std::cout << "Error 404-Did not found the folder. Something is off: " << tempPath << std::endl;
        log << "Error 404-Did not found the folder. Something is off: " << tempPath << std::endl;
        return 1;
    }

    std::cout << "\nPreparing to delete files...\n" << std::endl;

    // Delete files
    for (const auto& entry : fs::directory_iterator(tempPath)) {
        if (fs::is_regular_file(entry.path())) {
            fs::remove(entry.path());
            log << "Deleted file: " << entry.path() << std::endl;
        }
    }

    std::cout << "Files deleted.\n" << std::endl;

    std::cout << "Preparing to delete subfolders...\n" << std::endl;

    // Delete subfolders
    for (const auto& entry : fs::directory_iterator(tempPath)) {
        if (fs::is_directory(entry.path())) {
            fs::remove_all(entry.path());
            log << "Deleted folder: " << entry.path() << std::endl;
        }
    }

    std::cout << "Subfolders deleted.\n" << std::endl;

    std::cout << "Temp folder cleaned up! All trash terminated! Log saved at " << logFile << std::endl;
    log << "Temp folder cleaned up! All trash terminated!" << std::endl;

    log.close();

    // Optional pause (like batch)
    std::cout << "\nPress Enter to exit...";
    std::cin.get();

    return 0;
}
