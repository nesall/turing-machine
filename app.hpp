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
  // TODO: rename Mode to Menu
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
  //std::unique_ptr<ui::DrawObject> tempTransition_;
  const core::TuringMachine &tm() const { return tm_; }
  core::TuringMachine &tm() { return tm_; }
  AppState::Menu menu() const { return menu_; }
  void setMenu(AppState::Menu m) { menu_ = m; }
  ImVec2 statePosition(const core::State &state) const { 
    return canvasToScreen(stateToPosition_.at(state));
  }
  void setStatePosition(const core::State &state, ImVec2 pos) { 
    stateToPosition_[state] = screenToCanvas(pos); 
  }
  void addState(const core::State &state, ImVec2 pos) { 
    tm_.addUnconnectedState(state);
    stateToPosition_[state] = screenToCanvas(pos);
    drawObjects_.emplace_back(std::make_unique<ui::StateDrawObject>(state, this));
  }
  ui::TransitionDrawObject *addTransition(const core::Transition &trans) {
    tm_.addTransition(trans);
    drawObjects_.emplace_back(std::make_unique<ui::TransitionDrawObject>(trans, this));
    return static_cast<ui::TransitionDrawObject *>(drawObjects_.back().get());
  }
  void removeState(const core::State &state) {
    tm_.removeState(state);
    stateToPosition_.erase(state);
    // Remove from draw objects (state + all its transitions)
    drawObjects_.erase(std::remove_if(drawObjects_.begin(), drawObjects_.end(),
      [&](const std::unique_ptr<ui::DrawObject> &obj) {
        if (auto stateObj = obj->asState()) {
          return stateObj->getState() == state;
        }
        if (auto transObj = obj->asTransition()) {
          const auto &trans = transObj->getTransition();
          return trans.from() == state || trans.to() == state;
        }
        return false;
      }), drawObjects_.end());
  }
  void removeTransition(const core::Transition &trans) {
    tm_.removeTransition(trans);
    drawObjects_.erase(std::remove_if(drawObjects_.begin(), drawObjects_.end(),
      [&](const std::unique_ptr<ui::DrawObject> &obj) {
        if (auto p = obj->asTransition()) {
          return p->getTransition() == trans;
        }
        return false;
      }), drawObjects_.end());
  }
  ui::DrawObject *lastAddedDrawObject() {
    return drawObjects_.back().get();
  }

#if 0
  void updateObjects() {
    // Clear existing positions for states that no longer exist
    std::vector<core::State> toRemove;
    auto tms = tm_.states();
    for (const auto &[st, pos] : stateToPosition_) {
      if (std::find(tms.begin(), tms.end(), st) == tms.end()) {
        toRemove.push_back(st);
      }
    }
    for (const auto &st : toRemove) {
      stateToPosition_.erase(st);
    }
    // Add default positions for new states
    for (const auto &st : tm_.states()) {
      if (stateToPosition_.find(st) == stateToPosition_.end()) {
        stateToPosition_[st] = ImVec2(100.0f + 50.0f * stateToPosition_.size(), 100.0f + 50.0f * stateToPosition_.size());
      }
    }
  }
#endif

  void setCanvasOrigin(const ImVec2 &o) { canvasOrigin_ = o; }
  ImVec2 screenToCanvas(const ImVec2 &mouse) const {
    return ImVec2(mouse.x - canvasOrigin_.x + ImGui::GetScrollX(), mouse.y - canvasOrigin_.y + ImGui::GetScrollY());
  }
  ImVec2 canvasToScreen(const ImVec2 &p) const {
    return ImVec2(canvasOrigin_.x - ImGui::GetScrollX() + p.x, canvasOrigin_.y - ImGui::GetScrollY() + p.y);
  }
  ui::DrawObject *targetObject(const ImVec2 &pos) const {
    for (auto &obj : drawObjects_) {
      if (obj->containsPoint(pos.x, pos.y)) {
        return obj.get();
      }
    }
    return nullptr;
  }
  void drawObjects() {
    for (auto &obj : drawObjects_) {
      obj->draw();
      if (auto p = obj->getManipulator()) {
        p->draw();
      }
    }
  }
  void clearManipulators() {
    for (auto &obj : drawObjects_) {
      obj->removeManipulator();
    }
  }
  std::vector<ui::Manipulator *> getManipulators() const {
    std::vector<ui::Manipulator *> mans;
    for (auto &obj : drawObjects_) {
      if (auto p = obj->getManipulator()) {
        mans.push_back(p);
      }
    }
    return mans;
  }

};



#endif