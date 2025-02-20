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

#pragma once

#include <string>
#include <vector>

struct Setting {
    std::string name;
    void *value;
    bool isString;

    Setting(std::string name, void *value, bool isString):
        name(name), value(value), isString(isString) {}
};

namespace Settings {
    extern std::string boot11Path;
    extern std::string boot9Path;
    extern std::string nandPath;
    extern std::string sdPath;
    extern std::string basePath;

    void add(std::vector<Setting> &extra);
    bool load(std::string path = ".");
    bool save();
};
