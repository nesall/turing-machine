#include "ui/manipulators.hpp"
#include "app.hpp"
#include "defs.hpp"
#include "model/turingmachine.hpp"
#include "ui/drawobject.hpp"
#include <imgui.h>
#include <cassert>
#include <optional>
#include <algorithm>
#include <iostream>
#include <memory>
#include <cstdlib>


namespace {
  auto _stateRadius = ui::StateDrawObject::radius();
}


void ui::StateManipulator::draw(ImDrawList *dr)
{
  auto rc = dro_->boundingRect();
  dr->AddRect(ImVec2(rc.x, rc.y), ImVec2(rc.x + rc.w, rc.y + rc.h), IM_COL32(255, 0, 0, 255));
}

void ui::StateManipulator::setFirstPos(float x, float y)
{
}

void ui::StateManipulator::setNextPos(float x, float y, const ImVec2 &offset)
{
  if (isFreezed()) return;
  dro_->translate(offset);
}

void ui::StateManipulator::setLastPos(float x, float y)
{
}

bool ui::Manipulator::isFreezed() const
{
  return !dro_ || dro_->appState()->isFreezedManipulators();
}


//------------------------------------------------------------------------------------------


struct ui::TransitionManipulator::Impl {
  std::optional<ui::TransitionControlPoints::PointIndex> iPoint;
  std::unique_ptr<core::Transition> origTransition_;
};

ui::TransitionManipulator::TransitionManipulator(ui::DrawObject *p) : Manipulator(p), imp(new Impl)
{
  // TODO: expose to outside?
  imp->iPoint = ui::TransitionControlPoints::END;
}

ui::TransitionManipulator::~TransitionManipulator()
{
  std::cout << "TransitionManipulator::dtor\n";
}

void ui::TransitionManipulator::draw(ImDrawList *dr)
{
  if (imp->iPoint.has_value()) {
    auto drt = dro_->asTransition();
    auto pt = drt->controlPoints().points[imp->iPoint.value()];
    float d = 5;
    dr->AddRect(ImVec2(pt.x - d, pt.y - d), ImVec2(pt.x + d, pt.y + d), Colors::black);
  }
}

void ui::TransitionManipulator::setFirstPos(float x, float y)
{
  if (isFreezed()) return;
  std::cout << "TransitionManipulator::setFirstPos\n";
  auto drt = dro_->asTransition();
  assert(drt);
  if (drt->isVisible()) {
    auto hit = drt->controlPoints().hitTest(x, y);
    if (hit.has_value()) {
      imp->iPoint = hit.value();
      switch (imp->iPoint.value()) {
      case ui::TransitionControlPoints::START:
        startReconnection(DragState::Mode::TRANSITION_CONNECT_START, x, y);
        break;
      case ui::TransitionControlPoints::END:
        startReconnection(DragState::Mode::TRANSITION_CONNECT_END, x, y);
        break;
      default:
        break;
      }
    } else {
      startReconnection(DragState::Mode::TRANSITION_CONNECT_END, x, y);
    }
  }
}

void ui::TransitionManipulator::setNextPos(float x, float y, const ImVec2 &offset)
{
  if (isFreezed()) return;
  auto &appState = *dro_->appState();
  auto &dragState = appState.dragState;
  auto drt = dro_->asTransition();
  const auto &transition = drt->getTransition();
  if (dro_->isVisible()) {
    if (dragState.isTransitionConnecting()) {
      handleReconnection(dragState.mode, x, y);
    } else if (imp->iPoint.has_value()) {
      switch (imp->iPoint.value()) {
      case ui::TransitionControlPoints::START:
        handleReconnection(DragState::Mode::TRANSITION_CONNECT_START, x, y);
        return;
      case ui::TransitionControlPoints::END:
        handleReconnection(DragState::Mode::TRANSITION_CONNECT_END, x, y);
        return;
      default:
        if (1) {
          auto fromPos{ appState.statePosition(transition.from()) };
          auto toPos{ appState.statePosition(transition.to()) };
          const auto [distance, edgeFrom, edgeTo] = StatePosHelperData::calcEdges(fromPos, toPos);
          if (distance < 0.001f) {
            handleSelfLoopManipulation(x, y, offset, drt->style_);
            //drt->setTransitionStyle(drt->style_);
          } else {
            handleCurveManipulation(x, y, offset, edgeFrom, edgeTo);
          }
        }
        break;
      }
    }
  }
}

void ui::TransitionManipulator::setLastPos(float x, float y)
{
  if (isFreezed()) return;
  std::cout << "TransitionManipulator::setLastPos " << this << "\n";
  if (!imp->origTransition_) {
    return;
  }
  assert(dro_->asTransition());
  auto &appState = *dro_->appState();
  auto drt = dro_->asTransition();
  if (auto pObj = appState.targetObject({x, y}); pObj && pObj->asState()) {
    const auto &state = pObj->asState()->getState();
    auto old{ drt->transition_ };
    if (appState.dragState.mode == DragState::Mode::TRANSITION_CONNECT_START) {
      drt->transition_.setFrom(state);
    } else {
      drt->transition_.setTo(state);
    }
    appState.tm().updateTransition(old, drt->transition_);
  } else {
    auto tr{ *imp->origTransition_ };
    imp->origTransition_.reset();
    if (tr.to().isTemporary()) {
      appState.removeTransition(drt->transition_);
    } else {
      drt->transition_.setFrom(tr.from());
      drt->transition_.setTo(tr.to());
    }
  }
  appState.dragState.mode = DragState::Mode::NONE;
  //dro_->removeManipulator();
}  

void ui::TransitionManipulator::startReconnection(DragState::Mode mode, float x, float y)
{
  assert(!imp->origTransition_);
  auto &appState = *dro_->appState();
  auto &dragState = appState.dragState;
  dragState.mode = mode;
  auto drt = dro_->asTransition();
  const auto &tr = drt->getTransition();
  imp->origTransition_ = std::make_unique<core::Transition>(tr);
  core::State dummyState;
  if (tr.from().isTemporary()) dummyState = tr.from();
  else if (tr.to().isTemporary()) dummyState = tr.to();
  else {
    dummyState = core::State{ nextRandomId(), core::State::Type::TEMP };
    if (mode == DragState::Mode::TRANSITION_CONNECT_START) {
      drt->transition_.setFrom(dummyState);
    } else {
      drt->transition_.setTo(dummyState);
    }
  }
  appState.setStatePosition(dummyState, { x, y });
}

void ui::TransitionManipulator::handleReconnection(DragState::Mode mode, float x, float y)
{
  assert(imp->origTransition_);
  auto &appState = *dro_->appState();
  auto drt = dro_->asTransition();
  const auto &tr = drt->getTransition();
  core::State st;
  if (mode == DragState::Mode::TRANSITION_CONNECT_START) {
    st = tr.from();
  } else {
    st = tr.to();
  }
  appState.setStatePosition(st, { x, y });
}

void ui::TransitionManipulator::handleCurveManipulation(float x, float y, const ImVec2 &offset, const ImVec2 &fromPos, const ImVec2 &toPos, bool bEdges)
{
  auto drt = dro_->asTransition();
  auto style = drt->transitionStyle();
  auto &transition = drt->getTransition();
  auto &appState = *dro_->appState();

  // Get state positions
  //ImVec2 fromPos = appState.statePosition(transition.from());
  //ImVec2 toPos = appState.statePosition(transition.to());

  //const auto [distance, edgeFrom, edgeTo] = StatePosHelperData::calcEdges(fromPos, toPos);
  //if (distance < 0.001f) {
  //  handleSelfLoopManipulation(x, y, offset, style);
  //  drt->setTransitionStyle(style);
  //  return;
  //}
  const auto &edgeFrom{ fromPos };
  const auto &edgeTo{ toPos };
  ImVec2 edgeDelta = ImVec2(edgeTo.x - edgeFrom.x, edgeTo.y - edgeFrom.y);
  ImVec2 mid = ImVec2((edgeFrom.x + edgeTo.x) * 0.5f, (edgeFrom.y + edgeTo.y) * 0.5f);

  // Perpendicular direction for arc
  ImVec2 perp = ImVec2(-edgeDelta.y, edgeDelta.x);
  float len = sqrtf(perp.x * perp.x + perp.y * perp.y);
  if (len > 0.0f) {
    perp.x /= len;
    perp.y /= len;
  }

  // Method 1: Project mouse movement onto perpendicular direction
  float projectedOffset = offset.x * perp.x + offset.y * perp.y;

  // Account for current curve direction (alternating for multiple transitions)
  //float direction = (style.transitionIndex % 2 == 0) ? 1.0f : -1.0f;

  // Update arc height based on projected movement (allow negative for curve flipping)
  style.arcHeight += projectedOffset;

  // Method 2: Alternative - Calculate desired arc height from mouse position
  // This gives more intuitive control and preserves sign for curve direction
  ImVec2 mousePos = ImVec2(x, y);
  ImVec2 mouseToMid = ImVec2(mousePos.x - mid.x, mousePos.y - mid.y);
  float desiredHeight = mouseToMid.x * perp.x + mouseToMid.y * perp.y; // Keep sign!

  // Use a blend of both methods for smooth interaction
  float blendFactor = 0.7f; // Favor mouse position method
  float currentHeight = style.arcHeight + (style.transitionIndex * 25.0f);
  float targetHeight = blendFactor * desiredHeight + (1.0f - blendFactor) * (currentHeight + projectedOffset);

  // Remove the index offset to get base arc height
  style.arcHeight = targetHeight - (style.transitionIndex * 25.0f);

  // Enhanced constraints allowing negative values
  const float maxMagnitude = 300.0f;
  const float comfortableMax = 150.0f;

  // Apply soft limiting for both positive and negative values
  if (std::abs(style.arcHeight) > comfortableMax) {
    float sign = (style.arcHeight >= 0) ? 1.0f : -1.0f;
    float excess = abs(style.arcHeight) - comfortableMax;
    float maxExcess = maxMagnitude - comfortableMax;
    float dampened = maxExcess * (1.0f - expf(-excess / 50.0f));
    style.arcHeight = sign * (comfortableMax + dampened);
  }

  // Clamp to absolute maximum
  style.arcHeight = std::clamp(style.arcHeight, -maxMagnitude, maxMagnitude);

  // Optional: Snap to "straight line" when very close to zero
  if (std::abs(style.arcHeight) < 8.0f) {
    style.arcHeight = 0.0f; // Perfectly straight line
  }

  drt->setTransitionStyle(style);
}

void ui::TransitionManipulator::handleSelfLoopManipulation(float x, float y, const ImVec2 &offset, ui::TransitionStyle &style)
{
  const auto &transition = dro_->asTransition()->getTransition();
  ImVec2 pos = dro_->appState()->statePosition(transition.from());

  // Calculate distance from state center to mouse
  float dx = x - pos.x;
  float dy = y - pos.y;
  //float mouseDistance = sqrtf(dx * dx + dy * dy);

  //// Adjust loop radius based on mouse distance
  ////float desiredRadius = mouseDistance - StateDrawObject::radius() - style.selfLoopOffset;
  ////desiredRadius = std::clamp(desiredRadius, 8.0f, 40.0f);
  ////style.selfLoopRadius = desiredRadius;

  // Optional: Also adjust loop position (angle) based on mouse position
  if (offset.x != 0 || offset.y != 0) {
    float angle = atan2f(dy, dx);
    // Convert angle to transition index equivalent for consistent positioning
    int newIndex = (int)((angle * 180.0f / M_PI + 360.0f) / 60.0f) % 6;
    style.transitionIndex = newIndex;
  }
  auto res = StatePosHelperData::calcEdgesSelfLink(pos, style);
  handleCurveManipulation(x, y, offset, res.edgeTo, res.edgeFrom);
}


//------------------------------------------------------------------------------------------


void ui::TransitionLabelManipulator::draw(ImDrawList *dr)
{
  auto rc = dro_->boundingRect();
  dr->AddRect(ImVec2(rc.x - 1.5f, rc.y - 1.f), ImVec2(rc.x + rc.w + 2.f, rc.y + rc.h + 2.f), Colors::red);
  auto drl = dro_->asTransitionLabel();
  assert(drl);
  auto drt = drl->transitionDrawObject();
  assert(drt);
  auto controlPoints = drt->controlPoints();
  ImVec2 midPoint = controlPoints.points[TransitionControlPoints::MID];
  ImVec2 labelCenter = ImVec2(rc.x + rc.w * 0.5f, rc.y + rc.h * 0.5f);
  dr->AddLine(midPoint, labelCenter, Colors::gray, 1.0f);
}

void ui::TransitionLabelManipulator::setFirstPos(float x, float y)
{
}

void ui::TransitionLabelManipulator::setNextPos(float x, float y, const ImVec2 &offset)
{
  if (isFreezed()) return;
  dro_->translate(offset);
}

void ui::TransitionLabelManipulator::setLastPos(float x, float y)
{
}
