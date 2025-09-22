#ifndef _APP_HPP_
#define _APP_HPP_

#include "defs.hpp"
#include "model/turingmachine.hpp"
#include "ui/manipulators.hpp"
#include "ui/drawobject.hpp"
#include <imgui.h>
#include <map>
#include <vector>
#include <memory>

class AppState {
public:
  enum class Menu { SELECT, ADD_STATE, ADD_TRANSITION };

private:
  core::TuringMachine tm_;
  AppState::Menu menu_ = Menu::SELECT;
  std::map<core::State, ImVec2> stateToPosition_;
  ImVec2 canvasOrigin_;
  std::vector<std::unique_ptr<ui::DrawObject>> drawObjects_;

public:
  DragState dragState;
  core::State _tempAddState;

  const core::TuringMachine &tm() const { return tm_; }
  core::TuringMachine &tm() { return tm_; }
  AppState::Menu menu() const { return menu_; }

  void setMenu(AppState::Menu m);

  ImVec2 statePosition(const core::State &state) const;
  void setStatePosition(const core::State &state, ImVec2 pos);

  // --- State / Transition management ---
  void addState(const core::State &state, ImVec2 pos);
  ui::TransitionDrawObject *addTransition(const core::Transition &trans);
  void removeState(const core::State &state);
  void removeTransition(const core::Transition &trans);

  // --- Coordinate transformations ---
  void setCanvasOrigin(const ImVec2 &o);
  ImVec2 screenToCanvas(const ImVec2 &mouse) const;
  ImVec2 canvasToScreen(const ImVec2 &p) const;
  utils::Rect canvasToScreen(const utils::Rect &p) const;

  // --- Object handling ---
  ui::DrawObject *targetObject(const ImVec2 &pos) const;
  void drawObjects();
  void clearManipulators();
  std::vector<ui::Manipulator *> getManipulators() const;
#if 0
  ui::DrawObject *lastAddedDrawObject() { return drawObjects_.back().get(); }
  void updateObjects();
#endif
};

#endif
