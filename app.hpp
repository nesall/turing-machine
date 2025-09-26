#ifndef _APP_HPP_
#define _APP_HPP_

#include "defs.hpp"
#include "model/turingmachine.hpp"
#include "ui/manipulators.hpp"
#include "ui/drawobject.hpp"
#include <imgui.h>
#include <map>
#include <set>
#include <vector>
#include <memory>
#include <string>


namespace ImGui {
  class FileBrowser;
}


class AppState {
public:
  enum class Menu { SELECT, ADD_STATE, ADD_TRANSITION, RUNNING, PAUSED };

private:
  core::TuringMachine tm_;
  core::MachineExecutor executor_;
  AppState::Menu menu_ = Menu::SELECT;
  std::map<core::State, ImVec2> stateToPosition_;
  ImVec2 canvasOrigin_;
  std::vector<std::unique_ptr<ui::DrawObject>> drawObjects_;
  std::string windowTitle_;
  ui::TransitionLabelEditor labelEditor_;
  ui::StateEditor stateEditor_;
  ImVec2 scrollXY_;
  std::set<std::string> popupNames_;
  ui::SelectionDrawObject selectionObj_;

  ui::TransitionDrawObject *createTransitionObject(const core::Transition &trans);

public:
  AppState();
  void reset();

  DragState dragState;
  core::State tempAddState_;

  const core::TuringMachine &tm() const { return tm_; }
  core::TuringMachine &tm() { return tm_; }
  AppState::Menu menu() const { return menu_; }
  void setMenu(AppState::Menu m);
  std::string windowTitle() const { return windowTitle_; }
  void setWindowTitle(const std::string &s) { windowTitle_ = s; }

  ImVec2 statePosition(const core::State &state) const;
  void removeStatePosition(const core::State &state);
  void setStatePosition(const core::State &state, ImVec2 pos);

  const ui::TransitionLabelEditor &transitionLabelEditor() const { return labelEditor_; }
  ui::TransitionLabelEditor &transitionLabelEditor() { return labelEditor_; }
  const ui::StateEditor &stateEditor() const { return stateEditor_; }
  ui::StateEditor &stateEditor() { return stateEditor_; }

  // --- State / Transition management ---
  void addState(const core::State &state, ImVec2 pos);
  ui::TransitionDrawObject *addTransition(const core::Transition &trans);
  void removeState(const core::State &state);
  void removeTransition(const core::Transition &trans);
  void updateState(const core::State &what, const core::State &with);

  // --- Coordinate transformations ---
  void setCanvasOrigin(const ImVec2 &o);
  ImVec2 canvasOrigin() const { return canvasOrigin_; }
  void setScrollXY(const ImVec2 &s) { scrollXY_ = s; }
  ImVec2 scrollXY() const { return scrollXY_; }
  ImVec2 screenToCanvas(const ImVec2 &mouse) const;
  ImVec2 canvasToScreen(const ImVec2 &p) const;
  utils::Rect canvasToScreen(const utils::Rect &p) const;

  // --- Object handling ---
  ui::DrawObject *targetObject(const ImVec2 &pos) const;
  void drawObjects(ImDrawList *dr);
  void clearDrawObjects();
  void clearManipulators();
  void removeSelected();
  std::vector<ui::Manipulator *> getManipulators() const;
  size_t nofDrawObjects() const { return drawObjects_.size(); }
  const ui::DrawObject *getDrawObject(size_t i) const { return drawObjects_.at(i).get(); }
#if 0
  ui::DrawObject *lastAddedDrawObject() { return drawObjects_.back().get(); }
  void updateObjects();
#endif
  void rebuildDrawObjectsFromTM();
  ui::SelectionDrawObject &selectionObj() { return selectionObj_; }
  const ui::SelectionDrawObject &selectionObj() const { return selectionObj_; }
  bool isActiveSelection() const { return selectionObj_.getManipulator(); }

  // --- Execution ---
  void startExecution();
  void pauseExecution();
  void stepExecution();
  void stopExecution();
  void updateExecution();
  core::ExecutionState getExecutionState() const;
  bool isExecuting() const;
  float executionSpeed() const { return executor_.speedFactor(); }
  void setExecutionSpeed(float f) { executor_.setSpeedFactor(f); }
  core::ExecutionValidator::ValidationResult validateMachine() const;
  std::string getFormattedExecutionTime() const { return executor_.getFormattedTime(); }
  size_t getCellsUsed() const { return executor_.cellsUsed(); }
  std::pair<int, int> getTapeRange() const { return tm().tape().getUsedRange(); }
  size_t getStepCount() const { return executor_.stepCount(); }

  // --- Misc ---
  static ImGui::FileBrowser &fileBrowserSave();
  static ImGui::FileBrowser &fileBrowserOpen();

  void registerPopupName(const std::string &s);
  bool hasOpenPopup() const;
};

#endif
