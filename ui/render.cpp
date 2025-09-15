#include "ui/render.hpp"
#include "ui/fa_icons.hpp"
#include "app.hpp"
#include "model/turingmachine.hpp"
#include <format>
#include <functional>
#include <imgui.h>
#include <cmath>

namespace {
  const float stateRadius = 30.0f;
  core::State tempAddState;
  float _toolbarHeight = 0;
  float _statusBarHeight = 0;
  float _tapeHeight = 0;
  float _canvasHeight = 0;
  ImVec2 posMousePosAdjusted(const ImVec2 &pt) { return { pt.x - 5, pt.y - 5 }; }
  // Converts absolute mouse position -> virtual canvas coordinates
  ImVec2 _origin;
  ImVec2 screenToCanvas(const ImVec2 &mouse) {
    return ImVec2(mouse.x - _origin.x + ImGui::GetScrollX(), mouse.y - _origin.y + ImGui::GetScrollY());
  }
  // Converts virtual canvas coordinates -> screen space for drawing
  ImVec2 canvasToScreen(const ImVec2 &p) {
    return ImVec2(_origin.x - ImGui::GetScrollX() + p.x, _origin.y - ImGui::GetScrollY() + p.y);
  }

  void styledButton(const char *label, bool active, std::function<void()> onClick) {
    if (active) {
      ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(0, 255, 0, 255));
    }
    if (ImGui::Button(label)) {
      onClick();
    }
    if (active) {
      ImGui::PopStyleColor();
    }
  }

} // anonymous namespace

void drawToolbar(AppState &);
void drawCanvas(AppState &appState);
void drawTape(AppState &);
void drawStatusBar(AppState &appState);
void drawStates(AppState &);
void drawTransitions(AppState &, ImDrawList *);
void handleToolbar(AppState &, ImGuiIO &, ImDrawList *);
void drawState(const core::State &st, ImVec2 pos, ImU32 clr, bool temp = false);


void ui::render(AppState &appState)
{
  ImGuiIO &io = ImGui::GetIO();

  drawToolbar(appState);
  drawStatusBar(appState);
  drawTape(appState);
  drawCanvas(appState);
}

void drawToolbar(AppState &appState)
{
  ImGui::Begin("Toolbar", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove);
  ImGui::SetWindowPos(ImVec2(0, 0), ImGuiCond_Always);
  ImDrawList *dr = ImGui::GetWindowDrawList();

  using M = AppState::Mode;
  styledButton(ICON_FA_MOUSE_POINTER " Select", appState.mode() == M::SELECT, [&] { appState.setMode(M::SELECT); });
  ImGui::SameLine();
  styledButton(ICON_FA_PLUS_CIRCLE " Add State", appState.mode() == M::ADD_STATE, [&] { appState.setMode(M::ADD_STATE); });
  ImGui::SameLine();
  styledButton(ICON_FA_PLUS " Add Transition", appState.mode() == M::ADD_TRANSITION, [&] { appState.setMode(M::ADD_TRANSITION); });

  _toolbarHeight = ImGui::GetWindowHeight();
  ImGui::End();
}

void drawTape(AppState &appState)
{
  ImGuiIO &io = ImGui::GetIO();
  float h = 48;
  ImGui::Begin("Tape", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove);
  ImGui::SetWindowPos(ImVec2(0, io.DisplaySize.y - h - _statusBarHeight + 2), ImGuiCond_Always);
  ImGui::SetWindowSize(ImVec2(io.DisplaySize.x, h), ImGuiCond_Always);
  ImGui::Text("Tape Area");
  _tapeHeight = ImGui::GetWindowHeight();
  ImGui::End();
}

void drawStatusBar(AppState &appState)
{
  ImGuiIO &io = ImGui::GetIO();
  float h = 28;
  ImGui::Begin("StatusBar", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove);
  ImGui::SetWindowPos(ImVec2(0, io.DisplaySize.y - h), ImGuiCond_Always);
  ImGui::SetWindowSize(ImVec2(io.DisplaySize.x, h), ImGuiCond_Always);

  // Example status text
  ImGui::TextUnformatted("Status: Ready");

  _statusBarHeight = ImGui::GetWindowHeight();
  ImGui::End();
}

void drawCanvas(AppState &appState)
{
  ImGuiIO &io = ImGui::GetIO();
  const ImVec2 &topLeft{0, _toolbarHeight};
  const ImVec2 &size{ io.DisplaySize.x, io.DisplaySize.y - _toolbarHeight - _tapeHeight - _statusBarHeight };
  auto &tm = appState.tm();
  ImGui::Begin("Canvas", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar);
  ImGui::SetWindowPos(topLeft, ImGuiCond_Always);
  ImGui::SetWindowSize(size, ImGuiCond_Always);
  ImGui::BeginChild("ScrollableArea", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
  _origin = ImGui::GetCursorScreenPos();

  ImDrawList *dr = ImGui::GetWindowDrawList();
  handleToolbar(appState, io, dr);

  // Update manipulators
  for (auto &manipulator : appState.manipulators()) {
    if (manipulator->handleMouse(io, dr)) {
      break; // First manipulator wins
    }
  }

  // Draw machine
  drawStates(appState);
  drawTransitions(appState, dr);

  // Draw manipulator overlays
  for (auto &manipulator : appState.manipulators()) {
    manipulator->draw(dr);
  }

  // Check for mouse click on the canvas
  ImVec2 mousePos = io.MousePos;
  bool isHovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
  bool leftClicked = ImGui::IsMouseClicked(0); // 0 = left mouse button

  if (leftClicked) {
    if (appState.mode() == AppState::Mode::ADD_STATE && 
      topLeft.y < mousePos.y && mousePos.y < _canvasHeight) {
      appState.setStatePosition(tempAddState, screenToCanvas(posMousePosAdjusted(mousePos)));
      tm.addUnconnectedState(tempAddState);
    }
  }

  ImGui::Dummy(ImVec2(2000, 2000)); // Sets the scrollable area size
  ImGui::EndChild();

  _canvasHeight = ImGui::GetWindowHeight();

  ImGui::End();
}

void drawStates(AppState &appState)
{
  const core::TuringMachine &tm = appState.tm();
  for (auto &state : tm.states()) {
    ImVec2 pos = canvasToScreen(appState.statePosition(state));
    drawState(state, /*scrollAdjustedPos*/(pos), IM_COL32(0, 0, 0, 255));
  }
}

void drawTransitions(AppState &appState, ImDrawList *dr)
{
  const auto &tm = appState.tm();
  for (auto &trans : tm.transitions()) {
    ImVec2 posFrom = appState.statePosition(trans.from());
    ImVec2 posTo = appState.statePosition(trans.to());
    ImU32 col = IM_COL32(255, 255, 0, 255);

    // Compute control points for a quadratic Bezier curve to create an arc
    ImVec2 delta = ImVec2(posTo.x - posFrom.x, posTo.y - posFrom.y);
    ImVec2 mid = ImVec2((posFrom.x + posTo.x) * 0.5f, (posFrom.y + posTo.y) * 0.5f);

    // Perpendicular direction for arc "height"
    ImVec2 perp = ImVec2(-delta.y, delta.x);
    float len = sqrtf(perp.x * perp.x + perp.y * perp.y);
    if (len > 0.0f) {
      perp.x /= len;
      perp.y /= len;
    }
    // Arc height: adjust as needed for visual style
    float arcHeight = 40.0f;
    ImVec2 control = ImVec2(mid.x + perp.x * arcHeight, mid.y + perp.y * arcHeight);

    // Draw quadratic Bezier arc
    dr->AddBezierQuadratic(posFrom, control, posTo, col, 3.0f);

    // Draw arrowhead at the end of the arc
    // Compute direction at end of Bezier: tangent from control to posTo
    ImVec2 dir = ImVec2(posTo.x - control.x, posTo.y - control.y);
    float dirLen = sqrtf(dir.x * dir.x + dir.y * dir.y);
    if (dirLen > 0.0f) {
      dir.x /= dirLen;
      dir.y /= dirLen;
      ImVec2 arrowPerp = ImVec2(-dir.y, dir.x);
      float arrowSize = 10.0f;
      ImVec2 arrowP1 = ImVec2(posTo.x - dir.x * arrowSize + arrowPerp.x * (arrowSize / 2), posTo.y - dir.y * arrowSize + arrowPerp.y * (arrowSize / 2));
      ImVec2 arrowP2 = ImVec2(posTo.x - dir.x * arrowSize - arrowPerp.x * (arrowSize / 2), posTo.y - dir.y * arrowSize - arrowPerp.y * (arrowSize / 2));
      dr->AddTriangleFilled(posTo, arrowP1, arrowP2, col);
    }

    // Draw transition label at the arc's midpoint (use Bezier midpoint for better placement)
    float t = 0.5f;
    ImVec2 bezierMid = ImVec2(
      (1 - t) * (1 - t) * posFrom.x + 2 * (1 - t) * t * control.x + t * t * posTo.x,
      (1 - t) * (1 - t) * posFrom.y + 2 * (1 - t) * t * control.y + t * t * posTo.y
    );
    auto label = std::format("{}; {}, {}", trans.readSymbol(), trans.writeSymbol(), "->");
    ImVec2 text_size = ImGui::CalcTextSize(label.c_str());
    dr->AddText(ImVec2(bezierMid.x - text_size.x / 2, bezierMid.y - text_size.y / 2), IM_COL32(0, 0, 0, 255), label.c_str());
  }
}

void handleToolbar(AppState &appState, ImGuiIO &io, ImDrawList *dr)
{
  const auto &tm = appState.tm();

  // Mode switching is handled in drawToolbar
  // Here we can handle mode-specific actions if needed
  switch (appState.mode()) {
  case AppState::Mode::SELECT:
    // Handle select mode actions
    break;
  case AppState::Mode::ADD_STATE:
    if (1) {
      auto pos = io.MousePos;
      ImU32 orange = IM_COL32(255, 165, 0, 255);
      auto s = tm.nextUniqueStateName();
      auto t = tm.states().size() == 0 ? core::State::Type::START : core::State::Type::NORMAL;
      drawState(tempAddState = core::State(s, t), posMousePosAdjusted(pos), orange, true);
    }
    break;
  case AppState::Mode::ADD_TRANSITION:
    // Handle add transition actions
    break;
  }
}

void drawState(const core::State &state, ImVec2 pos, ImU32 clr, bool temp)
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
    dr->AddCircleFilled(pos, stateRadius, fill);
    textClr = IM_COL32(255, 255, 255, 255);
  }
  dr->AddCircle(pos, stateRadius, clr, 64, 2.0f);
  ImVec2 text_size = ImGui::CalcTextSize(state.name().c_str());
  dr->AddText(ImVec2(pos.x - text_size.x / 2, pos.y - text_size.y / 2), textClr, state.name().c_str());
}
