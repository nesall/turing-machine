#include "ui/render.hpp"
#include "ui/fa_icons.hpp"
#include "app.hpp"
#include "defs.hpp"
#include "model/turingmachine.hpp"
#include <functional>
#include <imgui.h>
#include <iostream>
#include "ui/drawobject.hpp"
#include <cassert>


namespace {

  float _toolbarHeight = 0;
  float _statusBarHeight = 0;
  float _tapeHeight = 0;
  float _canvasHeight = 0;

  //ImVec2 posMousePosAdjusted(const ImVec2 &pt) { return { pt.x - 5, pt.y - 5 }; }

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
void drawTempTransition(AppState &appState);
void canvasLeftMouseButtonClicked(AppState &appState, ImGuiIO &io);


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

  using M = AppState::Menu;
  styledButton(ICON_FA_MOUSE_POINTER " Select", appState.menu() == M::SELECT, [&] { appState.setMenu(M::SELECT); });
  ImGui::SameLine();
  styledButton(ICON_FA_PLUS_CIRCLE " Add State", appState.menu() == M::ADD_STATE, [&] { appState.setMenu(M::ADD_STATE); });
  ImGui::SameLine();
  styledButton(ICON_FA_PLUS " Add Transition", appState.menu() == M::ADD_TRANSITION, [&] { appState.setMenu(M::ADD_TRANSITION); });

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
      canvasLeftMouseButtonClicked(appState, io);
    }
  }

  if (ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem)) {
    if (ImGui::IsMouseDragging(0) && !appState.dragState.isDragging) {
      appState.dragState.isDragging = true;
      appState.dragState.dragStartPos = mousePos;
    }
  }

  if (appState.dragState.isDragging) {
    auto mans = appState.getManipulators();
    for (auto m : mans) {
      m->setNextPos(mousePos, mousePos - appState.dragState.dragStartPos);
    }
    appState.dragState.dragStartPos = mousePos;
  }

  if (appState.dragState.isDragging && !ImGui::IsMouseDown(0)) {
    if (!appState.dragState.isTransitionConnecting()) {
      appState.dragState.isDragging = false;
      auto mans = appState.getManipulators();
      for (auto m : mans) {
        m->setLastPos(mousePos);
      }
    }
  }

  // Draw machine
  appState.drawObjects();

  drawTempTransition(appState);

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
  switch (appState.menu()) {
  case AppState::Menu::SELECT:
    break;
  case AppState::Menu::ADD_STATE:
    if (1) {
      auto pos = io.MousePos;
      auto s = tm.nextUniqueStateName();
      auto t = tm.states().size() == 0 ? core::State::Type::START : core::State::Type::NORMAL;
      ui::StateDrawObject::drawState(appState._tempAddState = core::State(s, t), posOffset(pos), Colors::orange, true);
    }
    break;
  case AppState::Menu::ADD_TRANSITION:
    //appState.dragState.mode = DragState::Mode::TRANSITION_CONNECT_END;
    break;
  }
}

void drawTempTransition(AppState &appState)
{
  ImGuiIO &io = ImGui::GetIO();
  if (appState.dragState.isTransitionConnecting()) {
    auto pos = io.MousePos;
    if (auto pObj = appState.targetObject(pos); pObj && pObj->asState()) {
      ui::StateDrawObject::drawState(pObj->asState()->getState(), pObj->centerPoint(), Colors::maroon, false);
    }
  }
}

void canvasLeftMouseButtonClicked(AppState &appState, ImGuiIO &io)
{
  const auto &mousePos = io.MousePos;
  bool addingTransition = false;
  switch (appState.menu()) {
  case AppState::Menu::SELECT:
    if (!appState.dragState.isDragging) {
      if (!io.KeyCtrl) {
        appState.clearManipulators();
      }
      if (auto pObj = appState.targetObject(mousePos)) {
        auto man = pObj->createManipulator();
        man->setFirstPos(mousePos);
      } else {
        appState.clearManipulators();
      }
    }
    break;
  case AppState::Menu::ADD_STATE:
    appState.addState(appState._tempAddState, posOffset(mousePos));
    break;
  case AppState::Menu::ADD_TRANSITION:
    addingTransition = true;
    break;
  }
  if (auto pObj = appState.targetObject(mousePos); pObj) {
    if (pObj->asState()) {
      const auto &state = pObj->asState()->getState();
      if (addingTransition && appState.dragState.mode == DragState::Mode::NONE) {
        core::State dummyState{ nextRandomId() };
        appState.setStatePosition(dummyState, mousePos);
        core::Transition tr{ state, dummyState, core::Alphabet::Blank, core::Alphabet::Blank, core::Tape::Dir::RIGHT };
        auto dro = appState.addTransition(tr);
        auto man = dro->createManipulator();
        man->setFirstPos(mousePos);
        appState.dragState.isDragging = true;
        appState.dragState.dragStartPos = mousePos;
      } else if (appState.dragState.mode != DragState::Mode::NONE) {
        for (auto m : appState.getManipulators()) {
          m->setLastPos(mousePos);
        }
        appState.dragState.isDragging = false;
      }
    }
  } else {
    auto mans = appState.getManipulators();
    for (auto m : mans) {
      m->setLastPos(mousePos);
    }
    appState.dragState.isDragging = false;
  }
}