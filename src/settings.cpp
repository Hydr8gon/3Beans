/*
    Copyright 2023-2025 Hydr8gon

    This file is part of 3Beans.

    3Beans is free software: you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    3Beans is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with 3Beans. If not, see <https://www.gnu.org/licenses/>.
*/

#include <sys/stat.h>
#include "core.h"

namespace Settings {
    int fpsLimiter = 1;
    std::string boot11Path = "boot11.bin";
    std::string boot9Path = "boot9.bin";
    std::string nandPath = "nand.bin";
    std::string sdPath = "sd.img";
    std::string basePath = ".";

    std::vector<Setting> settings = {
        Setting("fpsLimiter", &fpsLimiter, false),
        Setting("boot11Path", &boot11Path, true),
        Setting("boot9Path", &boot9Path, true),
        Setting("nandPath", &nandPath, true),
        Setting("sdPath", &sdPath, true)
    };
}

void Settings::add(std::vector<Setting> &extra) {
    // Add additional settings to be loaded from the settings file
    settings.insert(settings.end(), extra.begin(), extra.end());
}

bool Settings::load(std::string path) {
    // Set the base path and ensure the folder exists
    mkdir((basePath = path).c_str() MKDIR_ARGS);

    // Open the settings file or set defaults if it doesn't exist
    FILE *file = fopen((basePath + "/3beans.ini").c_str(), "r");
    if (!file) {
        Settings::boot11Path = basePath + "/boot11.bin";
        Settings::boot9Path = basePath + "/boot9.bin";
        Settings::nandPath = basePath + "/nand.bin";
        Settings::sdPath = basePath + "/sd.img";
        Settings::save();
        return false;
    }

    // Read each line of the settings file and load values from them
    char data[512];
    while (fgets(data, 512, file) != nullptr) {
        std::string line = data;
        size_t split = line.find('=');
        std::string name = line.substr(0, split);
        for (int i = 0; i < settings.size(); i++) {
            if (name != settings[i].name) continue;
            std::string value = line.substr(split + 1, line.size() - split - 2);
            if (settings[i].isString)
                *(std::string*)settings[i].value = value;
            else if (value[0] >= '0' && value[0] <= '9')
                *(int*)settings[i].value = stoi(value);
            break;
        }
    }

    // Close the file after reading it
    fclose(file);
    return true;
}

bool Settings::save() {
    // Attempt to open the settings file
    FILE *file = fopen((basePath + "/3beans.ini").c_str(), "w");
    if (!file) return false;

    // Write each setting to the settings file
    for (int i = 0; i < settings.size(); i++) {
        std::string value = settings[i].isString ? *(std::string*)settings[i].value
            : std::to_string(*(int*)settings[i].value);
        fprintf(file, "%s=%s\n", settings[i].name.c_str(), value.c_str());
    }

    // Close the file after writing it
    fclose(file);
    return true;
}
