#include "ui/render.hpp"
#include "ui/fa_icons.hpp"
#include "app.hpp"
#include "model/turingmachine.hpp"
#include <format>
#include <functional>
#include <imgui.h>
#include <iostream>
#include "ui/drawobject.hpp"
#include <cassert>


namespace {

  std::string randomId() {
    static int counter = 0;
    return std::format("_random_q{}", counter++);
  }

  core::State _tempAddState;
  std::unique_ptr<ui::DrawObject> _tempTransition;
  float _toolbarHeight = 0;
  float _statusBarHeight = 0;
  float _tapeHeight = 0;
  float _canvasHeight = 0;

  ImVec2 posMousePosAdjusted(const ImVec2 &pt) { return { pt.x - 5, pt.y - 5 }; }

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
void handleToolbar(AppState &);

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
  appState.setCanvasOrigin(ImGui::GetCursorScreenPos());

  ImVec2 mousePos = io.MousePos;
  const bool leftClicked = ImGui::IsMouseClicked(0); // 0 = left mouse button

  if (leftClicked) {
    const bool inCanvas = topLeft.y < mousePos.y && mousePos.y < _canvasHeight;
    if (inCanvas) {
      switch (appState.mode()) {
      case AppState::Mode::SELECT:
        if (!io.KeyCtrl) {
          appState.clearManipulators();
        }
        if (auto pObj = appState.targetObject(mousePos)) {
          auto man = pObj->createManipulator();
          man->setFirstPos(mousePos);
        } else {
          appState.clearManipulators();
        }
        break;
      case AppState::Mode::ADD_STATE:
        tm.addUnconnectedState(_tempAddState);
        appState.addState(_tempAddState, posMousePosAdjusted(mousePos));
        break;
      case AppState::Mode::ADD_TRANSITION:
        if (auto pObj = appState.targetObject(mousePos); pObj && pObj->asState()) {
          const auto &state = pObj->asState()->getState();
          if (!_tempTransition) {
            core::State dummyState{ randomId() };
            appState.setStatePosition(dummyState, mousePos);
            core::Transition tr{ state, core::Alphabet::Blank, dummyState, core::Alphabet::Blank, core::Tape::Dir::RIGHT };
            auto dro = new ui::TransitionDrawObject(tr, &appState);
            _tempTransition.reset(dro);
          } else {
            auto tr = _tempTransition->asTransition()->getTransition();
            tr.setTo(state);
            tm.addTransition(tr);
            appState.addTransition(tr);
            _tempTransition.reset();
          }
        } else {
          _tempTransition.reset();
        }
        break;
      }
    }
  }

  if (ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem)) {
    if (ImGui::IsMouseDragging(0) && !appState.dragState.isDragging) {
      appState.dragState.isDragging = true;
      appState.dragState.dragStartPos = mousePos;
    }
  }

  if (appState.dragState.isDragging) {
    if (!_tempTransition) {
      auto mans = appState.getManipulators();
      for (auto m : mans) {
        m->setNextPos(mousePos, mousePos - appState.dragState.dragStartPos);
      }
      appState.dragState.dragStartPos = mousePos;
    }
  }

  if (_tempTransition) {
    const auto &tr = _tempTransition->asTransition()->getTransition();
    auto toState = tr.to();
    appState.setStatePosition(toState, posMousePosAdjusted(mousePos));
  }


  if (appState.dragState.isDragging && !ImGui::IsMouseDown(0)) {
    appState.dragState.isDragging = false;
    auto mans = appState.getManipulators();
    for (auto m : mans) {
      m->setLastPos(mousePos);
    }
  }

  // Draw machine
  appState.drawObjects();

  handleToolbar(appState);

  ImGui::Dummy(ImVec2(2000, 2000)); // Sets the scrollable area size
  ImGui::EndChild();

  _canvasHeight = ImGui::GetWindowHeight();

  ImGui::End();
}

void handleToolbar(AppState &appState)
{
  ImGuiIO &io = ImGui::GetIO();
  ImDrawList *dr = ImGui::GetWindowDrawList();
  const auto &tm = appState.tm();
  switch (appState.mode()) {
  case AppState::Mode::SELECT:
    break;
  case AppState::Mode::ADD_STATE:
    if (1) {
      auto pos = io.MousePos;
      ImU32 orange = IM_COL32(255, 165, 0, 255);
      auto s = tm.nextUniqueStateName();
      auto t = tm.states().size() == 0 ? core::State::Type::START : core::State::Type::NORMAL;
      ui::StateDrawObject::drawState(_tempAddState = core::State(s, t), posMousePosAdjusted(pos), orange, true);
      //std::cout << "handleToolbar " << s << "\n";
    }
    break;
  case AppState::Mode::ADD_TRANSITION:
    if (_tempTransition) {
      auto pos = io.MousePos;
      ImU32 orange = IM_COL32(255, 165, 0, 255);
      _tempTransition->draw();
      if (auto pObj = appState.targetObject(pos); pObj && pObj->asState()) {
        auto st{pObj->asState()->getState()};
        ui::StateDrawObject::drawState(st, pObj->centerPoint(), orange, false);
      }
    }
    break;
  }
}
