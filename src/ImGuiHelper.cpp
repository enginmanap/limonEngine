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

#include "API/Graphics/GraphicsProgram.h"
#include "Graphics/Texture.h"
#include "InputHandler.h"
#include "Options.h"


// This is the main rendering function that you have to implement and provide to ImGui (via setting up 'RenderDrawListsFn' in the ImGuiIO structure)
// Note that this implementation is little overcomplicated because we are saving/setting up/restoring every OpenGL state explicitly, in order to be able to run within any OpenGL engine that doesn't do so. 
// If text or lines are blurry when integrating ImGui in your engine: in your Render function, try translating your projection matrix by (0.5f,0.5f) or (0.375f,0.375f)
void ImGuiHelper::RenderDrawLists()
{
    ImGui::Render();
    ImDrawData* draw_data = ImGui::GetDrawData();
    // Avoid rendering when minimized, scale coordinates for retina displays (screen coordinates != framebuffer coordinates)
    ImGuiIO& io = ImGui::GetIO();
    int fb_width = (int)(io.DisplaySize.x * io.DisplayFramebufferScale.x);
    int fb_height = (int)(io.DisplaySize.y * io.DisplayFramebufferScale.y);
    if (fb_width == 0 || fb_height == 0)
        return;
    draw_data->ScaleClipRects(io.DisplayFramebufferScale);

    glm::mat4 ortho_projection =
    {
        { 2.0f/io.DisplaySize.x, 0.0f,                   0.0f, 0.0f },
        { 0.0f,                  2.0f/-io.DisplaySize.y, 0.0f, 0.0f },
        { 0.0f,                  0.0f,                  -1.0f, 0.0f },
        {-1.0f,                  1.0f,                   0.0f, 1.0f },
    };

    program->setUniform("Texture", 1);
    program->setUniform("TextureArray", 2);
    program->setUniform("TextureCubeArray", 3);
    program->setUniform("ProjMtx",ortho_projection);

    for (int n = 0; n < draw_data->CmdListsCount; n++)
    {
        const ImDrawList* cmd_list = draw_data->CmdLists[n];
        const uint32_t * idx_buffer_offset = 0;

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
        std::vector<glm::mediump_uvec3> faces;
        for (int i = 0; i*3+2 < cmd_list->IdxBuffer.Size; i++) {
            glm::mediump_uvec3 face = glm::mediump_uvec3(cmd_list->IdxBuffer[i*3], cmd_list->IdxBuffer[i*3+1], cmd_list->IdxBuffer[i*3+2]);
            faces.push_back(face);
        }

        graphicsWrapper->updateVertexData(positions, faces, g_VaoHandle, g_VboHandle, g_ElementsHandle);
        graphicsWrapper->updateExtraVertexData(colors, g_VaoHandle, g_colorHandle);
        graphicsWrapper->updateVertexTextureCoordinates(textureCoordinates, g_VaoHandle, g_UVHandle);

        for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++)
        {
            const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
            if (pcmd->UserCallback)
            {
                pcmd->UserCallback(cmd_list, pcmd);
            }
            else
            {
                ImGuiImageWrapper* imGuiImageWrapper = static_cast<ImGuiImageWrapper*>((pcmd->TextureId));
                switch (imGuiImageWrapper->texture->getType()) {
                    case GraphicsInterface::TextureTypes::T2D:
                        graphicsWrapper->attachTexture(imGuiImageWrapper->texture->getTextureID(), 1);
                        program->setUniform("isArray", 0);
                        break;
                    case GraphicsInterface::TextureTypes::T2D_ARRAY:
                        graphicsWrapper->attach2DArrayTexture(imGuiImageWrapper->texture->getTextureID(), 2);
                        program->setUniform("isArray", 1);
                        program->setUniform("layer", (float)imGuiImageWrapper->layer);
                        break;
                    case GraphicsInterface::TextureTypes::TCUBE_MAP_ARRAY:
                        graphicsWrapper->attachCubeMapArrayTexture(imGuiImageWrapper->texture->getTextureID(), 3);
                        program->setUniform("isArray", 2);
                        program->setUniform("layer", (float)imGuiImageWrapper->layer);
                        break;
                    default:
                        std::cerr << "Unsupported texture type for IMGUI" << std::endl;
                }

                graphicsWrapper->setScissorRect((int)pcmd->ClipRect.x,
                        (int)(fb_height - pcmd->ClipRect.w),
                        (int)(pcmd->ClipRect.z - pcmd->ClipRect.x),
                        (int)(pcmd->ClipRect.w - pcmd->ClipRect.y));
                graphicsWrapper->render(program->getID(), g_VaoHandle, g_ElementsHandle, pcmd->ElemCount, idx_buffer_offset);
            }
            idx_buffer_offset += pcmd->ElemCount;
        }
    }

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


        if(inputStates.keyBufferSize > sizeof(io.KeysDown)) {
            memcpy(io.KeysDown, inputStates.getRawKeyStates(), inputStates.keyBufferSize);
        } else {
            memcpy(io.KeysDown, inputStates.getRawKeyStates(), sizeof(io.KeysDown));
        }
        return true;
    } else {
        return false;
    }
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

    io.Fonts->TexID = &fontTexture;
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
bool ImGuiHelper::CreateDeviceObjects()
{
    graphicsWrapper->backupCurrentState();

    program = std::make_shared<GraphicsProgram>(assetManager.get(),"./Data/Shaders/ImGui/vertex.glsl",
                                                     "./Data/Shaders/ImGui/fragment.glsl", true);

    g_AttribLocationPosition = program->getAttributeLocation("Position");
    g_AttribLocationUV = program->getAttributeLocation("UV");
    g_AttribLocationColor = program->getAttributeLocation("Color");

    graphicsWrapper->bufferVertexData(std::vector<glm::vec3>(), std::vector<glm::mediump_uvec3>(), g_VaoHandle, g_VboHandle, g_AttribLocationPosition, g_ElementsHandle);
    graphicsWrapper->bufferExtraVertexData(std::vector<glm::vec4>(), g_VaoHandle, g_colorHandle, g_AttribLocationColor);
    graphicsWrapper->bufferVertexTextureCoordinates(std::vector<glm::vec2>(), g_VaoHandle, g_UVHandle, g_AttribLocationUV);
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

    program = nullptr;

    if (g_FontTexture)
    {
        ImGui::GetIO().Fonts->TexID = nullptr;
        g_FontTexture = 0;
        this->fontTexture.texture = nullptr;
    }
}

ImGuiHelper::ImGuiHelper(std::shared_ptr<AssetManager> assetManager, Options* options) : assetManager(assetManager), graphicsWrapper(assetManager->getGraphicsWrapper()), options(options) {

    context = ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.KeyMap[ImGuiKey_Tab] = SDLK_TAB;                     // Keyboard mapping. ImGui will use those indices to peek into the io.KeyDown[] array.
    io.KeyMap[ImGuiKey_LeftArrow] = SDL_SCANCODE_LEFT;
    io.KeyMap[ImGuiKey_RightArrow] = SDL_SCANCODE_RIGHT;
    io.KeyMap[ImGuiKey_UpArrow] = SDL_SCANCODE_UP;
    io.KeyMap[ImGuiKey_DownArrow] = SDL_SCANCODE_DOWN;
    io.KeyMap[ImGuiKey_PageUp] = SDL_SCANCODE_PAGEUP;
    io.KeyMap[ImGuiKey_PageDown] = SDL_SCANCODE_PAGEDOWN;
    io.KeyMap[ImGuiKey_Home] = SDL_SCANCODE_HOME;
    io.KeyMap[ImGuiKey_End] = SDL_SCANCODE_END;
    io.KeyMap[ImGuiKey_Delete] = SDLK_DELETE;
    io.KeyMap[ImGuiKey_Backspace] = SDLK_BACKSPACE;
    io.KeyMap[ImGuiKey_Enter] = SDLK_RETURN;
    io.KeyMap[ImGuiKey_Escape] = SDLK_ESCAPE;
    io.KeyMap[ImGuiKey_A] = SDLK_a;
    io.KeyMap[ImGuiKey_C] = SDLK_c;
    io.KeyMap[ImGuiKey_V] = SDLK_v;
    io.KeyMap[ImGuiKey_X] = SDLK_x;
    io.KeyMap[ImGuiKey_Y] = SDLK_y;
    io.KeyMap[ImGuiKey_Z] = SDLK_z;

    //io.RenderDrawListsFn = RenderDrawLists;   // Alternatively you can set this to NULL and call ImGui::GetDrawData() after ImGui::Render() to get the same ImDrawData pointer.
    io.SetClipboardTextFn = SetClipboardText;
    io.GetClipboardTextFn = GetClipboardText;
    io.ClipboardUserData = NULL;

#ifdef _WIN32
    io.ImeWindowHandle = options->getImeWindowHandle();
#endif

    // Setup style
    ImGui::StyleColorsClassic();
}

ImGuiHelper::~ImGuiHelper()
{
    InvalidateDeviceObjects();
    ImGui::DestroyContext(context);
}

void ImGuiHelper::NewFrame() {
    if (!g_FontTexture) {
        CreateDeviceObjects();
    }

    ImGuiIO& io = ImGui::GetIO();

    // Setup display size (every frame to accommodate for window resizing)
    io.DisplaySize = ImVec2((float)options->getWindowWidth(), (float)options->getWindowHeight());
    io.DisplayFramebufferScale = ImVec2(options->getWindowWidth() > 0 ? ((float)options->getDrawableWidth() / options->getWindowWidth()) : 0,
                                        options->getWindowHeight() > 0 ? ((float)options->getWindowHeight() / options->getWindowHeight()) : 0);

    // Setup time step
    Uint32	time = SDL_GetTicks();
    double current_time = time / 1000.0;
    io.DeltaTime = g_Time > 0.0 ? (float)(current_time - g_Time) : (float)(1.0f / 60.0f);
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
