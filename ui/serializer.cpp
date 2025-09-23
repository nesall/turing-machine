#include "ui/serializer.hpp"
#include "app.hpp"

#include <fstream>
#include <filesystem>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <iostream>

#include "imgui.h"

using json = nlohmann::json;

namespace {
  AppState::Menu stringToMode(const std::string &str) {
    if (str == "add_state") return AppState::Menu::ADD_STATE;
    if (str == "add_transition") return AppState::Menu::ADD_TRANSITION;
    return AppState::Menu::SELECT;
  }

  std::string modeToString(AppState::Menu mode) {
    switch (mode) {
    case AppState::Menu::SELECT: return "select";
    case AppState::Menu::ADD_STATE: return "add_state";
    case AppState::Menu::ADD_TRANSITION: return "add_transition";
    default: return "select";
    }
  }


} // anonymous namespace

json AppSerializer::serialize(const AppState &appState)
{
  json j;

  // Metadata
  j["version"] = "1.0";
  j["created"] = getCurrentTimestamp();

  // Use existing TuringMachine serialization
  j["turingMachine"] = json::parse(appState.tm().toJson());

  // Use existing Tape serialization
  j["tape"] = json::parse(appState.tm().tape().toJson());

  // UI state - positions and layout
  j["ui"] = json::object();
  j["ui"]["mode"] = modeToString(appState.menu());

  // State positions
  j["ui"]["statePositions"] = json::object();
  for (const auto &state : appState.tm().states()) {
    ImVec2 pos = appState.statePosition(state);
    j["ui"]["statePositions"][state.name()] = {
        {"x", pos.x}, {"y", pos.y}
    };
  }

  return j;
}

bool AppSerializer::deserialize(const json &j, AppState &appState)
{
  try {
    // Clear current state
    appState = AppState();

    // Load TuringMachine using existing fromJson
    if (j.contains("turingMachine")) {
      appState.tm().fromJson(j["turingMachine"].dump());
    }

    // Load Tape using existing fromJson
    if (j.contains("tape")) {
      appState.tm().tape().fromJson(j["tape"].dump());
    }

    // Rebuild UI state
    rebuildDrawObjects(appState);

    // Load UI settings
    if (j.contains("ui")) {
      const auto &ui = j["ui"];

      // Load mode
      if (ui.contains("mode")) {
        appState.setMenu(stringToMode(ui["mode"]));
      }

      // Load state positions
      if (ui.contains("statePositions")) {
        for (const auto &[stateName, posJson] : ui["statePositions"].items()) {
          for (const auto &state : appState.tm().states()) {
            if (state.name() == stateName) {
              ImVec2 pos{ posJson["x"], posJson["y"] };
              appState.setStatePosition(state, pos);
              break;
            }
          }
        }
      }
    }

    return true;
  } catch (const std::exception &e) {
    std::cerr << "Load error: " << e.what() << std::endl;
    return false;
  }
}

bool AppSerializer::saveToFile(const AppState &appState, const std::string &filename)
{
  std::string filepath = filename;
  if (filepath.empty()) {
    filepath = "machine_" + getCurrentTimestamp() + ".json";
  }

  try {
    json j = serialize(appState);
    std::ofstream file(filepath);
    file << j.dump(2);
    std::cout << "Saved to: " << filepath << std::endl;
    return true;
  } catch (const std::exception &e) {
    std::cerr << "Save error: " << e.what() << std::endl;
    return false;
  }
}

bool AppSerializer::loadFromFile(AppState &appState, const std::string &filename)
{
  std::string filepath = filename;

  if (filepath.empty()) {
    filepath = findMostRecentFile();
    if (filepath.empty()) {
      std::cerr << "No saved files found" << std::endl;
      return false;
    }
  }

  if (!std::filesystem::exists(filepath)) {
    std::cerr << "File not found: " << filepath << std::endl;
    return false;
  }

  try {
    std::ifstream file(filepath);
    json j;
    file >> j;
    bool success = deserialize(j, appState);
    if (success) {
      std::cout << "Loaded from: " << filepath << std::endl;
    }
    return success;
  } catch (const std::exception &e) {
    std::cerr << "Load error: " << e.what() << std::endl;
    return false;
  }
}

std::vector<std::string> AppSerializer::getSavedFiles()
{
  std::vector<std::string> files;
  try {
    for (const auto &entry : std::filesystem::directory_iterator(".")) {
      if (entry.path().extension() == ".json" &&
        entry.path().filename().string().find("machine_") == 0) {
        files.push_back(entry.path().filename().string());
      }
    }
    std::sort(files.begin(), files.end(), [](const std::string &a, const std::string &b) {
      return std::filesystem::last_write_time(a) > std::filesystem::last_write_time(b);
      });
  } catch (const std::exception &e) {
    std::cerr << "Error listing files: " << e.what() << std::endl;
  }
  return files;
}

void AppSerializer::rebuildDrawObjects(AppState &appState)
{
  appState.clearManipulators();

  for (const auto &state : appState.tm().states()) {
    appState.addState(state, ImVec2(100, 100));
  }

  for (const auto &transition : appState.tm().transitions()) {
    appState.addTransition(transition);
  }
}

std::string AppSerializer::getCurrentTimestamp()
{
  auto now = std::chrono::system_clock::now();
  auto time_t = std::chrono::system_clock::to_time_t(now);
  std::stringstream ss;
  ss << std::put_time(std::localtime(&time_t), "%Y%m%d_%H%M%S");
  return ss.str();
}

std::string AppSerializer::findMostRecentFile()
{
  auto files = getSavedFiles();
  return files.empty() ? "" : files[0];
}
