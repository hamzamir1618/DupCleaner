#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "app.h"
#include "theme.h"
#include "../../third_party/IconsFontAwesome6.h"
#include <stdio.h>
#define GL_SILENCE_DEPRECATION
#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <GLES2/gl2.h>
#endif
#include <GLFW/glfw3.h>

static void glfw_error_callback(int error, const char* description) {
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

int main(int, char**) {
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        return 1;

    // Decide GL+GLSL versions
#if defined(IMGUI_IMPL_OPENGL_ES2)
    const char* glsl_version = "#version 100";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
#elif defined(__APPLE__)
    const char* glsl_version = "#version 150";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // Required on Mac
#else
    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
#endif

    // Create window with graphics context
    GLFWwindow* window = glfwCreateWindow(1280, 720, "dupcleaner", nullptr, nullptr);
    if (window == nullptr)
        return 1;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    
    ImFontConfig font_cfg;
    font_cfg.OversampleH = 2;
    font_cfg.OversampleV = 2;
    
    ImFontConfig icons_config;
    icons_config.MergeMode = true;
    icons_config.PixelSnapH = true;
    icons_config.GlyphMinAdvanceX = 14.0f;
    static const ImWchar icons_ranges[] = { ICON_MIN_FA, ICON_MAX_16_FA, 0 };

    // Resolve path relative to this source file
    std::string base_dir = std::string(__FILE__);
    size_t last_slash = base_dir.find_last_of("\\/");
    base_dir = base_dir.substr(0, last_slash);
    std::string roboto_path = base_dir + "/../../third_party/Roboto-Regular.ttf";
    std::string fa_path = base_dir + "/../../third_party/fa-solid-900.ttf";

    printf("Loading fonts from:\n");
    printf("Roboto: %s\n", roboto_path.c_str());
    printf("FA: %s\n", fa_path.c_str());
    fflush(stdout);

    dupcleaner::gui::font_regular = io.Fonts->AddFontFromFileTTF(roboto_path.c_str(), 16.0f, &font_cfg);
    io.Fonts->AddFontFromFileTTF(fa_path.c_str(), 14.0f, &icons_config, icons_ranges);

    dupcleaner::gui::font_headline = io.Fonts->AddFontFromFileTTF(roboto_path.c_str(), 24.0f, &font_cfg);
    icons_config.GlyphMinAdvanceX = 22.0f;
    io.Fonts->AddFontFromFileTTF(fa_path.c_str(), 22.0f, &icons_config, icons_ranges);

    dupcleaner::gui::font_small = io.Fonts->AddFontFromFileTTF(roboto_path.c_str(), 13.0f, &font_cfg);
    icons_config.GlyphMinAdvanceX = 12.0f;
    io.Fonts->AddFontFromFileTTF(fa_path.c_str(), 12.0f, &icons_config, icons_ranges);
    
    dupcleaner::gui::ApplyTheme();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
#ifdef __EMSCRIPTEN__
    ImGui_ImplGlfw_InstallEmscriptenCanvasResizeCallback("#canvas");
#endif
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Setup Application
    dupcleaner::gui::DupCleanerApp app;

    // Main loop
    try {
        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();

            // Start the Dear ImGui frame
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            // 1. Render the application
            app.render();

            // Rendering
            ImGui::Render();
            int display_w, display_h;
            glfwGetFramebufferSize(window, &display_w, &display_h);
            glViewport(0, 0, display_w, display_h);
            glClearColor(0.45f, 0.55f, 0.60f, 1.00f);
            glClear(GL_COLOR_BUFFER_BIT);
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

            glfwSwapBuffers(window);
        }
    } catch (const std::exception& e) {
        fprintf(stderr, "Main Loop std::exception: %s\n", e.what());
    } catch (...) {
        fprintf(stderr, "Main Loop unknown exception type\n");
    }


    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
