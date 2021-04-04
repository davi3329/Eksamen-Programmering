#include "../include/App.h"

auto App::glfw_errorCallback(int error, const char *description) -> void
{
    throw std::runtime_error(fmt::format("ERROR{}: {}", error, description));
}

auto App::glfw_windowResizeCallback(__attribute__((unused)) GLFWwindow *window, int width, int height) -> void
{
    auto app = (App*)glfwGetWindowUserPointer(window);
    bgfx::reset(width, height, app->vsync? BGFX_RESET_VSYNC : BGFX_RESET_NONE);
}

auto App::createWindow(bgfx::RendererType::Enum backend) -> void
{
    glfwSetErrorCallback(App::glfw_errorCallback);

    if(!glfwInit())
        throw std::runtime_error("ERROR: Could not initialize glfw");

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    m_window = glfwCreateWindow(window_width, window_height, project_name.c_str(), nullptr, nullptr);

    if(!m_window)
        throw std::runtime_error("ERROR: Failed to create window");

    // Handle window resize
    glfwSetWindowSizeCallback(m_window, App::glfw_windowResizeCallback);
    glfwSetWindowUserPointer(m_window, (void *)this);

    // Calling renderFrame before init sets main thread to be render thread
    // See: https://bkaradzic.github.io/bgfx/internals.html
    bgfx::renderFrame();

    bgfx::Init init;
    init.type = backend;  // Set vulkan rendering
    init.resolution.reset = vsync? BGFX_RESET_VSYNC : BGFX_RESET_NONE;   // Use vsync
    init.resolution.width = window_width;
    init.resolution.height = window_height;

#if BX_PLATFORM_LINUX || BX_PLATFORM_BSD
    init.platformData.ndt = glfwGetX11Display();
    init.platformData.nwh = (void*)(uintptr_t)glfwGetX11Window(m_window);
#elif BX_PLATFORM_OSX
    init.platformData.nwh = glfwGetCocoaWindow(m_window);
#elif BX_PLATFORM_WINDOWS
    init.platformData.nwh = glfwGetWin32Window(m_window);
#endif

    if (!bgfx::init(init))
        throw std::runtime_error("ERROR: Could not initialize bgfx");

    bgfx::setDebug(BGFX_DEBUG_NONE);
}

App::App(bgfx::RendererType::Enum backend, bool _vsync) : vsync(_vsync), m_viewId(0)
{
    // Create window with GLFW
    createWindow(backend);

    // Clear view with blue color(#68c4e8)
    bgfx::setViewClear(m_viewId, BGFX_CLEAR_COLOR, 0x68c4e8ff);
    bgfx::setViewRect(m_viewId, 0, 0, bgfx::BackbufferRatio::Equal);

    // Init ImGUI
    imguiCreate(22);
    ImGui_ImplGlfw_InitForOther(m_window, true);
    ImGui::StyleColorsDark();

    // Init NanoVG
    m_ctx = nvgCreate(1, m_viewId);
    auto fs = cmrc::fonts::get_filesystem();
    auto serifFile = fs.open("fonts/FreeSerif.ttf");
    auto serifData = std::string_view(serifFile.begin(), serifFile.end() - serifFile.begin());
    nvgCreateFontMem(m_ctx, "regular", (unsigned char*)serifData.data(), serifData.size(), 1);

    auto emojiFile = fs.open("fonts/NotoEmoji-Regular.ttf");
    auto emojiData = std::string_view(emojiFile.begin(), emojiFile.end() - emojiFile.begin());
    nvgCreateFontMem(m_ctx, "emoji", (unsigned char*)emojiData.data(), emojiData.size(), 1);
}

App::~App()
{
    nvgDelete(m_ctx);
    ImGui_ImplGlfw_Shutdown();
    imguiDestroy();
    bgfx::shutdown();
    glfwDestroyWindow(m_window);
    glfwTerminate();
}

///
/// \brief App::run Main loop for the game
///
auto App::run() -> void
{
    while (!glfwWindowShouldClose(m_window)) {
        glfwPollEvents();
        glfwGetFramebufferSize(m_window, &window_width, &window_height);
        // Run physics

        // Clear screen
        bgfx::setViewRect(0, 0, 0, window_width, window_height);
        bgfx::touch(m_viewId);

        // Draw ImGui
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        drawGUI();
        imguiEndFrame();

        // Draw nanovg
        nvgBeginFrame(m_ctx, window_width, window_height, 1.0f);
        drawVG();
        nvgEndFrame(m_ctx);

        // Render frame
        bgfx::frame();
    }
}

///
/// \brief App::drawVG Draws vector graphics using nanovg
///
auto App::drawVG() -> void
{
    nvgBeginPath(m_ctx);
    nvgRect(m_ctx, 40, window_height-window_height*0.3, 100, 150);
    nvgStrokeWidth(m_ctx, 15);
    nvgStrokeColor(m_ctx, nvgRGB(20, 250, 10));
    nvgLineJoin(m_ctx, NVG_BEVEL);
    nvgStroke(m_ctx);

    nvgFontSize(m_ctx, 36);
    nvgFontFace(m_ctx, "regular");
    nvgText(m_ctx, 150, window_height-window_height*0.2, "This is some text", NULL);

    nvgFontSize(m_ctx, 36*4);
    nvgFontFace(m_ctx, "emoji");
    nvgFillColor(m_ctx, nvgRGB(200, 10, 10));
    nvgText(m_ctx, 30, window_height-window_height*0.4, "😃🎉🍆", NULL);

}

///
/// \brief App::drawGUI Draws the user interface with ImGui
///
auto App::drawGUI() -> void
{
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::Begin("Stats");
    ImGui::Text(fmt::format("Backend: {}", bgfx::getRendererName( bgfx::getRendererType())).c_str());
    ImGui::Text(fmt::format("FPS: {:.1f}", ImGui::GetIO().Framerate).c_str());
//    ImGui::Text(fmt::format("CPU Time: {}", bgfx::getStats()->cpuTimeFrame).c_str());

    ImGui::Text(fmt::format("GPU Time: {:.2f}ms", (bgfx::getStats()->gpuTimeEnd-bgfx::getStats()->gpuTimeBegin)/1000000.f).c_str());


    ImGui::End();

    ImGui::SetNextWindowSize(ImVec2(window_width*0.251, window_height));
    ImGui::SetNextWindowPos(ImVec2(window_width*0.75, 0));

    ImGui::PushStyleColor(ImGuiCol_ResizeGrip, 0);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);

    bool show_window = true;
    ImGui::Begin("Inspector", &show_window,  ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoMove);
    ImGui::Text("Hello from main window!");

    ImGui::End();

    ImGui::PopStyleVar();
    ImGui::PopStyleColor();


}
