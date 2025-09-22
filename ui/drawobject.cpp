#include "ui/drawobject.hpp"
#include "app.hpp"
#include "defs.hpp"
#include "ui/manipulators.hpp"
#include "model/turingmachine.hpp"
#include <variant>
#include <format>
#include <cassert>

namespace {
  const float _stateRadius = 30.0f;
} // anonymous namespace


ui::StateDrawObject::StateDrawObject(const core::State &state, AppState *app)
  : DrawObject(app), state_(state)
{
}

void ui::StateDrawObject::draw() const
{
  ImVec2 pos = appState_->statePosition(state_);
  drawState(state_, pos, Colors::black);
}

utils::Rect ui::StateDrawObject::boundingRect() const
{
  ImVec2 pos = appState_->statePosition(state_);
  return utils::Rect{ pos.x - _stateRadius, pos.y - _stateRadius, _stateRadius * 2, _stateRadius * 2 };
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

void ui::StateDrawObject::drawState(const core::State &state, ImVec2 pos, ImU32 clr, bool temp)
{
  ImDrawList *dr = ImGui::GetWindowDrawList();
  ImU32 textClr = Colors::black;
  if (!temp) {
    auto  fill = IM_COL32(0, 128, 255, 255);
    if (state.isAccept()) {
      fill = Colors::green;
    } else if (state.isReject()) {
      fill = Colors::red;
    } else if (state.isStart()) {
      fill = IM_COL32(102, 178, 255, 255);
    }
    dr->AddCircleFilled(pos, _stateRadius, fill);
    textClr = Colors::white;
  }
  dr->AddCircle(pos, _stateRadius, clr, 64, 2.0f);
  ImVec2 sz = ImGui::CalcTextSize(state.name().c_str());
  dr->AddText(ImVec2(pos.x - sz.x / 2, pos.y - sz.y / 2), textClr, state.name().c_str());
}

float ui::StateDrawObject::radius()
{
  return _stateRadius;
}


//------------------------------------------------------------------------------------------


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
  float direction = 1.f;// (style.transitionIndex % 2 == 0) ? 1.0f : -1.0f;
  ImVec2 control = ImVec2(mid.x + perp.x * arcOffset * direction,
    mid.y + perp.y * arcOffset * direction);

  // Calculate bezier midpoint (where label goes)
  float t = 0.5f;
  ImVec2 bezierMid = ImVec2(
    (1 - t) * (1 - t) * edgeFrom.x + 2 * (1 - t) * t * control.x + t * t * edgeTo.x,
    (1 - t) * (1 - t) * edgeFrom.y + 2 * (1 - t) * t * control.y + t * t * edgeTo.y
  );

  // Set control points BEFORE drawing
  controlPoints.points[TransitionControlPoints::START] = edgeFrom;
  controlPoints.points[TransitionControlPoints::MID] = bezierMid;
  controlPoints.points[TransitionControlPoints::END] = edgeTo;
  controlPoints.isValid = true;

  // Draw quadratic Bezier arc from edge to edge
  dr->AddBezierQuadratic(edgeFrom, control, edgeTo, style.color, style.lineThickness);
  if (style.colorHightlight) {
    dr->AddBezierQuadratic(edgeFrom, control, edgeTo, style.colorHightlight.value(), (style.lineThickness / 2.f));
  }

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
  controlPoints.points[TransitionControlPoints::START] = startPoint;
  controlPoints.points[TransitionControlPoints::MID] = midPoint;
  controlPoints.points[TransitionControlPoints::END] = endPoint;
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

ui::TransitionDrawObject::TransitionDrawObject(const core::Transition &trans, AppState *app) 
  : DrawObject(app), transition_(trans)
{
}

void ui::TransitionDrawObject::draw() const
{
  if (isVisible()) {
    ImVec2 posFrom = appState_->statePosition(transition_.from());
    ImVec2 posTo = appState_->statePosition(transition_.to());
    auto style{ style_ };
    if (manipulator_) {
      style.colorHightlight = Colors::red;
    }
    controlPoints_ = drawTransition(transition_, posFrom, posTo, style);
    if (controlPoints_.isValid) {
      ImDrawList *drawList = ImGui::GetWindowDrawList();
      const float handleRadius = 4.0f;
      ImU32 handleColor = Colors::magenta;
      drawList->AddCircleFilled(controlPoints_.points[TransitionControlPoints::START], handleRadius, handleColor);
      drawList->AddCircleFilled(controlPoints_.points[TransitionControlPoints::MID], handleRadius, handleColor);
      drawList->AddCircleFilled(controlPoints_.points[TransitionControlPoints::END], handleRadius, handleColor);
    }
  }
}

bool ui::TransitionDrawObject::containsPoint(float x, float y) const
{
  if (controlPoints_.isValid) {
    return controlPoints_.hitTest(x, y).has_value();
  }
  return false;
}

utils::Rect ui::TransitionDrawObject::boundingRect() const
{
  return utils::Rect();
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

void ui::TransitionDrawObject::addLabel(TransitionLabelDrawObject *label)
{
  labels_.push_back(label);
}

void ui::TransitionDrawObject::removeLabel(TransitionLabelDrawObject *label)
{
  labels_.erase(std::remove(labels_.begin(), labels_.end(), label), labels_.end());
}


//------------------------------------------------------------------------------------------


ui::TransitionLabelDrawObject::TransitionLabelDrawObject(const ui::TransitionDrawObject *tdo, AppState *app)
  : DrawObject(app), tdo_(tdo)
{
}

ImVec2 ui::TransitionLabelDrawObject::getAutoPosition() const
{
  auto controlPoints = tdo_->controlPoints();
  return controlPoints.points[TransitionControlPoints::MID];
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

  auto displayC = [](char c) {
    return std::string(1, '-');
    };
  auto displayDir = [](core::Tape::Dir d) {
    return std::string(d == core::Tape::Dir::LEFT ? "<=" : "=>");
    };

  auto label = std::format("('{}') ; ('{}', {})", displayC(trans.readSymbol()), displayC(trans.writeSymbol()), displayDir(trans.direction()));

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
  rect_.h = textSize.y;
}

utils::Rect ui::TransitionLabelDrawObject::boundingRect() const
{
  return rect_;
}

void ui::TransitionLabelDrawObject::translate(const ImVec2 &delta)
{
  if (!hasManualPosition_) {
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
