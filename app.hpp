#ifndef _APP_HPP_
#define _APP_HPP_

#include <map>
#include <vector>
#include <memory>
#include "model/turingmachine.hpp"
#include "ui/manipulators.hpp"

class AppState {
public:
  enum class Mode { SELECT, ADD_STATE, ADD_TRANSITION };
private:
  core::TuringMachine tm_;
  std::vector<std::unique_ptr<ui::Manipulator>> manipulators_;
  AppState::Mode mode_ = Mode::SELECT;
  std::map<core::State, ImVec2> stateToPosition_;
public:
  const core::TuringMachine &tm() const { return tm_; }
  core::TuringMachine &tm() { return tm_; }
  AppState::Mode mode() const { return mode_; }
  void setMode(AppState::Mode m) { mode_ = m; }
  std::vector<std::unique_ptr<ui::Manipulator>> &manipulators() { return manipulators_; }
  const std::vector<std::unique_ptr<ui::Manipulator>> &manipulators() const { return manipulators_; }
  ImVec2 statePosition(const core::State &state) const { return stateToPosition_.at(state); }
  void setStatePosition(const core::State &state, ImVec2 pos) { stateToPosition_[state] = pos; }
};



#endif