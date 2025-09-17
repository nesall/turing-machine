#ifndef _APP_HPP_
#define _APP_HPP_

#include <imgui.h>
#include <map>
#include <vector>
#include <memory>
#include "model/turingmachine.hpp"
#include "ui/manipulators.hpp"
#include "ui/drawobject.hpp"


inline ImVec2 operator -(const ImVec2 &a, const ImVec2 &b) {
  return ImVec2(a.x - b.x, a.y - b.y);
}

inline ImVec2 operator +(const ImVec2 &a, const ImVec2 &b) {
  return ImVec2(a.x + b.x, a.y + b.y);
}

inline ImVec2 &operator+=(ImVec2 &a, const ImVec2 &b) {
  a.x += b.x;
  a.y += b.y;
  return a;
}



struct DragState {
  bool isDragging = false;
  ImVec2 dragStartPos;
};

class AppState {
public:
  enum class Mode { SELECT, ADD_STATE, ADD_TRANSITION };
private:
  core::TuringMachine tm_;
  AppState::Mode mode_ = Mode::SELECT;
  std::map<core::State, ImVec2> stateToPosition_;
  ImVec2 canvasOrigin_;
  std::vector<std::unique_ptr<ui::DrawObject>> drawObjects_;
public:
  DragState dragState;
  const core::TuringMachine &tm() const { return tm_; }
  core::TuringMachine &tm() { return tm_; }
  AppState::Mode mode() const { return mode_; }
  void setMode(AppState::Mode m) { mode_ = m; }
  ImVec2 statePosition(const core::State &state) const { 
    return canvasToScreen(stateToPosition_.at(state));
  }
  void setStatePosition(const core::State &state, ImVec2 pos) { 
    stateToPosition_[state] = screenToCanvas(pos); 
  }
  void addState(const core::State &state, ImVec2 pos) { 
    stateToPosition_[state] = screenToCanvas(pos);
    drawObjects_.emplace_back(std::make_unique<ui::StateDrawObject>(state, this));
  }
  void addTransition(const core::Transition &trans) { 
    drawObjects_.emplace_back(std::make_unique<ui::TransitionDrawObject>(trans, this));
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