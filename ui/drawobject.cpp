#include "ui/drawobject.hpp"
#include "app.hpp"
#include "ui/manipulators.hpp"
#include "model/turingmachine.hpp"
#include <variant>
#include <format>
#include <cassert>

namespace {
  const float _stateRadius = 30.0f;
} // anonymous namespace

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#if 0

struct ui::DrawObject::Impl {
  AppState *pAppState_ = nullptr;
  std::variant<core::State, core::Transition> obj_;
  std::unique_ptr<ui::Manipulator> manipulator_ = nullptr;
};

ui::DrawObject::DrawObject(const core::State &state, AppState *app) : imp(std::make_unique<Impl>())
{
  imp->obj_ = state;
  imp->pAppState_ = app;
}

ui::DrawObject::DrawObject(const core::Transition &trans, AppState *app) : imp(std::make_unique<Impl>())
{
  imp->obj_ = trans;
  imp->pAppState_ = app;
}

ui::DrawObject::~DrawObject()
{
}

ui::Manipulator *ui::DrawObject::getOrCreateManipulator(bool bCreate)
{
  if (imp->manipulator_) return imp->manipulator_.get();
  if (bCreate) {
    if (std::holds_alternative<core::State>(imp->obj_)) {
      imp->manipulator_.reset(new ui::StateManipulator(this));
      return imp->manipulator_.get();
    } else if (std::holds_alternative<core::Transition>(imp->obj_)) {
      imp->manipulator_.reset(new ui::TransitionManipulator(this));
      return imp->manipulator_.get();
    }
    assert(!"Should not be here!");
  }
  return nullptr;
}

void ui::DrawObject::removeManipulator()
{
  imp->manipulator_.reset();
}

ui::Rect ui::DrawObject::boundingRect() const
{
  if (std::holds_alternative<core::State>(imp->obj_)) {
    const core::State &st = std::get<core::State>(imp->obj_);
    ImVec2 pos = imp->pAppState_->statePosition(st);
    return ui::Rect{ pos.x - _stateRadius, pos.y - _stateRadius, _stateRadius * 2, _stateRadius * 2 };
  } else if (std::holds_alternative<core::Transition>(imp->obj_)) {
    const core::Transition &tr = std::get<core::Transition>(imp->obj_);
    ImVec2 posFrom = imp->pAppState_->statePosition(tr.from());
    ImVec2 posTo = imp->pAppState_->statePosition(tr.to());
    float minX = std::min(posFrom.x, posTo.x);
    float minY = std::min(posFrom.y, posTo.y);
    float maxX = std::max(posFrom.x, posTo.x);
    float maxY = std::max(posFrom.y, posTo.y);
    // Add some padding for the curve and arrowhead
    const float padding = 0.0f;
    return ui::Rect{ minX - padding, minY - padding, (maxX - minX) + 2 * padding, (maxY - minY) + 2 * padding };
  }
  return ui::Rect();
}

ImVec2 ui::DrawObject::centerPoint() const
{
  return boundingRect().center();
}

bool ui::DrawObject::containsPoint(float x, float y) const
{
  return boundingRect().contains(x, y);
}

void ui::DrawObject::draw() const
{
  if (std::holds_alternative<core::State>(imp->obj_)) {
    const core::State &st = std::get<core::State>(imp->obj_);
    ImVec2 pos = imp->pAppState_->statePosition(st);
    ui::DrawObject::drawState(st, pos, IM_COL32(0, 0, 0, 255));
  } else if (std::holds_alternative<core::Transition>(imp->obj_)) {
    const core::Transition &tr = std::get<core::Transition>(imp->obj_);
    ImVec2 posFrom = imp->pAppState_->statePosition(tr.from());
    ImVec2 posTo = imp->pAppState_->statePosition(tr.to());
    auto controlPoints = ui::DrawObject::drawTransition(tr, posFrom, posTo);
    if (controlPoints.isValid) {
      ImDrawList *drawList = ImGui::GetWindowDrawList();
      const float handleRadius = 4.0f;
      ImU32 handleColor = IM_COL32(32, 165, 255, 255);
      drawList->AddCircleFilled(controlPoints.startPoint, handleRadius, handleColor);
      drawList->AddCircleFilled(controlPoints.midPoint, handleRadius, handleColor);
      drawList->AddCircleFilled(controlPoints.endPoint, handleRadius, handleColor);
    }
  }
}

void ui::DrawObject::translate(const ImVec2 &d)
{
  if (std::holds_alternative<core::State>(imp->obj_)) {
    const core::State &st = std::get<core::State>(imp->obj_);
    ImVec2 pos = imp->pAppState_->statePosition(st);
    imp->pAppState_->setStatePosition(st, pos + d);
  }
}

bool ui::DrawObject::isState() const
{
  return std::holds_alternative<core::State>(imp->obj_);
}

std::optional<core::State> ui::DrawObject::getState() const
{
  if (std::holds_alternative<core::State>(imp->obj_)) {
    return std::get<core::State>(imp->obj_);
  }
  return std::nullopt;
}

std::optional<core::Transition> ui::DrawObject::getTransition() const
{
  if (std::holds_alternative<core::Transition>(imp->obj_)) {
    return std::get<core::Transition>(imp->obj_);
  }
  return std::nullopt;
}
#endif

void ui::StateDrawObject::drawState(const core::State &state, ImVec2 pos, ImU32 clr, bool temp)
{
  ImDrawList *dr = ImGui::GetWindowDrawList();
  ImU32 textClr = IM_COL32(0, 0, 0, 255);
  if (!temp) {
    auto  fill = IM_COL32(0, 128, 255, 255);
    if (state.isAccept()) {
      fill = IM_COL32(0, 255, 0, 255);
    } else if (state.isReject()) {
      fill = IM_COL32(255, 0, 0, 255);
    } else if (state.isStart()) {
      fill = IM_COL32(102, 178, 255, 255);
    }
    dr->AddCircleFilled(pos, _stateRadius, fill);
    textClr = IM_COL32(255, 255, 255, 255);
  }
  dr->AddCircle(pos, _stateRadius, clr, 64, 2.0f);
  ImVec2 sz = ImGui::CalcTextSize(state.name().c_str());
  dr->AddText(ImVec2(pos.x - sz.x / 2, pos.y - sz.y / 2), textClr, state.name().c_str());
}

ui::TransitionControlPoints ui::TransitionDrawObject::drawTransition(const core::Transition &trans, ImVec2 posFrom, ImVec2 posTo, const TransitionStyle &style)
{
  ImDrawList *dr = ImGui::GetWindowDrawList();
  TransitionControlPoints controlPoints;

  // Calculate direction from center to center
  ImVec2 delta = ImVec2(posTo.x - posFrom.x, posTo.y - posFrom.y);
  float distance = sqrtf(delta.x * delta.x + delta.y * delta.y);

  // Handle self-loops or very close states
  if (distance < 0.001f) {
    return drawSelfLoop(trans, posFrom, style);
  }

  // Normalize direction vector
  ImVec2 dir = ImVec2(delta.x / distance, delta.y / distance);

  // Calculate edge points by moving inward by radius
  ImVec2 edgeFrom = ImVec2(posFrom.x + dir.x * _stateRadius, posFrom.y + dir.y * _stateRadius);
  ImVec2 edgeTo = ImVec2(posTo.x - dir.x * _stateRadius, posTo.y - dir.y * _stateRadius);

  // Handle multiple transitions between same states by offsetting the curve
  float arcOffset = style.arcHeight + (style.transitionIndex * 25.0f);

  // Compute control points for a quadratic Bezier curve
  ImVec2 edgeDelta = ImVec2(edgeTo.x - edgeFrom.x, edgeTo.y - edgeFrom.y);
  ImVec2 mid = ImVec2((edgeFrom.x + edgeTo.x) * 0.5f, (edgeFrom.y + edgeTo.y) * 0.5f);

  // Perpendicular direction for arc "height"
  ImVec2 perp = ImVec2(-edgeDelta.y, edgeDelta.x);
  float len = sqrtf(perp.x * perp.x + perp.y * perp.y);
  if (len > 0.0f) {
    perp.x /= len;
    perp.y /= len;
  }

  // Alternate direction for multiple transitions
  float direction = (style.transitionIndex % 2 == 0) ? 1.0f : -1.0f;
  ImVec2 control = ImVec2(mid.x + perp.x * arcOffset * direction,
    mid.y + perp.y * arcOffset * direction);

  // Calculate bezier midpoint (where label goes)
  float t = 0.5f;
  ImVec2 bezierMid = ImVec2(
    (1 - t) * (1 - t) * edgeFrom.x + 2 * (1 - t) * t * control.x + t * t * edgeTo.x,
    (1 - t) * (1 - t) * edgeFrom.y + 2 * (1 - t) * t * control.y + t * t * edgeTo.y
  );

  // Set control points BEFORE drawing
  controlPoints.startPoint = edgeFrom;
  controlPoints.midPoint = bezierMid;
  controlPoints.endPoint = edgeTo;
  controlPoints.isValid = true;

  // Draw quadratic Bezier arc from edge to edge
  dr->AddBezierQuadratic(edgeFrom, control, edgeTo, style.color, style.lineThickness);

  // Draw arrowhead
  drawArrowhead(edgeTo, control, style);

  //// Draw transition label
  //drawTransitionLabel(trans, edgeFrom, control, edgeTo, style);

  return controlPoints;
}



ui::TransitionControlPoints ui::TransitionDrawObject::drawSelfLoop(const core::Transition &trans, ImVec2 pos, const TransitionStyle &style)
{
  ImDrawList *dr = ImGui::GetWindowDrawList();
  TransitionControlPoints controlPoints;

  // Position loop based on transition index to avoid overlap
  float angle = (style.transitionIndex * 60.0f) * (M_PI / 180.0f); // 60 degrees apart
  float loopDistance = _stateRadius + style.selfLoopOffset;

  ImVec2 loopCenter = ImVec2(
    pos.x + cosf(angle) * loopDistance,
    pos.y + sinf(angle) * loopDistance
  );

  // Calculate control points for self-loop
  // Start point: where loop connects to state
  ImVec2 startPoint = ImVec2(
    pos.x + cosf(angle - M_PI * 0.25f) * _stateRadius,
    pos.y + sinf(angle - M_PI * 0.25f) * _stateRadius
  );

  // End point: where loop returns to state  
  ImVec2 endPoint = ImVec2(
    pos.x + cosf(angle + M_PI * 0.25f) * _stateRadius,
    pos.y + sinf(angle + M_PI * 0.25f) * _stateRadius
  );

  // Mid point: opposite side of loop (good for label and user interaction)
  ImVec2 midPoint = ImVec2(
    loopCenter.x + cosf(angle + M_PI) * style.selfLoopRadius,
    loopCenter.y + sinf(angle + M_PI) * style.selfLoopRadius
  );

  // Set control points
  controlPoints.startPoint = startPoint;
  controlPoints.midPoint = midPoint;
  controlPoints.endPoint = endPoint;
  controlPoints.isValid = true;

  // Draw loop circle
  dr->AddCircle(loopCenter, style.selfLoopRadius, style.color, 32, style.lineThickness);

  // Arrow position on the loop
  ImVec2 arrowPos = ImVec2(
    loopCenter.x + cosf(angle + M_PI * 0.25f) * style.selfLoopRadius,
    loopCenter.y + sinf(angle + M_PI * 0.25f) * style.selfLoopRadius
  );

  // Draw arrowhead
  ImVec2 arrowDir = ImVec2(cosf(angle + M_PI * 0.25f), sinf(angle + M_PI * 0.25f));
  drawArrowheadAtPoint(arrowPos, arrowDir, style);

  // Label position (use midPoint for consistency)
  auto label = std::format("{}; {}, {}", trans.readSymbol(), trans.writeSymbol(),
    trans.direction() == core::Tape::Dir::LEFT ? "L" : "R");
  ImVec2 textSize = ImGui::CalcTextSize(label.c_str());

  // Place label near midPoint but offset outward for readability
  ImVec2 labelOffset = ImVec2(cosf(angle + M_PI) * 15.0f, sinf(angle + M_PI) * 15.0f);
  ImVec2 labelPos = ImVec2(midPoint.x + labelOffset.x - textSize.x / 2,
    midPoint.y + labelOffset.y - textSize.y / 2);

  // Background for readability
  dr->AddRectFilled(ImVec2(labelPos.x - 2, labelPos.y - 1),
    ImVec2(labelPos.x + textSize.x + 2, labelPos.y + textSize.y + 1),
    IM_COL32(255, 255, 255, 200));

  dr->AddText(labelPos, style.textColor, label.c_str());

  return controlPoints;
}

void ui::TransitionDrawObject::drawArrowhead(ImVec2 tipPos, ImVec2 controlPos, const TransitionStyle &style)
{
  ImDrawList *dr = ImGui::GetWindowDrawList();

  // Compute direction at end of Bezier: tangent from control to tip
  ImVec2 arrowDir = ImVec2(tipPos.x - controlPos.x, tipPos.y - controlPos.y);
  float arrowDirLen = sqrtf(arrowDir.x * arrowDir.x + arrowDir.y * arrowDir.y);

  if (arrowDirLen > 0.0f) {
    arrowDir.x /= arrowDirLen;
    arrowDir.y /= arrowDirLen;
    drawArrowheadAtPoint(tipPos, arrowDir, style);
  }
}

void ui::TransitionDrawObject::drawArrowheadAtPoint(ImVec2 tipPos, ImVec2 direction, const TransitionStyle &style)
{
  ImDrawList *dr = ImGui::GetWindowDrawList();

  ImVec2 arrowPerp = ImVec2(-direction.y, direction.x);
  float halfSize = style.arrowSize * 0.5f;

  ImVec2 arrowP1 = ImVec2(tipPos.x - direction.x * style.arrowSize + arrowPerp.x * halfSize,
    tipPos.y - direction.y * style.arrowSize + arrowPerp.y * halfSize);
  ImVec2 arrowP2 = ImVec2(tipPos.x - direction.x * style.arrowSize - arrowPerp.x * halfSize,
    tipPos.y - direction.y * style.arrowSize - arrowPerp.y * halfSize);

  dr->AddTriangleFilled(tipPos, arrowP1, arrowP2, style.color);
}

//void ui::TransitionDrawObject::drawTransitionLabel(const core::Transition &trans, ImVec2 start, ImVec2 control, ImVec2 end, const TransitionStyle &style)
//{
//  ImDrawList *dr = ImGui::GetWindowDrawList();
//
//  // Use Bezier midpoint for label placement
//  float t = 0.5f;
//  ImVec2 bezierMid = ImVec2(
//    (1 - t) * (1 - t) * start.x + 2 * (1 - t) * t * control.x + t * t * end.x,
//    (1 - t) * (1 - t) * start.y + 2 * (1 - t) * t * control.y + t * t * end.y
//  );
//
//  auto label = std::format("{}; {}, {}", trans.readSymbol(), trans.writeSymbol(),
//    trans.direction() == core::Tape::Dir::LEFT ? "L" : "R");
//
//  ImVec2 textSize = ImGui::CalcTextSize(label.c_str());
//
//  // Add background rectangle for better readability
//  ImVec2 labelPos = ImVec2(bezierMid.x - textSize.x / 2, bezierMid.y - textSize.y / 2);
//  dr->AddRectFilled(ImVec2(labelPos.x - 2, labelPos.y - 1),
//    ImVec2(labelPos.x + textSize.x + 2, labelPos.y + textSize.y + 1),
//    IM_COL32(255, 255, 255, 200));
//
//  dr->AddText(labelPos, style.textColor, label.c_str());
//}



ui::StateDrawObject::StateDrawObject(const core::State &state, AppState *app) 
  : DrawObject(app), state_(state)
{
}

void ui::StateDrawObject::draw() const
{
  ImVec2 pos = appState_->statePosition(state_);
  drawState(state_, pos, IM_COL32(0, 0, 0, 255));
}

ui::Rect ui::StateDrawObject::boundingRect() const
{
  ImVec2 pos = appState_->statePosition(state_);
  return ui::Rect{ pos.x - _stateRadius, pos.y - _stateRadius, _stateRadius * 2, _stateRadius * 2 };
}

void ui::StateDrawObject::translate(const ImVec2 &delta)
{
  ImVec2 pos = appState_->statePosition(state_);
  appState_->setStatePosition(state_, pos + delta);
}

ui::Manipulator *ui::StateDrawObject::getOrCreateManipulator(bool bCreate)
{
  if (manipulator_) return manipulator_.get();
  if (bCreate) {
    manipulator_.reset(new ui::StateManipulator(this));
    return manipulator_.get();
  }
  return nullptr;
}



ui::TransitionDrawObject::TransitionDrawObject(const core::Transition &trans, AppState *app) 
  : DrawObject(app), transition_(trans)
{
}

void ui::TransitionDrawObject::draw() const
{
  ImVec2 posFrom = appState_->statePosition(transition_.from());
  ImVec2 posTo = appState_->statePosition(transition_.to());
  controlPoints_ = drawTransition(transition_, posFrom, posTo);
  if (controlPoints_.isValid) {
    ImDrawList *drawList = ImGui::GetWindowDrawList();
    const float handleRadius = 4.0f;
    ImU32 handleColor = IM_COL32(255, 0, 255, 255);
    drawList->AddCircleFilled(controlPoints_.startPoint, handleRadius, handleColor);
    drawList->AddCircleFilled(controlPoints_.midPoint, handleRadius, handleColor);
    drawList->AddCircleFilled(controlPoints_.endPoint, handleRadius, handleColor);
  }
}

bool ui::TransitionDrawObject::containsPoint(float x, float y) const
{
  return false;
}

ui::Rect ui::TransitionDrawObject::boundingRect() const
{
  return ui::Rect();
}

void ui::TransitionDrawObject::translate(const ImVec2 &delta)
{
}

ui::Manipulator *ui::TransitionDrawObject::getOrCreateManipulator(bool bCreate)
{
  if (manipulator_) return manipulator_.get();
  if (bCreate) {
    manipulator_.reset(new ui::TransitionManipulator(this));
    return manipulator_.get();
  }
  return nullptr;
}



ui::TransitionLabelDrawObject::TransitionLabelDrawObject(const ui::TransitionDrawObject *tdo, AppState *app)
  : DrawObject(app), tdo_(tdo)
{
}

ImVec2 ui::TransitionLabelDrawObject::getAutoPosition() const
{
  auto controlPoints = tdo_->controlPoints();
  return controlPoints.midPoint;
}

ImVec2 ui::TransitionLabelDrawObject::getFinalPosition() const
{
  return getAutoPosition() + manualOffset_;
}

void ui::TransitionLabelDrawObject::setManualOffset(const ImVec2 &offset)
{
  manualOffset_ = offset;
  hasManualPosition_ = true;
}

void ui::TransitionLabelDrawObject::resetToAutoPosition()
{
  manualOffset_ = ImVec2(0, 0);
  hasManualPosition_ = false;
}

void ui::TransitionLabelDrawObject::draw() const
{
  ImDrawList *dr = ImGui::GetWindowDrawList();

  float t = 0.5f;
  auto pos = getFinalPosition();

  const auto &trans = tdo_->getTransition();
  const auto &style = tdo_->transitionStyle();

  auto label = std::format("{}; {}, {}", trans.readSymbol(), trans.writeSymbol(),
    trans.direction() == core::Tape::Dir::LEFT ? "L" : "R");

  ImVec2 textSize = ImGui::CalcTextSize(label.c_str());

  // Add background rectangle for better readability
  ImVec2 labelPos = ImVec2(pos.x - textSize.x / 2, pos.y - textSize.y / 2);
  dr->AddRectFilled(ImVec2(labelPos.x - 2, labelPos.y - 1),
    ImVec2(labelPos.x + textSize.x + 2, labelPos.y + textSize.y + 1),
    IM_COL32(255, 255, 255, 200));

  dr->AddText(labelPos, style.textColor, label.c_str());

  rect_.x = labelPos.x;
  rect_.y = labelPos.y;
  rect_.w = textSize.x;
  rect_.y = textSize.y;
}

void ui::TransitionLabelDrawObject::translate(const ImVec2 &delta)
{
  if (!hasManualPosition_) {
    // First time user is moving it, switch to manual mode
    ImVec2 autoPos = getAutoPosition();
    manualOffset_ = getFinalPosition() - autoPos;
    hasManualPosition_ = true;
  }
  manualOffset_ += delta;
}

ui::Manipulator *ui::TransitionLabelDrawObject::getOrCreateManipulator(bool bCreate)
{
  if (manipulator_) return manipulator_.get();
  if (bCreate) {
    manipulator_.reset(new ui::TransitionLabelManipulator(this));
    return manipulator_.get();
  }
  return nullptr;
}
