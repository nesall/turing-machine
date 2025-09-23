#include "ui/render.hpp"
#include "ui/fa_icons.hpp"
#include "app.hpp"
#include "defs.hpp"
#include "model/turingmachine.hpp"
#include "ui/drawobject.hpp"
#include "ui/serializer.hpp"
#include "ui/imfilebrowser.h"
#include <functional>
#include <imgui.h>
#include <string>
#include <format>
#include <algorithm>
#include <cassert>
#include <chrono>
#include <array>


namespace {

  float _toolbarHeight = 0;
  float _statusBarHeight = 0;
  float _tapeHeight = 0;
  float _canvasHeight = 0;
  std::string _statusMessage;
  std::chrono::steady_clock::time_point _statusTime;
  std::array<char, 255> _fnameBuffer;

  void styledButton(const char *label, bool active, bool enabled, std::function<void()> onClick) {
    if (active) {
      ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(0, 255, 0, 255));
    }
    if (!enabled) {
      ImGui::BeginDisabled();
    }

    if (ImGui::Button(label)) {
      onClick();
    }
    if (!enabled) {
      ImGui::EndDisabled();
    }
    if (active) {
      ImGui::PopStyleColor();
    }
  }


  // Add to AppState or create TapeEditor class
  struct TapeEditor {
    bool isEditing = false;
    int editingIndex = 0;
    char editBuffer[2] = "";

    void startEdit(int tapeIndex, char currentValue) {
      isEditing = true;
      editingIndex = tapeIndex;
      editBuffer[0] = (currentValue == '\0') ? '_' : currentValue;
      editBuffer[1] = '\0';
    }

    void cancelEdit() {
      isEditing = false;
    }

    bool finishEdit(core::Tape &tape) {
      if (isEditing) {
        char newValue = (editBuffer[0] == '_') ? '\0' : editBuffer[0];
        tape.writeAt(editingIndex, newValue);
        isEditing = false;
        return true;
      }
      return false;
    }
  };

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

  AppState::fileBrowser().Display();
}

void drawToolbar(AppState &appState)
{
  ImGui::Begin("Toolbar", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove);
  ImGui::SetWindowPos(ImVec2(0, 0), ImGuiCond_Always);
  ImDrawList *dr = ImGui::GetWindowDrawList();

  using M = AppState::Menu;
  const auto &menu = appState.menu();
  styledButton(ICON_FA_MOUSE_POINTER " Select", menu == M::SELECT, true, [&] { appState.setMenu(M::SELECT); });
  ImGui::SameLine();
  styledButton(ICON_FA_PLUS_CIRCLE " Add State", menu == M::ADD_STATE, true, [&] { appState.setMenu(M::ADD_STATE); });
  ImGui::SameLine();
  styledButton(ICON_FA_PLUS " Add Transition", menu == M::ADD_TRANSITION, true, [&] { appState.setMenu(M::ADD_TRANSITION); });

  ImGui::SameLine();
  ImGui::Spacing();
  ImGui::SameLine();

  styledButton(ICON_FA_PLAY "", menu == M::RUNNING, true, [&] { appState.setMenu(M::RUNNING); });
  ImGui::SameLine();
  styledButton(ICON_FA_PAUSE "", menu == M::PAUSED, menu == M::RUNNING, [&] { appState.setMenu(M::PAUSED); });
  ImGui::SameLine();
  styledButton(ICON_FA_STOP "", false, menu == M::PAUSED || menu == M::RUNNING, [&] { appState.setMenu(M::SELECT); });

  ImGui::SameLine();
  ImGui::Spacing();
  ImGui::SameLine();

  if (ImGui::Button("Save")) {
    if (AppSerializer::saveToFile(appState, std::string{ _fnameBuffer.data() })) {
      _statusMessage = "Saved successfully!";
      _statusTime = std::chrono::steady_clock::now();
      appState.setWindowTitle(_fnameBuffer.data());
    } else {
      _statusMessage = "Save failed!";
      _statusTime = std::chrono::steady_clock::now();
    }
  }

  ImGui::SameLine();
  if (ImGui::InputText("##edit", _fnameBuffer.data(), _fnameBuffer.size(),
    ImGuiInputTextFlags_CharsNoBlank | ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_EnterReturnsTrue)) {
    //editor.finishEdit(tape);
  }

  ImGui::SameLine();

  // Load button  
  if (ImGui::Button("Load")) {
    if (AppSerializer::loadFromFile(appState)) {
      _statusMessage = "Loaded successfully!";
      _statusTime = std::chrono::steady_clock::now();
    } else {
      _statusMessage = "Load failed!";
      _statusTime = std::chrono::steady_clock::now();
    }
  }

  // Recent files dropdown
  ImGui::SameLine();
  if (ImGui::BeginCombo("Recent##files", "Recent Files")) {
    auto recentFiles = AppSerializer::getSavedFiles();
    for (const auto &file : recentFiles) {
      if (ImGui::Selectable(file.c_str())) {
        if (AppSerializer::loadFromFile(appState, file)) {          
          _statusMessage = "Loaded: " + file;
          _statusTime = std::chrono::steady_clock::now();
          std::fill(_fnameBuffer.begin(), _fnameBuffer.end(), '\0');
          assert(file.size() <= _fnameBuffer.size());
          for (size_t j = 0; j < file.size(); j ++) {
            _fnameBuffer[j] = file[j];
          }
        }
      }

      // Show file timestamp on hover
      if (ImGui::IsItemHovered()) {
        try {
          auto ftime = std::filesystem::last_write_time(file);
          auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
            ftime - std::filesystem::file_time_type::clock::now() +
            std::chrono::system_clock::now());
          auto time_t = std::chrono::system_clock::to_time_t(sctp);

          ImGui::BeginTooltip();
          ImGui::Text("Modified: %s", std::ctime(&time_t));
          ImGui::EndTooltip();
        } catch (...) {
          // Ignore timestamp errors
        }
      }
    }
    ImGui::EndCombo();
  }

  _toolbarHeight = ImGui::GetWindowHeight();
  ImGui::End();
}

void drawTape(AppState &appState) {
  static TapeEditor editor;

  ImGuiIO &io = ImGui::GetIO();
  const float h = 60.0f;
  const float cellSize = 40.0f;

  ImGui::Begin("Tape", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove);
  ImGui::SetWindowPos(ImVec2(0, io.DisplaySize.y - h - _statusBarHeight + 2), ImGuiCond_Always);
  ImGui::SetWindowSize(ImVec2(io.DisplaySize.x, h), ImGuiCond_Always);

  ImDrawList *dr = ImGui::GetWindowDrawList();
  auto &tm = appState.tm();
  auto &tape = tm.tape();

  const int numCells = static_cast<int>(std::ceil(io.DisplaySize.x / cellSize));
  const ImVec2 startPos = ImGui::GetWindowPos() + ImVec2{ 0, 1 };

  if (editor.isEditing) {
    if (ImGui::IsKeyPressed(ImGuiKey_Enter)) {
      editor.finishEdit(tape);
    } else if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
      editor.cancelEdit();
    }
  }

  for (int i = 0; i < numCells; ++i) {
    const bool middleCell = i == numCells / 2;
    const int tapeIndex = tape.head() + i - numCells / 2;
    const ImVec2 cellPos = ImVec2(startPos.x + i * cellSize, startPos.y);

    ImU32 cellColor = middleCell ? Colors::blue : Colors::pastelBlue;
    if (editor.isEditing && editor.editingIndex == tapeIndex) {
      cellColor = Colors::yellow;
    }

    const char c = tape.readAt(tapeIndex);
    if (c != core::Tape::Blank)
      dr->AddRectFilled(cellPos, ImVec2(cellPos.x + cellSize, cellPos.y + cellSize), utils::colorFromChar(c));

    dr->AddRectFilled(cellPos, ImVec2(cellPos.x + cellSize, cellPos.y + cellSize), IM_COL32(255, 255, 255, 50));
    dr->AddRect(cellPos, ImVec2(cellPos.x + cellSize, cellPos.y + cellSize), cellColor, 0.0f, 0, 2.0f);

    if (middleCell) {
      dr->AddTriangleFilled(
        ImVec2(cellPos.x + cellSize * 0.5f - 6, cellPos.y + cellSize - 4),
        ImVec2(cellPos.x + cellSize * 0.5f + 6, cellPos.y + cellSize - 4),
        ImVec2(cellPos.x + cellSize * 0.5f, cellPos.y + cellSize - 12),
        Colors::red);
    }

    if (editor.isEditing && editor.editingIndex == tapeIndex) {
      ImGui::SetCursorScreenPos(ImVec2(cellPos.x + cellSize * 0.3f, cellPos.y + cellSize * 0.3f));
      ImGui::PushItemWidth(cellSize * 0.4f);
      ImGui::SetKeyboardFocusHere();

      if (ImGui::InputText("##edit", editor.editBuffer, sizeof(editor.editBuffer),
        ImGuiInputTextFlags_CharsNoBlank | ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_EnterReturnsTrue)) {
        editor.finishEdit(tape);
      }

      if (!ImGui::IsItemActive() && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        ImVec2 mousePos = ImGui::GetMousePos();
        if (mousePos.x < cellPos.x || mousePos.x > cellPos.x + cellSize ||
          mousePos.y < cellPos.y || mousePos.y > cellPos.y + cellSize) {
          editor.cancelEdit();
        }
      }

      ImGui::PopItemWidth();
    } else {
      std::string cellText = (c == core::Tape::Blank) ? "_" : std::string(1, c);
      ImVec2 textSize = ImGui::CalcTextSize(cellText.c_str());
      ImVec2 textPos = ImVec2(cellPos.x + (cellSize - textSize.x) * 0.5f, cellPos.y + (cellSize - textSize.y) * 0.5f);

      ImGui::SetCursorScreenPos(textPos);
      ImGui::PushID(tapeIndex);

      if (ImGui::Selectable("##cell", false, ImGuiSelectableFlags_AllowDoubleClick, ImVec2(textSize.x, textSize.y))) {
        if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
          editor.startEdit(tapeIndex, c);
        }
      }

      ImGui::SetCursorScreenPos(textPos);
      ImGui::TextUnformatted(cellText.c_str());

      if (ImGui::IsItemHovered()) {
        ImGui::BeginTooltip();
        ImGui::Text("Cell %d: '%s'", tapeIndex, cellText.c_str());
        ImGui::Text("Double-click to edit");
        ImGui::EndTooltip();
      }

      ImGui::PopID();
    }

    ImGui::SetCursorScreenPos(ImVec2(cellPos.x + 4, cellPos.y));
    ImGui::PushStyleColor(ImGuiCol_Text, Colors::lightGray);
    ImGui::Text("%d", tapeIndex);
    ImGui::PopStyleColor();
  }

  ImGui::SetCursorScreenPos(ImVec2(startPos.x + 5, startPos.y + cellSize + 1));
  if (ImGui::SmallButton("<<")) { tape.moveLeft(); tape.moveLeft(); tape.moveLeft(); }
  ImGui::SameLine();
  if (ImGui::SmallButton("<")) { tape.moveLeft(); }
  ImGui::SameLine();

  auto fmtAlphabet = std::format("\tAlphabet: [{}]", utils::join(tape.alphabet()));
  ImGui::TextUnformatted(fmtAlphabet.c_str());

  ImGui::SameLine();
  ImGui::SetCursorPosX(io.DisplaySize.x - 51);
  if (ImGui::SmallButton(">")) { tape.moveRight(); }
  ImGui::SameLine();
  if (ImGui::SmallButton(">>")) { tape.moveRight(); tape.moveRight(); tape.moveRight(); }

  ImGui::SameLine();
  ImGui::SetCursorPosX(io.DisplaySize.x - 120);
  if (ImGui::SmallButton(ICON_FA_TIMES " Clear")) {
    for (int i = 0; i < numCells; ++i) {
      int idx = tape.head() + i - numCells / 2;
      tape.writeAt(idx, core::Tape::Blank);
    }
  }

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

  if (!_statusMessage.empty()) {
    auto now = std::chrono::steady_clock::now();
    if (std::chrono::duration_cast<std::chrono::seconds>(now - _statusTime).count() < 3) {
      ImGui::SameLine();
      ImGui::TextUnformatted(_statusMessage.c_str());
    } else {
      _statusMessage.clear();
    }
  } else {
    ImGui::TextUnformatted("Ready.");
  }

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

  ImDrawList *dr = ImGui::GetWindowDrawList();
  
  // Draw machine
  appState.drawObjects(dr);

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
      ui::StateDrawObject::drawState(appState.tempAddState_ = core::State(s, t), posOffset(pos), Colors::orange, true);
    }
    break;
  case AppState::Menu::ADD_TRANSITION:
    break;
  }
}

void drawTempTransition(AppState &appState)
{
  ImGuiIO &io = ImGui::GetIO();
  if (appState.menu() == AppState::Menu::ADD_TRANSITION || appState.dragState.isTransitionConnecting()) {
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
    appState.addState(appState.tempAddState_, posOffset(mousePos));
    break;
  case AppState::Menu::ADD_TRANSITION:
    if (!appState.dragState.isTransitionConnecting()) {
      appState.clearManipulators();
    }
    addingTransition = true;
    break;
  }
  if (auto pObj = appState.targetObject(mousePos); pObj) {
    if (pObj->asState()) {
      const auto &state = pObj->asState()->getState();
      if (addingTransition && appState.dragState.mode == DragState::Mode::NONE) {
        core::State dummyState{ nextRandomId() };
        appState.setStatePosition(dummyState, mousePos);
        core::Transition tr{ state, dummyState, core::Tape::Blank, core::Tape::Blank, core::Tape::Dir::RIGHT };
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