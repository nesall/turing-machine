#include "app.hpp"
#include <algorithm>
#include "ui/imfilebrowser.h"


void AppState::setMenu(AppState::Menu m)
{ 
  menu_ = m; 
}

ImVec2 AppState::statePosition(const core::State &state) const
{
  return canvasToScreen(stateToPosition_.at(state));
}

void AppState::setStatePosition(const core::State &state, ImVec2 pos)
{
  stateToPosition_[state] = screenToCanvas(pos);
}

void AppState::addState(const core::State &state, ImVec2 pos)
{
  tm_.addUnconnectedState(state);
  stateToPosition_[state] = screenToCanvas(pos);
  drawObjects_.emplace_back(std::make_unique<ui::StateDrawObject>(state, this));
}

ui::TransitionDrawObject *AppState::addTransition(const core::Transition &trans)
{
  tm_.addTransition(trans);
  auto transObj = std::make_unique<ui::TransitionDrawObject>(trans, this);
  auto *tr = transObj.get();
  drawObjects_.push_back(std::move(transObj));
  auto lb = std::make_unique<ui::TransitionLabelDrawObject>(tr, this);
  tr->addLabel(lb.get());
  drawObjects_.push_back(std::move(lb));
  return tr;
}

void AppState::removeState(const core::State &state)
{
  tm_.removeState(state);
  stateToPosition_.erase(state);
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

void AppState::removeTransition(const core::Transition &trans)
{
  tm_.removeTransition(trans);
  std::vector<ui::DrawObject *> toRemove;
  for (auto &obj : drawObjects_) {
    if (auto t = obj->asTransition()) {
      if (t->getTransition() == trans) {
        for (auto *label : t->getLabels()) {
          toRemove.push_back(label);
        }
        toRemove.push_back(t);
        break;
      }
    }
  }
  drawObjects_.erase(std::remove_if(drawObjects_.begin(), drawObjects_.end(),
    [&](const std::unique_ptr<ui::DrawObject> &obj) {
      return std::find(toRemove.begin(), toRemove.end(), obj.get()) != toRemove.end();
    }), drawObjects_.end());
}

void AppState::setCanvasOrigin(const ImVec2 &o)
{ 
  canvasOrigin_ = o;
}

ImVec2 AppState::screenToCanvas(const ImVec2 &p) const
{
  return ImVec2(p.x - canvasOrigin_.x + scrollXY_.x, p.y - canvasOrigin_.y + scrollXY_.y);
}

ImVec2 AppState::canvasToScreen(const ImVec2 &p) const
{
  return ImVec2(canvasOrigin_.x - scrollXY_.x + p.x, canvasOrigin_.y - scrollXY_.y + p.y);
}

utils::Rect AppState::canvasToScreen(const utils::Rect &p) const
{
  auto t = canvasToScreen(ImVec2{ p.x, p.y });
  return { t.x, t.y, p.w, p.h };
}

ui::DrawObject *AppState::targetObject(const ImVec2 &pos) const
{
  ui::DrawObject *other = nullptr;
  ui::DrawObject *transitionHit = nullptr;
  for (auto &obj : drawObjects_) {
    if (obj->containsPoint(pos.x, pos.y)) {
      if (obj->asTransition()) {
        transitionHit = obj.get();
        break; // First transition wins
      } else if (!other) {
        other = obj.get();
      }
    }
  }
  return transitionHit ? transitionHit : other;
}

void AppState::drawObjects(ImDrawList *dr)
{
  for (auto &obj : drawObjects_) {
    obj->draw(dr);
    if (auto p = obj->getManipulator()) {
      p->draw(dr);
    }
  }
  transitionLabelEditor().render();
}

void AppState::clearDrawObjects()
{
  drawObjects_.clear();
}

void AppState::clearManipulators()
{
  for (auto &obj : drawObjects_) {
    obj->removeManipulator();
  }
}

void AppState::removeSelected()
{
  std::vector<core::State> statesToRemove;
  std::vector<core::Transition> transToRemove;
  for (auto &obj : drawObjects_) {
    if (obj->getManipulator()) {
      if (auto st = obj->asState())
        statesToRemove.push_back(st->getState());
      else if (auto tr = obj->asTransition())
        transToRemove.push_back(tr->getTransition());
    }
  }
  for (const auto &tr : transToRemove) {
    removeTransition(tr);
  }
  for (const auto &st : statesToRemove) {
    removeState(st);
  }
}

std::vector<ui::Manipulator *> AppState::getManipulators() const
{
  std::vector<ui::Manipulator *> mans;
  for (auto &obj : drawObjects_) {
    if (auto p = obj->getManipulator()) {
      mans.push_back(p);
    }
  }
  return mans;
}

#if 0
void AppState::updateObjects()
{
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

ImGui::FileBrowser &AppState::fileBrowser()
{
  static ImGui::FileBrowser fileDialog;
  return fileDialog;
}

void AppState::rebuildDrawObjectsFromTM()
{
  // Clear existing draw objects without affecting TM
  drawObjects_.clear();
  clearManipulators();

  // Create DrawObjects for existing states (don't add to TM again)
  for (const auto &state : tm_.states()) {
    drawObjects_.emplace_back(std::make_unique<ui::StateDrawObject>(state, this));

    // Set default position if not in position map
    if (stateToPosition_.find(state) == stateToPosition_.end()) {
      stateToPosition_[state] = screenToCanvas(ImVec2(100, 100));
    }
  }

  // Create DrawObjects for existing transitions
  for (const auto &transition : tm_.transitions()) {
    drawObjects_.emplace_back(std::make_unique<ui::TransitionDrawObject>(transition, this));
  }
}
