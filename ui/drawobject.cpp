#include "ui/drawobject.hpp"
#include "app.hpp"
#include "defs.hpp"
#include "ui/manipulators.hpp"
#include "model/turingmachine.hpp"
#include <cmath>
#include <format>
#include <string>
#include <cstring>
#include <iostream>


namespace {
  const float _stateRadius = 25.0f;
} // anonymous namespace


ui::StatePosHelperData ui::StatePosHelperData::calcEdges(const ImVec2 &fr, const ImVec2 &to)
{
  StatePosHelperData res;
  ImVec2 dir = ImVec2(to.x - fr.x, to.y - fr.y);
  res.distance = std::sqrt(dir.x * dir.x + dir.y * dir.y);
  if (res.distance < 0.0001f) {
    // Same position (self-loop)
    res.edgeFrom = fr;
    res.edgeTo = to;
    res.distance = 0.0f;
  } else {
    dir.x /= res.distance;
    dir.y /= res.distance;
    res.edgeFrom = ImVec2(fr.x + dir.x * _stateRadius, fr.y + dir.y * _stateRadius);
    res.edgeTo = ImVec2(to.x - dir.x * _stateRadius, to.y - dir.y * _stateRadius);
  }
  return res;
}

ui::StatePosHelperData ui::StatePosHelperData::calcEdgesSelfLink(const ImVec2 &pos, const ui::TransitionStyle &style)
{
  StatePosHelperData res;
  const float angle = style.selfLinkRotationAngle * (style.transitionIndex * 60.0f) * (M_PI / 180.0f); // 60 degrees apart
  const float rate = 0.125f;
  res.edgeFrom = ImVec2(
    pos.x + cosf(angle - M_PI * rate) * _stateRadius,
    pos.y + sinf(angle - M_PI * rate) * _stateRadius
  );
  res.edgeTo = ImVec2(
    pos.x + cosf(angle + M_PI * rate) * _stateRadius,
    pos.y + sinf(angle + M_PI * rate) * _stateRadius
  );
  return res;
}


nlohmann::json ui::TransitionStyle::toJson() const
{
  return {
    {"arcHeight", arcHeight},
    {"lineThickness", lineThickness},
    {"arrowSize", arrowSize},
    //{"selfLoopRadius", selfLoopRadius},
    //{"selfLoopOffset", selfLoopOffset},
    {"color", color},
    {"textColor", textColor},
    {"transitionIndex", transitionIndex}
  };
}

void ui::TransitionStyle::fromJson(const nlohmann::json &j)
{
  arcHeight = j.value("arcHeight", 40.0f);
  lineThickness = j.value("lineThickness", 3.0f);
  arrowSize = j.value("arrowSize", 10.0f);
  //selfLoopRadius = j.value("selfLoopRadius", 15.0f);
  //selfLoopOffset = j.value("selfLoopOffset", 20.0f);
  color = j.value("color", IM_COL32(255, 165, 0, 255));
  textColor = j.value("textColor", IM_COL32(0, 0, 0, 255));
  transitionIndex = j.value("transitionIndex", 0);
}


//------------------------------------------------------------------------------------------


ui::StateDrawObject::StateDrawObject(const core::State &state, AppState *app)
  : DrawObject(app), state_(state)
{
}

void ui::StateDrawObject::draw(ImDrawList *) const
{
  ImVec2 pos = appState_->statePosition(state_);
  drawState(*appState_, state_, pos, Colors::black);
  ImVec2 mousePos = ImGui::GetMousePos();
  if (containsPoint(mousePos.x, mousePos.y) && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
    appState_->stateEditor().openEditor(state_,
      [=](const core::State &st) {
        appState_->updateState(state_, st);
      });
  }
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

void ui::StateDrawObject::drawState(AppState &appState, const core::State &state, ImVec2 pos, ImU32 clr, bool temp)
{
  ImDrawList *dr = ImGui::GetWindowDrawList();
  ImU32 textClr = Colors::black;
  if (!temp) {
    auto  fill = Colors::royalBlue;
    if (state.isAccept()) {
      fill = Colors::darkGreen;
    } else if (state.isReject()) {
      fill = Colors::red;
    } else if (state.isStart()) {
      fill = Colors::dodgerBlue;
    }
    if (appState.isExecuting() && appState.tm().currentState() == state) {
      fill = Colors::pastelYellow; // Yellow highlight
      // Optional: pulsing effect
      float pulse = (sin(ImGui::GetTime() * 8.0f) + 1.0f) * 0.5f;
      fill = IM_COL32(255, 255, (int)(255 * pulse), 200);
    } else {
      textClr = Colors::white;
    }
    dr->AddCircleFilled(pos, _stateRadius, fill);
  }
  dr->AddCircle(pos, _stateRadius, clr, 64, 2.0f);
  ImVec2 sz = ImGui::CalcTextSize(state.name().c_str());
  dr->AddText(ImVec2(pos.x - sz.x / 2, pos.y - sz.y / 2), textClr, state.name().c_str());
  if (state.isStart()) {
    // Draw a little arrow pointing to the state circle from the left side (arrow points to the right to the circle)
    ImVec2 p1 = ImVec2(pos.x - _stateRadius - 20, pos.y);
    ImVec2 p2 = ImVec2(pos.x - _stateRadius, pos.y);
    dr->AddLine(p1, p2, Colors::black, 1.0f);
    dr->AddTriangleFilled(ImVec2(p2.x - 10, p2.y - 5), ImVec2(p2.x, p2.y), ImVec2(p2.x - 10, p2.y + 5), Colors::black);
  }
}

float ui::StateDrawObject::radius()
{
  return _stateRadius;
}


//------------------------------------------------------------------------------------------


ui::TransitionControlPoints ui::TransitionDrawObject::drawTransition(
  AppState &appState, const core::Transition &trans, ImVec2 fromPos, ImVec2 toPos, const TransitionStyle &style)
{
  ImDrawList *dr = ImGui::GetWindowDrawList();
  TransitionControlPoints controlPoints;

  const auto [distance, edgeFrom, edgeTo] = StatePosHelperData::calcEdges(fromPos, toPos);
  if (distance < 0.001f) {
    return drawSelfLoop(appState, trans, fromPos, style);
  }

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

  ImVec2 control = ImVec2(mid.x + perp.x * arcOffset, mid.y + perp.y * arcOffset);

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

  //float lineThickness = style.lineThickness;
  //auto lineColor = style.color;
  auto colorHighlight = style.colorHighlight;
  if (appState.tm().lastExecutedTransition() == trans.uniqueKey()) {
    colorHighlight = Colors::cyan;
    //lineThickness *= 2.0f; // Thicker line
  }

  // Draw quadratic Bezier arc from edge to edge
  dr->AddBezierQuadratic(edgeFrom, control, edgeTo, style.color, style.lineThickness);
  if (colorHighlight) {
    dr->AddBezierQuadratic(edgeFrom, control, edgeTo, colorHighlight.value(), (style.lineThickness / 2.f));
  }

  // Draw arrowhead
  drawArrowhead(edgeTo, control, style);

  return controlPoints;
}

ui::TransitionControlPoints ui::TransitionDrawObject::drawSelfLoop(AppState &appState, const core::Transition &trans, ImVec2 pos, const TransitionStyle &style)
{
  auto res = StatePosHelperData::calcEdgesSelfLink(pos, style);
  return drawTransition(appState, trans, res.edgeFrom, res.edgeTo, style);
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

void ui::TransitionDrawObject::draw(ImDrawList *dr) const
{
  if (isVisible()) {
    ImVec2 posFrom = appState_->statePosition(transition_.from());
    ImVec2 posTo = appState_->statePosition(transition_.to());
    auto style{ style_ };
    if (manipulator_) {
      style.colorHighlight = Colors::red;
    }
    controlPoints_ = drawTransition(*appState_, transition_, posFrom, posTo, style);
    if (controlPoints_.isValid) {
      const float handleRadius = 4.0f;
      ImU32 handleColor = Colors::gray;
      dr->AddCircleFilled(controlPoints_.points[TransitionControlPoints::START], handleRadius, handleColor);
      dr->AddCircleFilled(controlPoints_.points[TransitionControlPoints::MID], handleRadius, handleColor);
      dr->AddCircleFilled(controlPoints_.points[TransitionControlPoints::END], handleRadius, handleColor);
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

nlohmann::json ui::TransitionDrawObject::toJson() const
{
  nlohmann::json j;
  j["style"] = style_.toJson();
  j["visible"] = isVisible();
  return j;
}

void ui::TransitionDrawObject::fromJson(const nlohmann::json &j)
{
  if (j.contains("style")) {
    style_.fromJson(j["style"]);
  }
  setVisible(j.value("visible", true));
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

void ui::TransitionLabelDrawObject::draw(ImDrawList *dr) const
{
  float t = 0.5f;
  auto pos = getFinalPosition();
  const auto &trans = tdo_->getTransition();
  const auto &style = tdo_->transitionStyle();
  auto displayC = [](char c) { return c == core::Tape::Blank ? '-' : c; };
  auto label = std::format("({}, {} ; {})", displayC(trans.readSymbol()), displayC(trans.writeSymbol()), core::dirToStr(trans.direction()));
  ImVec2 textSize = ImGui::CalcTextSize(label.c_str());
  ImVec2 labelPos = ImVec2(pos.x - textSize.x / 2, pos.y - textSize.y / 2);
  dr->AddRectFilled(ImVec2(labelPos.x - 2, labelPos.y - 1),
    ImVec2(labelPos.x + textSize.x + 2, labelPos.y + textSize.y + 1),
    IM_COL32(255, 255, 255, 200));
  dr->AddText(labelPos, style.textColor, label.c_str());
  rect_.x = labelPos.x;
  rect_.y = labelPos.y;
  rect_.w = textSize.x;
  rect_.h = textSize.y;
  ImVec2 mousePos = ImGui::GetMousePos();
  if (containsPoint(mousePos.x, mousePos.y) && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
    auto p = const_cast<ui::TransitionDrawObject *>(tdo_);
    appState_->transitionLabelEditor().openEditor(p->getTransition(),
      [=](const core::Transition &tr){
        appState_->tm().updateTransition(p->getTransition(), tr);
        p->getTransition().setDirection(tr.direction());
        p->getTransition().setReadSymbol(tr.readSymbol());
        p->getTransition().setWriteSymbol(tr.writeSymbol());
      });
  }
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

nlohmann::json ui::TransitionLabelDrawObject::toJson() const
{
  nlohmann::json j;
  j["hasManualPosition"] = hasManualPosition_;
  j["manualOffset"] = { {"x", manualOffset_.x}, {"y", manualOffset_.y} };
  return j;
}

void ui::TransitionLabelDrawObject::fromJson(const nlohmann::json &j)
{
  hasManualPosition_ = j.value("hasManualPosition", false);
  if (j.contains("manualOffset")) {
    manualOffset_.x = j["manualOffset"].value("x", 0.0f);
    manualOffset_.y = j["manualOffset"].value("y", 0.0f);
  }
}


//------------------------------------------------------------------------------------------


ui::SelectionDrawObject::SelectionDrawObject(AppState *a) : ui::DrawObject(a)
{
}

void ui::SelectionDrawObject::draw(ImDrawList *dr) const
{
  const auto &rc = boundingRect();
  ImVec2 p1{ rc.x + rc.w, rc.y + rc.h };
  ImVec2 p0{ rc.x, rc.y };
  dr->AddRect(p0, p1, Colors::gray, 0.0f, 0, 1.0f);
  dr->AddRectFilled(p0, p1, IM_COL32(200, 200, 255, 50));
}

utils::Rect ui::SelectionDrawObject::boundingRect() const
{
  auto minX = std::min(startPos_.x, endPos_.x);
  auto minY = std::min(startPos_.y, endPos_.y);
  auto maxX = std::max(startPos_.x, endPos_.x);
  auto maxY = std::max(startPos_.y, endPos_.y);
  return utils::Rect{ minX, minY, maxX - minX, maxY - minY };
}

void ui::SelectionDrawObject::translate(const ImVec2 &)
{
}

ui::Manipulator *ui::SelectionDrawObject::getOrCreateManipulator(bool bCreate)
{
  if (manipulator_) return manipulator_.get();
  if (bCreate) {
    manipulator_.reset(new ui::SelectionManipulator(this));
    return manipulator_.get();
  }
  return nullptr;
}

void ui::SelectionDrawObject::setPos0(float x, float y)
{
  startPos_ = ImVec2(x, y);
  endPos_ = startPos_;
}

void ui::SelectionDrawObject::setPos1(float x, float y)
{
  endPos_ = ImVec2(x, y);
}

void ui::SelectionDrawObject::reset()
{
  startPos_ = endPos_ = { 0, 0 };
}


//------------------------------------------------------------------------------------------


void ui::StateEditor::openEditor(const core::State &st, std::function<void(const core::State &)> f)
{
  onCommit_ = f;
  showDialog_ = true;
  state_ = std::make_unique<core::State>(st);
  std::snprintf(name_, sizeof(name_), "%s", st.name().c_str());
  switch (state_->type()) {
  case core::State::Type::START:
    type_ = 0;
    break;
  case core::State::Type::NORMAL:
    type_ = 1;
    break;
  case core::State::Type::ACCEPT:
    type_ = 2;
    break;
  case core::State::Type::REJECT:
    type_ = 3;
    break;
  }
  ImGui::OpenPopup("Edit state");
  appState_.registerPopupName("Edit state");
}

void ui::StateEditor::render()
{
  if (ImGui::BeginPopupModal("Edit state", &showDialog_, ImGuiWindowFlags_AlwaysAutoResize)) {
    ImGui::Text("Edit State Properties");
    ImGui::Separator();
    ImGui::Text("State Name:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(100);
    ImGui::PushStyleColor(ImGuiCol_Border, Colors::gray);
    ImGui::InputText("##name", name_, sizeof(name_));
    ImGui::PopStyleColor();
    ImGui::Text("State Type:");
    ImGui::SameLine();
    const char *types[] = { "Start", "Normal", "Accept", "Reject"};
    ImGui::Combo("##type", &type_, types, 4);
    if (ImGui::Button("OK")) {
      applyChanges();
      showDialog_ = false;
      ImGui::CloseCurrentPopup();
    }
    ImGui::SameLine();
    if (ImGui::Button("Cancel")) {
      showDialog_ = false;
      ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
  }
}

void ui::StateEditor::applyChanges()
{
  state_->setName(std::string(name_));
  switch (type_) {
  case 0:
    state_->setType(core::State::Type::START);
    break;
  case 1:
    state_->setType(core::State::Type::NORMAL);
    break;
  case 2:
    state_->setType(core::State::Type::ACCEPT);
    break;
  case 3:
    state_->setType(core::State::Type::REJECT);
    break;
  default:
    break;
  }
  onCommit_(*state_);
}


//------------------------------------------------------------------------------------------


void ui::TransitionLabelEditor::openEditor(const core::Transition &trans, std::function<void(const core::Transition &)> f)
{
  onCommit_ = f;
  showDialog_ = true;
  transition_ = std::make_unique<core::Transition>(trans);
  readSymbol_[0] = trans.readSymbol();
  writeSymbol_[0] = trans.writeSymbol();
  switch (trans.direction()) {
  case core::Tape::Dir::LEFT:
    direction_ = 0;
    break;
  case core::Tape::Dir::RIGHT:
    direction_ = 1;
    break;
  case core::Tape::Dir::STAY:
    direction_ = 2;
    break;
  }
  ImGui::OpenPopup("Edit transition");
  appState_.registerPopupName("Edit transition");
}

void ui::TransitionLabelEditor::render()
{
  if (ImGui::BeginPopupModal("Edit transition", &showDialog_, ImGuiWindowFlags_AlwaysAutoResize)) {

    ImGui::Text("Edit Transition Properties");
    ImGui::Separator();

    ImGui::Text("Read Symbol:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(50);
    ImGui::PushStyleColor(ImGuiCol_Border, Colors::pastelGray);
    ImGui::InputText("##read", readSymbol_, 2);
    ImGui::PopStyleColor();

    ImGui::Text("Write Symbol:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(50);
    ImGui::PushStyleColor(ImGuiCol_Border, Colors::pastelGray);
    ImGui::InputText("##write", writeSymbol_, 2);
    ImGui::PopStyleColor();

    ImGui::Text("Direction:");
    ImGui::SameLine();
    const char *dirs[] = { "Left", "Right", "Stay"};
    ImGui::Combo("##direction", &direction_, dirs, 3);

    if (ImGui::Button("OK")) {
      applyChanges();
      showDialog_ = false;
      ImGui::CloseCurrentPopup();
    }
    ImGui::SameLine();
    if (ImGui::Button("Cancel")) {
      showDialog_ = false;
      ImGui::CloseCurrentPopup();
    }

    ImGui::EndPopup();
  }
}

void ui::TransitionLabelEditor::applyChanges()
{
  transition_->setReadSymbol(readSymbol_[0]);
  transition_->setWriteSymbol(writeSymbol_[0]);
  switch (direction_) {
  case 0:
    transition_->setDirection(core::Tape::Dir::LEFT);
    break;
  case 1:
    transition_->setDirection(core::Tape::Dir::RIGHT);
    break;
  default:
    transition_->setDirection(core::Tape::Dir::STAY);
    break;
  }
  onCommit_(*transition_);
}
