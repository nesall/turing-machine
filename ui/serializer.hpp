#ifndef _SERIALIZER_HPP_
#define _SERIALIZER_HPP_

#include <nlohmann/json.hpp>
#include <string>
#include <vector>

class AppState;

namespace core {
  class Transition;
}

namespace ui {
  class TransitionDrawObject;
}

class AppSerializer {
public:
  // Save complete application state
  static nlohmann::json serialize(const AppState &appState);
  // Load complete application state
  static bool deserialize(const nlohmann::json &j, AppState &appState);
  static bool saveToFile(const AppState &appState, const std::string &filename = "");
  static bool loadFromFile(AppState &appState, const std::string &filename = "");
  static std::vector<std::string> getSavedFiles();

  static void saveToFileWithDialog(const AppState &appState);
  static void loadFrFileWithDialog(const AppState &appState);

private:
  static void rebuildDrawObjects(AppState &appState);
  static std::string getCurrentTimestamp();
  static std::string findMostRecentFile();
};


#endif // _SERIALIZER_HPP_