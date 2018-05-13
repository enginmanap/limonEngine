// ImGui SDL2 binding with OpenGL3
// In this binding, ImTextureID is used to store an OpenGL 'GLuint' texture identifier. Read the FAQ about ImTextureID in imgui.cpp.
// (SDL is a cross-platform general purpose library for handling windows, inputs, OpenGL/Vulkan graphics context creation, etc.)
// (GL3W is a helper library to access OpenGL functions since there is no standard header to access modern OpenGL functions easily. Alternatives are GLEW, Glad, etc.)

// You can copy and use unmodified imgui_impl_* files in your project. See main.cpp for an example of using this.
// If you use this binding you'll need to call 4 functions: ImGui_ImplXXXX_Init(), ImGui_ImplXXXX_NewFrame(), ImGui::Render() and ImGui_ImplXXXX_Shutdown().
// If you are new to ImGui, see examples/README.txt and documentation at the top of imgui.cpp.
// https://github.com/ocornut/imgui

#include "../libs/ImGui/imgui.h"

struct SDL_Window;
class GLHelper;
class GLSLProgram;
class InputHandler;
class Options;
typedef union SDL_Event SDL_Event;

class ImGuiHelper {
private:
    GLHelper* glHelper = nullptr;
    GLSLProgram* program = nullptr;
    Options* options;
    void CreateFontsTexture();
    static const char* GetClipboardText(void*);
    static void SetClipboardText(void*, const char* text);

public:
    ImGuiHelper(GLHelper* glHelper, Options* options);
    ~ImGuiHelper();
    void        NewFrame();
    bool        ProcessEvent(const InputHandler& inputHandler);
    void RenderDrawLists();

// Use if you want to reset your rendering device without losing ImGui state.
    void        InvalidateDeviceObjects();
    bool        CreateDeviceObjects();

};