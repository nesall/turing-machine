#include "ui/serializer.hpp"
#include "ui/drawobject.hpp"
#include "model/turingmachine.hpp"
#include "app.hpp"

#include <fstream>
#include <filesystem>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <iostream>

#include <imgui.h>
#include "ui/imfilebrowser.h"


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

  ui::TransitionDrawObject *findTransitionDrawObject(AppState &appState, const std::string &key) {
    for (size_t j = 0; j < appState.nofDrawObjects(); j ++) {
      auto obj = appState.getDrawObject(j);
      if (auto transObj = obj->asTransition()) {
        auto tk = transObj->getTransition().uniqueKey();
        if (tk == key) {
          return const_cast<ui::TransitionDrawObject *>(transObj);
        }
      }
    }
    return nullptr;
  }

  ui::TransitionLabelDrawObject *findTransitionLabelDrawObject(AppState &appState, const std::string &key) {
    for (size_t j = 0; j < appState.nofDrawObjects(); j ++) {
      auto obj = appState.getDrawObject(j);
      if (auto labelObj = obj->asTransitionLabel(); labelObj && labelObj->transitionDrawObject()) {
        auto tk = labelObj->transitionDrawObject()->getTransition().uniqueKey();
        if (tk == key) {
          return const_cast<ui::TransitionLabelDrawObject *>(labelObj);
        }
      }
    }
    return nullptr;
  }

} // anonymous namespace

json AppSerializer::serialize(const AppState &appState)
{
  json j;
  j["version"] = "1.0";
  j["created"] = getCurrentTimestamp();
  j["turingMachine"] = appState.tm().toJson();
  j["tape"] = appState.tm().tape().toJson();
  j["ui"] = json::object();
  j["ui"]["mode"] = modeToString(appState.menu());
  j["ui"]["statePositions"] = json::object();
  for (const auto &state : appState.tm().states()) {
    ImVec2 pos = appState.statePosition(state) + appState.scrollXY() - appState.canvasOrigin();
    std::string name = state.name();
    j["ui"]["statePositions"][name] = { {"x", pos.x}, {"y", pos.y} };
  }
  j["ui"]["transitionStyles"] = json::object();
  j["ui"]["transitionLabels"] = json::object();
  for (size_t i = 0; i < appState.nofDrawObjects(); i ++) {
    auto obj = appState.getDrawObject(i);
    if (auto tr = obj->asTransition()) {
      std::string transKey = tr->getTransition().uniqueKey();
      j["ui"]["transitionStyles"][transKey] = tr->toJson();
    } else if (auto lb = obj->asTransitionLabel()) {
      std::string transKey = lb->transitionDrawObject()->getTransition().uniqueKey();
      j["ui"]["transitionLabels"][transKey] = lb->toJson();
    }
  }
  return j;
}

bool AppSerializer::deserialize(const json &j, AppState &appState)
{
  try {
    appState.reset();
    if (j.contains("turingMachine")) {
      appState.tm().fromJson(j["turingMachine"]);
    }
    if (j.contains("tape")) {
      appState.tm().tape().fromJson(j["tape"]);
    }
    rebuildDrawObjects(appState);
    if (j.contains("ui")) {
      const auto &ui = j["ui"];
      if (ui.contains("mode")) {
        appState.setMenu(stringToMode(ui["mode"]));
      }
      if (ui.contains("statePositions")) {
        for (const auto &[stateName, posJson] : ui["statePositions"].items()) {
          for (const auto &state : appState.tm().states()) {
            if (state.name() == stateName) {
              ImVec2 pos{ posJson["x"], posJson["y"] };
              appState.setStatePosition(state, pos - appState.scrollXY() + appState.canvasOrigin());
              break;
            }
          }
        }
      }
      if (ui.contains("transitionStyles")) {
        for (const auto &[transKey, styleData] : ui["transitionStyles"].items()) {
          if (auto transObj = findTransitionDrawObject(appState, transKey)) {
            transObj->fromJson(styleData);
          }
        }
      }
      if (ui.contains("transitionLabels")) {
        for (const auto &[transKey, labelData] : ui["transitionLabels"].items()) {
          if (auto labelObj = findTransitionLabelDrawObject(appState, transKey)) {
            labelObj->fromJson(labelData);
          }
        }
      }

    }
    appState.tm().reset();
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
    file.close();
    std::cout << "Saved to: " << filepath << std::endl;
    return true;
  } catch (const std::exception &e) {
    std::cerr << "Save error: " << e.what() << std::endl;
    return false;
  }
}

bool AppSerializer::loadFromFile(AppState &appState, const std::string &path)
{
  std::string filepath = path;
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
    std::filesystem::path fsPath(filepath);
    appState.setWindowTitle(fsPath.filename().string());
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

void AppSerializer::saveToFileWithDialog(const AppState &appState)
{
  auto &dlg = AppState::fileBrowserSave();
  dlg.SetTitle("Save to file");
  dlg.SetTypeFilters({ ".json", ".txt" });
  dlg.Open();
}

void AppSerializer::loadFrFileWithDialog(const AppState &appState)
{
  auto &dlg = AppState::fileBrowserOpen();
  dlg.SetTitle("Open file");
  dlg.SetTypeFilters({ ".json", ".txt" });
  dlg.Open();
}

void AppSerializer::rebuildDrawObjects(AppState &appState)
{
  //appState.clearManipulators();

  //for (const auto &state : appState.tm().states()) {
  //  appState.addState(state, ImVec2(100, 100));
  //}

  //for (const auto &transition : appState.tm().transitions()) {
  //  appState.addTransition(transition);
  //}
  appState.rebuildDrawObjectsFromTM();
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
