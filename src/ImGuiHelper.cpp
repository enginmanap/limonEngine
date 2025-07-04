// ImGui SDL2 binding with OpenGL3
// In this binding, ImTextureID is used to store an OpenGL 'GLuint' texture identifier. Read the FAQ about ImTextureID in imgui.cpp.
// (SDL is a cross-platform general purpose library for handling windows, inputs, OpenGL/Vulkan graphics context creation, etc.)
// (GL3W is a helper library to access OpenGL functions since there is no standard header to access modern OpenGL functions easily. Alternatives are GLEW, Glad, etc.)

// You can copy and use unmodified imgui_impl_* files in your project. See main.cpp for an example of using this.
// If you use this binding you'll need to call 4 functions: ImGui_ImplXXXX_Init(), ImGui_ImplXXXX_NewFrame(), ImGui::Render() and ImGui_ImplXXXX_Shutdown().
// If you are new to ImGui, see examples/README.txt and documentation at the top of imgui.cpp.
// https://github.com/ocornut/imgui

#include "../libs/ImGui/imgui.h"
#include "ImGuiHelper.h"

#include "limonAPI/Graphics/GraphicsProgram.h"
#include "Graphics/Texture.h"
#include "InputHandler.h"
#include "limonAPI/Options.h"


// This is the main rendering function that you have to implement and provide to ImGui (via setting up 'RenderDrawListsFn' in the ImGuiIO structure)
// Note that this implementation is little overcomplicated because we are saving/setting up/restoring every OpenGL state explicitly, in order to be able to run within any OpenGL engine that doesn't do so. 
// If text or lines are blurry when integrating ImGui in your engine: in your Render function, try translating your projection matrix by (0.5f,0.5f) or (0.375f,0.375f)
void ImGuiHelper::RenderDrawLists(std::shared_ptr<GraphicsProgram> graphicsProgram)
{
    ImGui::Render();
    ImDrawData* draw_data = ImGui::GetDrawData();
    // Avoid rendering when minimized, scale coordinates for retina displays (screen coordinates != framebuffer coordinates)
    ImGuiIO& io = ImGui::GetIO();
    int fb_width = (int)(io.DisplaySize.x * io.DisplayFramebufferScale.x);
    int fb_height = (int)(io.DisplaySize.y * io.DisplayFramebufferScale.y);
    if (fb_width == 0 || fb_height == 0) {
        return;
    }
    draw_data->ScaleClipRects(io.DisplayFramebufferScale);

    glm::mat4 ortho_projection =
    {
        { 2.0f/io.DisplaySize.x, 0.0f,                   0.0f, 0.0f },
        { 0.0f,                  2.0f/-io.DisplaySize.y, 0.0f, 0.0f },
        { 0.0f,                  0.0f,                  -1.0f, 0.0f },
        {-1.0f,                  1.0f,                   0.0f, 1.0f },
    };

    graphicsProgram->setUniform("Texture", 6);
    graphicsProgram->setUniform("TextureArray", 7);
    graphicsProgram->setUniform("TextureCubeArray", 8);
    graphicsProgram->setUniform("ProjMtx",ortho_projection);
    for (int n = 0; n < draw_data->CmdListsCount; n++) {
        const ImDrawList* cmd_list = draw_data->CmdLists[n];
        const uint32_t * idx_buffer_offset = nullptr;

        std::vector<glm::vec3> positions;
        std::vector<glm::vec4> colors;
        std::vector<glm::vec2> textureCoordinates;
        for (int i = 0; i < cmd_list->VtxBuffer.Size; ++i) {
            ImU32 colorValue = cmd_list->VtxBuffer[i].col;
            glm::vec4 colorVec = glm::vec4((uint8_t)colorValue, (uint8_t)(colorValue>>8), (uint8_t)(colorValue>>16), (uint8_t)(colorValue>>24));
            colorVec = colorVec / 256;
            colors.emplace_back(colorVec);

            glm::vec2 coordinate = glm::vec2(cmd_list->VtxBuffer[i].uv.x, cmd_list->VtxBuffer[i].uv.y);
            textureCoordinates.emplace_back(coordinate);

            glm::vec3 position = glm::vec3(cmd_list->VtxBuffer[i].pos.x, cmd_list->VtxBuffer[i].pos.y, 0);
            positions.emplace_back(position);
        }
        std::vector<glm::uvec3> faces;
        for (int i = 0; i*3+2 < cmd_list->IdxBuffer.Size; i++) {
            glm::uvec3 face = glm::uvec3(cmd_list->IdxBuffer[i*3], cmd_list->IdxBuffer[i*3+1], cmd_list->IdxBuffer[i*3+2]);
            faces.push_back(face);
        }

        graphicsWrapper->updateVertexData(positions, faces, g_VboHandle, g_ElementsHandle);
        graphicsWrapper->updateExtraVertexData(colors, g_colorHandle);
        graphicsWrapper->updateVertexTextureCoordinates(textureCoordinates, g_UVHandle);

        for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++) {
            const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
            if (pcmd->UserCallback) {
                pcmd->UserCallback(cmd_list, pcmd);
            } else {
                //ImVec4 clip_rect = ImVec4(pcmd->ClipRect.x, pcmd->ClipRect.y, pcmd->ClipRect.z, pcmd->ClipRect.w);
                if (pcmd->ClipRect.x < fb_width && pcmd->ClipRect.y < fb_height && pcmd->ClipRect.z >= 0.0f && pcmd->ClipRect.w >= 0.0f) {
                    graphicsWrapper->setScissorRect(pcmd->ClipRect.x,
                                                    (fb_height - pcmd->ClipRect.w),
                                                    (pcmd->ClipRect.z - pcmd->ClipRect.x),
                                                    (pcmd->ClipRect.w - pcmd->ClipRect.y));

                    ImGuiImageWrapper* imGuiImageWrapper = reinterpret_cast<ImGuiImageWrapper*>((pcmd->TextureId));
                    switch (imGuiImageWrapper->texture->getType()) {
                        case GraphicsInterface::TextureTypes::T2D:
                            graphicsWrapper->attachTexture(imGuiImageWrapper->texture->getTextureID(), 6);
                            graphicsProgram->setUniform("isArray", 0);
                            break;
                        case GraphicsInterface::TextureTypes::T2D_ARRAY:
                            graphicsWrapper->attach2DArrayTexture(imGuiImageWrapper->texture->getTextureID(), 7);
                            graphicsProgram->setUniform("isArray", 1);
                            graphicsProgram->setUniform("layer", (float)imGuiImageWrapper->layer);
                            break;
                        case GraphicsInterface::TextureTypes::TCUBE_MAP_ARRAY:
                            graphicsWrapper->attachCubeMapArrayTexture(imGuiImageWrapper->texture->getTextureID(), 8);
                            graphicsProgram->setUniform("isArray", 2);
                            graphicsProgram->setUniform("layer", (float)imGuiImageWrapper->layer);
                            break;
                        default:
                            std::cerr << "Unsupported texture type for IMGUI" << std::endl;
                    }
                    graphicsWrapper->render(graphicsProgram->getID(), g_VaoHandle, g_ElementsHandle, pcmd->ElemCount, idx_buffer_offset);
                }
            }
            idx_buffer_offset += pcmd->ElemCount;
        }
    }

    graphicsWrapper->setScissorRect(0,
                                    0,
                                    fb_width,
                                    fb_height);
    /************* This part should be done by pipelineSetup **************/
    //graphicsWrapper->restoreLastState();
    /************* This part should be done by pipelineSetup **************/
}

const char* ImGuiHelper::GetClipboardText(void*)
{
    return SDL_GetClipboardText();
}

void ImGuiHelper::SetClipboardText(void*, const char* text)
{
    SDL_SetClipboardText(text);
}

// You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
// Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
/**
 * Returns if we processed the input. False means we didn't so game should work as no ImGui
 * @param inputHandler
 * @return
 */
bool ImGuiHelper::ProcessEvent(const InputHandler& inputHandler) {
    const InputStates& inputStates =inputHandler.getInputStates();

    ImGuiIO& io = ImGui::GetIO();
    if(io.WantCaptureKeyboard || io.WantCaptureMouse) {
        if(inputStates.getInputEvents(InputStates::Inputs::MOUSE_WHEEL_UP)) {
            g_MouseWheel = 1;
        }
        if(inputStates.getInputEvents(InputStates::Inputs::MOUSE_WHEEL_DOWN)) {
            g_MouseWheel = -1;
        }

        if(inputStates.getInputStatus(InputStates::Inputs::MOUSE_BUTTON_LEFT)) {
            g_MousePressed[0] = true;
        } else {
            g_MousePressed[0] = false;
        }
        if(inputStates.getInputStatus(InputStates::Inputs::MOUSE_BUTTON_RIGHT)) {
            g_MousePressed[1] = true;
        } else {
            g_MousePressed[1] = false;
        }
        if(inputStates.getInputStatus(InputStates::Inputs::MOUSE_BUTTON_MIDDLE)) {
            g_MousePressed[2] = true;
        } else {
            g_MousePressed[2] = false;
        }

        if(inputStates.getInputEvents(InputStates::Inputs::TEXT_INPUT)) {
            io.AddInputCharactersUTF8(inputStates.getText());
        }

        if(inputStates.getInputEvents(InputStates::Inputs::KEY_SHIFT)) {
            io.KeyShift = ((SDL_GetModState() & KMOD_SHIFT) != 0);
        }
        if(inputStates.getInputEvents(InputStates::Inputs::KEY_SUPER)) {
            io.KeySuper = ((SDL_GetModState() & KMOD_GUI) != 0);
        }
        if(inputStates.getInputEvents(InputStates::Inputs::KEY_CTRL)) {
            io.KeyCtrl = ((SDL_GetModState() & KMOD_CTRL) != 0);
        }
        if(inputStates.getInputEvents(InputStates::Inputs::KEY_ALT)) {
            io.KeyAlt = ((SDL_GetModState() & KMOD_ALT) != 0);
        }

        for (size_t i = 0; i < InputStates::keyBufferSize; i++) {
            ImGuiKey key = SDL2KeyEventToImGuiKey(i);
            if (key == ImGuiKey_None) {
                 key = SDL2KeyEventToImGuiKey(SDL_SCANCODE_TO_KEYCODE(i));
            }
            io.AddKeyEvent(key, inputStates.getRawKeyStates()[i]);
        }
        return true;
    } else {
        return false;
    }
}

ImGuiKey ImGuiHelper::SDL2KeyEventToImGuiKey(const uint32_t keycodeInt){
    SDL_Keycode keycode = keycodeInt;
    switch (keycode)
    {
        case SDLK_TAB: return ImGuiKey_Tab;
        case SDLK_LEFT: return ImGuiKey_LeftArrow;
        case SDLK_RIGHT: return ImGuiKey_RightArrow;
        case SDLK_UP: return ImGuiKey_UpArrow;
        case SDLK_DOWN: return ImGuiKey_DownArrow;
        case SDLK_PAGEUP: return ImGuiKey_PageUp;
        case SDLK_PAGEDOWN: return ImGuiKey_PageDown;
        case SDLK_HOME: return ImGuiKey_Home;
        case SDLK_END: return ImGuiKey_End;
        case SDLK_INSERT: return ImGuiKey_Insert;
        case SDLK_DELETE: return ImGuiKey_Delete;
        case SDLK_BACKSPACE: return ImGuiKey_Backspace;
        case SDLK_SPACE: return ImGuiKey_Space;
        case SDLK_RETURN: return ImGuiKey_Enter;
        case SDLK_ESCAPE: return ImGuiKey_Escape;
        case SDLK_QUOTE: return ImGuiKey_Apostrophe;
        case SDLK_COMMA: return ImGuiKey_Comma;
        case SDLK_MINUS: return ImGuiKey_Minus;
        case SDLK_PERIOD: return ImGuiKey_Period;
        case SDLK_SLASH: return ImGuiKey_Slash;
        case SDLK_SEMICOLON: return ImGuiKey_Semicolon;
        case SDLK_EQUALS: return ImGuiKey_Equal;
        case SDLK_LEFTBRACKET: return ImGuiKey_LeftBracket;
        case SDLK_BACKSLASH: return ImGuiKey_Backslash;
        case SDLK_RIGHTBRACKET: return ImGuiKey_RightBracket;
        case SDLK_BACKQUOTE: return ImGuiKey_GraveAccent;
        case SDLK_CAPSLOCK: return ImGuiKey_CapsLock;
        case SDLK_SCROLLLOCK: return ImGuiKey_ScrollLock;
        case SDLK_NUMLOCKCLEAR: return ImGuiKey_NumLock;
        case SDLK_PRINTSCREEN: return ImGuiKey_PrintScreen;
        case SDLK_PAUSE: return ImGuiKey_Pause;
        case SDLK_KP_0: return ImGuiKey_Keypad0;
        case SDLK_KP_1: return ImGuiKey_Keypad1;
        case SDLK_KP_2: return ImGuiKey_Keypad2;
        case SDLK_KP_3: return ImGuiKey_Keypad3;
        case SDLK_KP_4: return ImGuiKey_Keypad4;
        case SDLK_KP_5: return ImGuiKey_Keypad5;
        case SDLK_KP_6: return ImGuiKey_Keypad6;
        case SDLK_KP_7: return ImGuiKey_Keypad7;
        case SDLK_KP_8: return ImGuiKey_Keypad8;
        case SDLK_KP_9: return ImGuiKey_Keypad9;
        case SDLK_KP_PERIOD: return ImGuiKey_KeypadDecimal;
        case SDLK_KP_DIVIDE: return ImGuiKey_KeypadDivide;
        case SDLK_KP_MULTIPLY: return ImGuiKey_KeypadMultiply;
        case SDLK_KP_MINUS: return ImGuiKey_KeypadSubtract;
        case SDLK_KP_PLUS: return ImGuiKey_KeypadAdd;
        case SDLK_KP_ENTER: return ImGuiKey_KeypadEnter;
        case SDLK_KP_EQUALS: return ImGuiKey_KeypadEqual;
        case SDLK_LCTRL: return ImGuiKey_LeftCtrl;
        case SDLK_LSHIFT: return ImGuiKey_LeftShift;
        case SDLK_LALT: return ImGuiKey_LeftAlt;
        case SDLK_LGUI: return ImGuiKey_LeftSuper;
        case SDLK_RCTRL: return ImGuiKey_RightCtrl;
        case SDLK_RSHIFT: return ImGuiKey_RightShift;
        case SDLK_RALT: return ImGuiKey_RightAlt;
        case SDLK_RGUI: return ImGuiKey_RightSuper;
        case SDLK_APPLICATION: return ImGuiKey_Menu;
        case SDLK_0: return ImGuiKey_0;
        case SDLK_1: return ImGuiKey_1;
        case SDLK_2: return ImGuiKey_2;
        case SDLK_3: return ImGuiKey_3;
        case SDLK_4: return ImGuiKey_4;
        case SDLK_5: return ImGuiKey_5;
        case SDLK_6: return ImGuiKey_6;
        case SDLK_7: return ImGuiKey_7;
        case SDLK_8: return ImGuiKey_8;
        case SDLK_9: return ImGuiKey_9;
        case SDLK_a: return ImGuiKey_A;
        case SDLK_b: return ImGuiKey_B;
        case SDLK_c: return ImGuiKey_C;
        case SDLK_d: return ImGuiKey_D;
        case SDLK_e: return ImGuiKey_E;
        case SDLK_f: return ImGuiKey_F;
        case SDLK_g: return ImGuiKey_G;
        case SDLK_h: return ImGuiKey_H;
        case SDLK_i: return ImGuiKey_I;
        case SDLK_j: return ImGuiKey_J;
        case SDLK_k: return ImGuiKey_K;
        case SDLK_l: return ImGuiKey_L;
        case SDLK_m: return ImGuiKey_M;
        case SDLK_n: return ImGuiKey_N;
        case SDLK_o: return ImGuiKey_O;
        case SDLK_p: return ImGuiKey_P;
        case SDLK_q: return ImGuiKey_Q;
        case SDLK_r: return ImGuiKey_R;
        case SDLK_s: return ImGuiKey_S;
        case SDLK_t: return ImGuiKey_T;
        case SDLK_u: return ImGuiKey_U;
        case SDLK_v: return ImGuiKey_V;
        case SDLK_w: return ImGuiKey_W;
        case SDLK_x: return ImGuiKey_X;
        case SDLK_y: return ImGuiKey_Y;
        case SDLK_z: return ImGuiKey_Z;
        case SDLK_F1: return ImGuiKey_F1;
        case SDLK_F2: return ImGuiKey_F2;
        case SDLK_F3: return ImGuiKey_F3;
        case SDLK_F4: return ImGuiKey_F4;
        case SDLK_F5: return ImGuiKey_F5;
        case SDLK_F6: return ImGuiKey_F6;
        case SDLK_F7: return ImGuiKey_F7;
        case SDLK_F8: return ImGuiKey_F8;
        case SDLK_F9: return ImGuiKey_F9;
        case SDLK_F10: return ImGuiKey_F10;
        case SDLK_F11: return ImGuiKey_F11;
        case SDLK_F12: return ImGuiKey_F12;
        case SDLK_F13: return ImGuiKey_F13;
        case SDLK_F14: return ImGuiKey_F14;
        case SDLK_F15: return ImGuiKey_F15;
        case SDLK_F16: return ImGuiKey_F16;
        case SDLK_F17: return ImGuiKey_F17;
        case SDLK_F18: return ImGuiKey_F18;
        case SDLK_F19: return ImGuiKey_F19;
        case SDLK_F20: return ImGuiKey_F20;
        case SDLK_F21: return ImGuiKey_F21;
        case SDLK_F22: return ImGuiKey_F22;
        case SDLK_F23: return ImGuiKey_F23;
        case SDLK_F24: return ImGuiKey_F24;
        case SDLK_AC_BACK: return ImGuiKey_AppBack;
        case SDLK_AC_FORWARD: return ImGuiKey_AppForward;
        default: break;
    }
    return ImGuiKey_None;
}

void ImGuiHelper::CreateFontsTexture()
{
    // Build texture atlas
    ImGuiIO& io = ImGui::GetIO();
    unsigned char* pixels;
    int width, height;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);   // Load as RGBA 32-bits for OpenGL3 demo because it is more likely to be compatible with user's existing shader.

    fontTexture.texture = std::make_unique<Texture>(graphicsWrapper, GraphicsInterface::TextureTypes::T2D,
                                            GraphicsInterface::InternalFormatTypes::RGBA, GraphicsInterface::FormatTypes::RGBA, GraphicsInterface::DataTypes::UNSIGNED_BYTE,
                                            width, height);
    fontTexture.texture->loadData(pixels);

    g_FontTexture = fontTexture.texture->getTextureID();

    io.Fonts->TexID = ((ImTextureID)(intptr_t)&fontTexture);
}

/**
 * creates Program using shaders
 * sets up a vao with ->
 *   glEnableVertexAttribArray(g_AttribLocationPosition);
 *   glEnableVertexAttribArray(g_AttribLocationUV);
 *   glEnableVertexAttribArray(g_AttribLocationColor);
 *
 *   glVertexAttribPointer magic
 * @return
 */
bool ImGuiHelper::CreateDeviceObjects(std::shared_ptr<GraphicsProgram> graphicsProgram)
{
    graphicsWrapper->backupCurrentState();

    g_AttribLocationPosition = graphicsProgram->getAttributeLocation("PositionIMGUI");
    g_AttribLocationUV = graphicsProgram->getAttributeLocation("UV");
    g_AttribLocationColor = graphicsProgram->getAttributeLocation("Color");

    graphicsWrapper->bufferVertexData(std::vector<glm::vec3>(), std::vector<glm::uvec3>(), g_VaoHandle, g_VboHandle, g_AttribLocationPosition, g_ElementsHandle);
    graphicsWrapper->bufferExtraVertexData(std::vector<glm::vec4>(), g_VaoHandle, g_colorHandle, g_AttribLocationColor);
    graphicsWrapper->bufferVertexTextureCoordinates(std::vector<glm::vec2>(), g_VaoHandle, g_UVHandle, g_AttribLocationUV);
/*
    g_AttribLocationModelPosition = graphicsProgram->getAttributeLocation("position");
    g_AttribLocationModelUV = graphicsProgram->getAttributeLocation("textureCoordinate");
    g_AttribLocationModelNormal = graphicsProgram->getAttributeLocation("normal");

    graphicsWrapper->bufferVertexData(std::vector<glm::vec3>(), std::vector<glm::uvec3>(), g_VaoHandle, g_VboHandle, g_AttribLocationPosition, g_ElementsHandle);
    graphicsWrapper->bufferExtraVertexData(std::vector<glm::vec4>(), g_VaoHandle, g_colorHandle, g_AttribLocationColor);
    graphicsWrapper->bufferVertexTextureCoordinates(std::vector<glm::vec2>(), g_VaoHandle, g_UVHandle, g_AttribLocationUV);
*/
    CreateFontsTexture();

    graphicsWrapper->restoreLastState();
    return true;
}

void    ImGuiHelper::InvalidateDeviceObjects() {
    graphicsWrapper->freeBuffer(g_VboHandle);
    graphicsWrapper->freeBuffer(g_UVHandle);
    graphicsWrapper->freeBuffer(g_colorHandle);
    graphicsWrapper->freeVAO(g_VaoHandle);
    g_VaoHandle = g_VboHandle = g_ElementsHandle = 0;

    if (g_FontTexture)
    {
        ImGui::GetIO().Fonts->TexID = 0;
        g_FontTexture = 0;
        this->fontTexture.texture = nullptr;
    }
}

ImGuiHelper::ImGuiHelper(std::shared_ptr<AssetManager> assetManager, OptionsUtil::Options* options) : assetManager(assetManager), graphicsWrapper(assetManager->getGraphicsWrapper()), options(options) {

    context = ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    
    ImFontConfig config;
    config.OversampleH = 1;
    config.OversampleV = 1;
    
    io.Fonts->Clear();
    io.Fonts->AddFontFromFileTTF("./Data/Fonts/Helvetica-Normal.ttf", 12.f, &config);

    //io.RenderDrawListsFn = RenderDrawLists;   // Alternatively you can set this to NULL and call ImGui::GetDrawData() after ImGui::Render() to get the same ImDrawData pointer.
    io.SetClipboardTextFn = SetClipboardText;
    io.GetClipboardTextFn = GetClipboardText;
    io.ClipboardUserData = nullptr;

#ifdef _WIN32
    ImGui::GetMainViewport()->PlatformHandleRaw = options->getImeWindowHandle();
#endif

    // Setup style
    ImGui::StyleColorsClassic();
}

ImGuiHelper::~ImGuiHelper()
{
    InvalidateDeviceObjects();
    ImGui::DestroyContext(context);
}

void ImGuiHelper::NewFrame(std::shared_ptr<GraphicsProgram> graphicsProgram) {
    if (!g_FontTexture) {
        CreateDeviceObjects(graphicsProgram);
    }

    ImGuiIO& io = ImGui::GetIO();

    // Setup display size (every frame to accommodate for window resizing)
    io.DisplaySize = ImVec2((float)options->getWindowWidth(), (float)options->getWindowHeight());
    io.DisplayFramebufferScale = ImVec2(options->getWindowWidth() > 0 ? ((float)options->getDrawableWidth() / options->getWindowWidth()) : 0,
                                        options->getWindowHeight() > 0 ? ((float)options->getDrawableHeight() / options->getWindowHeight()) : 0);

    // Setup time step
    static Uint64 frequency = SDL_GetPerformanceFrequency();
    Uint64 current_time = SDL_GetPerformanceCounter();
    io.DeltaTime = (float)g_Time > 0.0 ? (float)(current_time - g_Time) / (float)frequency: (float)(1.0f / TICK_PER_SECOND);
    assert(io.DeltaTime > 0.0);
    g_Time = current_time;

    // Setup inputs
    // (we already got mouse wheel, keyboard keys & characters from SDL_PollEvent())
    int mx, my;
    Uint32 mouseMask = SDL_GetMouseState(&mx, &my);
    if (options->isIsWindowInFocus())
        io.MousePos = ImVec2((float)mx, (float)my);   // Mouse position, in pixels (set to -1,-1 if no mouse / on another screen, etc.)
    else
        io.MousePos = ImVec2(-FLT_MAX, -FLT_MAX);

    io.MouseDown[0] = g_MousePressed[0] || (mouseMask & SDL_BUTTON(SDL_BUTTON_LEFT)) != 0;		// If a mouse press event came, always pass it as "mouse held this frame", so we don't miss click-release events that are shorter than 1 frame.
    io.MouseDown[1] = g_MousePressed[1] || (mouseMask & SDL_BUTTON(SDL_BUTTON_RIGHT)) != 0;
    io.MouseDown[2] = g_MousePressed[2] || (mouseMask & SDL_BUTTON(SDL_BUTTON_MIDDLE)) != 0;
    g_MousePressed[0] = g_MousePressed[1] = g_MousePressed[2] = false;

    io.MouseWheel = g_MouseWheel;
    g_MouseWheel = 0.0f;

    // Hide OS mouse cursor if ImGui is drawing it
    SDL_ShowCursor(io.MouseDrawCursor ? 0 : 1);

    // Start the frame. This call will update the io.WantCaptureMouse, io.WantCaptureKeyboard flag that you can use to dispatch inputs (or not) to your application.
    ImGui::NewFrame();
}
