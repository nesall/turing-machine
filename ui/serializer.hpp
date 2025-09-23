#ifndef _SERIALIZER_HPP_
#define _SERIALIZER_HPP_

#include <nlohmann/json.hpp>
#include <string>
#include <vector>

// Forward declarations
class AppState;

class AppSerializer {
public:
  // Save complete application state
  static nlohmann::json serialize(const AppState &appState);

  // Load complete application state
  static bool deserialize(const nlohmann::json &j, AppState &appState);

  // Save to file
  static bool saveToFile(const AppState &appState, const std::string &filename = "");

  // Load from file
  static bool loadFromFile(AppState &appState, const std::string &filename = "");

  // Get list of saved files for UI dropdown
  static std::vector<std::string> getSavedFiles();

private:
  // Rebuild DrawObjects after loading TuringMachine data
  static void rebuildDrawObjects(AppState &appState);
  static std::string getCurrentTimestamp();
  static std::string findMostRecentFile();
};


#endif // _SERIALIZER_HPP_