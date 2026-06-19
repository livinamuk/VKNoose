#include "Util.h"

namespace Util {
    std::vector<FileInfo> IterateDirectory(const std::string& directory, std::vector<std::string> extensions) {
        std::vector<FileInfo> fileInfoList;
        if (!std::filesystem::exists(directory)) return fileInfoList;

        for (const auto& entry : std::filesystem::directory_iterator(directory)) {
            if (!std::filesystem::is_regular_file(entry)) continue;

            FileInfo fileInfo = { GetFullPath(entry), GetFileNameWithoutExtension(entry), GetFileExtension(entry), directory };

            if (extensions.empty() || std::find(extensions.begin(), extensions.end(), fileInfo.ext) != extensions.end()) {
                fileInfoList.push_back(fileInfo);
            }
        }
        return fileInfoList;
    }

    std::string GetFullPath(const std::filesystem::directory_entry& entry) {
        return entry.path().string();
    }

    std::string GetFileName(const std::filesystem::directory_entry& entry) {
        return entry.path().filename().string();
    }

    std::string GetFileNameWithoutExtension(const std::filesystem::directory_entry& entry) {
        return entry.path().stem().string();
    }

    std::string GetFileExtension(const std::filesystem::directory_entry& entry) {
        return entry.path().extension().string().substr(1);
    }
}