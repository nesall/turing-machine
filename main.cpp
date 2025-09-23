#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <iostream>

#include "ui/render.hpp"
#include "ui/fa_icons.hpp"
#include "app.hpp"


int main() {
  // Initialize GLFW
  if (!glfwInit()) {
    std::cerr << "Failed to initialize GLFW\n";
    return -1;
  }

  // Configure GLFW
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  // Create window
  GLFWwindow *window = glfwCreateWindow(1200, 760, "Turing Machine GUI", nullptr, nullptr);
  if (!window) {
    std::cerr << "Failed to create GLFW window\n";
    glfwTerminate();
    return -1;
  }

  glfwMakeContextCurrent(window);
  glfwSwapInterval(1); // Enable vsync

  // Initialize ImGui
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO(); (void)io;
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable keyboard controls
  io.Fonts->AddFontFromFileTTF("../ui/fonts/rubik-regular.ttf", 16.0f);
  static const ImWchar icons_ranges[] = { FA_ICON_MIN, FA_ICON_MAX, 0 }; // For FontAwesome
  ImFontConfig icons_config; 
  icons_config.MergeMode = true; 
  icons_config.PixelSnapH = true;
  io.Fonts->AddFontFromFileTTF("../ui/fonts/fa-light-300.ttf", 16.0f, &icons_config, icons_ranges);

  // Setup ImGui style
  ImGui::StyleColorsLight(); // or ImGui::StyleColorsLight()

  // Setup Platform/Renderer backends
  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplOpenGL3_Init("#version 330 core");

  AppState state;

  //auto &tape = state.tm().tape();
  //char a[4] = {'A', 'B', 'C', 'D'};
  //for (int j = 0; j < 20; j ++) {
  //  tape.write(a[(j % (j % 4 + 1) + 1) % 4]);
  //  tape.moveRight();
  //}
  //tape.moveToLeftMost();

  // Test positions
  //state.setStatePosition(core::State("q0", core::State::Type::START), ImVec2(200, 200));
  //state.setStatePosition(core::State("q1"), ImVec2(400, 200));
  //state.setStatePosition(core::State("qAccept", core::State::Type::ACCEPT), ImVec2(600, 200));
  //state.setStatePosition(core::State("qReject", core::State::Type::REJECT), ImVec2(800, 200));


  // Main loop
  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();

    // Start the Dear ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    std::string title{"Turing Machine GUI"};
    if (!state.windowTitle().empty()) {
      title += " - " + state.windowTitle();
    }
    glfwSetWindowTitle(window, title.c_str());

    ui::render(state);

    // Rendering
    ImGui::Render();
    int display_w, display_h;
    glfwGetFramebufferSize(window, &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);
    glClearColor(0.61, 0.61f, 0.61f, 1.00f);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    glfwSwapBuffers(window);
  }

  // Cleanup
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();

  glfwDestroyWindow(window);
  glfwTerminate();

  return 0;
}

