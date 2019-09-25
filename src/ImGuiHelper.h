// ImGui SDL2 binding with OpenGL3
// In this binding, ImTextureID is used to store an OpenGL 'GLuint' texture identifier. Read the FAQ about ImTextureID in imgui.cpp.
// (SDL is a cross-platform general purpose library for handling windows, inputs, OpenGL/Vulkan graphics context creation, etc.)
// (GL3W is a helper library to access OpenGL functions since there is no standard header to access modern OpenGL functions easily. Alternatives are GLEW, Glad, etc.)

// You can copy and use unmodified imgui_impl_* files in your project. See main.cpp for an example of using this.
// If you use this binding you'll need to call 4 functions: ImGui_ImplXXXX_Init(), ImGui_ImplXXXX_NewFrame(), ImGui::Render() and ImGui_ImplXXXX_Shutdown().
// If you are new to ImGui, see examples/README.txt and documentation at the top of imgui.cpp.
// https://github.com/ocornut/imgui

#include <cstdint>
#include <string>
#include "../libs/ImGui/imgui.h"
#include "Assets/AssetManager.h"
#include "Graphics/GraphicsInterface.h"

struct SDL_Window;

class GraphicsProgram;
class InputHandler;
class Options;
typedef union SDL_Event SDL_Event;

class ImGuiHelper {

    std::unique_ptr<Texture> fontTexture;
    // ImGUI Data
    double       g_Time = 0.0f;
    bool         g_MousePressed[3] = { false, false, false };
    float        g_MouseWheel = 0.0f;
    uint32_t     g_FontTexture = 0;
    int          g_VertHandle = 0, g_FragHandle = 0;
    int          g_AttribLocationPosition = 0, g_AttribLocationUV = 0, g_AttribLocationColor = 0;
    unsigned int g_VboHandle = 0, g_VaoHandle = 0, g_ElementsHandle = 0;
    // ImGUI Data end"

    GraphicsInterface* graphicsWrapper = nullptr;
    std::shared_ptr<GraphicsProgram> program = nullptr;
    Options* options;

    ImGuiContext* context = nullptr;
    void CreateFontsTexture();
    static const char* GetClipboardText(void*);
    static void SetClipboardText(void*, const char* text);

    static void buildTreeFromAssetsRecursive(const AssetManager::AvailableAssetsNode* assetsNode, AssetManager::AssetTypes typeToShow,
                                             const std::string &customPrefix, const AssetManager::AvailableAssetsNode **selectedNode) {
        if(assetsNode->assetType == AssetManager::Asset_type_UNKNOWN) {
            return;
        }
        ImGuiTreeNodeFlags node_flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ((assetsNode == *selectedNode) ? ImGuiTreeNodeFlags_Selected : 0);
        if(assetsNode->assetType == AssetManager::AssetTypes::Asset_type_DIRECTORY) {
            if(typeToShow == AssetManager::AssetTypes::Asset_type_DIRECTORY) {
                if (ImGui::TreeNodeEx((assetsNode->name + "##"+ customPrefix + assetsNode->fullPath).c_str(),node_flags)) {
                    if(ImGui::IsItemClicked()) {
                        if(*selectedNode != assetsNode) {
                            *selectedNode = assetsNode;
                        }
                    }
                    for (size_t i = 0; i < assetsNode->children.size(); ++i) {
                        buildTreeFromAssetsRecursive(assetsNode->children[i], typeToShow, customPrefix, selectedNode);
                    }
                    ImGui::TreePop();
                }
            } else {
                if (ImGui::TreeNode((assetsNode->name + "##" + customPrefix + assetsNode->fullPath).c_str())) {
                    for (size_t i = 0; i < assetsNode->children.size(); ++i) {
                        buildTreeFromAssetsRecursive(assetsNode->children[i], typeToShow, customPrefix, selectedNode);
                    }
                    ImGui::TreePop();
                }
            }
        } else if(assetsNode->assetType == typeToShow) {
            node_flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
            ImGui::TreeNodeEx((assetsNode->name + "## " + assetsNode->fullPath).c_str(), node_flags);
            if(ImGui::IsItemClicked()) {
                if(*selectedNode != assetsNode) {
                    *selectedNode = assetsNode;
                }
            }
        }
    }

public:
    ImGuiHelper(GraphicsInterface* graphicsWrapper, Options* options);
    ~ImGuiHelper();
    void        NewFrame();
    bool        ProcessEvent(const InputHandler& inputHandler);
    void RenderDrawLists();

    /**
     * Builds a tree view for assets.
     *
     * Returns true if an element is selected, and sets the selected elements full path to selectedAsset
     *
     * @param assetsNode Root node of the asset tree to process.
     * @param typeToShow The type to show in the tree view. Only one type can be shown
     * @param customPrefix Imgui mandates each item should have unique labels. If multiple trees are going to be created, different prefixes must be passed.
     * @param selectedNode The node that was selected. Null if nothing selected, or invalid node
     */
    static void buildTreeFromAssets(const AssetManager::AvailableAssetsNode* assetsNode, AssetManager::AssetTypes typeToShow,
                                             const std::string &customPrefix, const AssetManager::AvailableAssetsNode **selectedNode) {
        ImGui::Separator();
        ImGui::BeginChild(("Asset Selector##" + customPrefix).c_str(), ImVec2(0, 300), true, ImGuiWindowFlags_HorizontalScrollbar);
        if(assetsNode != nullptr) {//possible because of filtering
            buildTreeFromAssetsRecursive(assetsNode, typeToShow, customPrefix, selectedNode);
        }
        ImGui::EndChild();
        ImGui::Separator();
    }

    static void ShowHelpMarker(const std::string &helpDescription)
    {
        ImGui::TextDisabled("(?)");
        if (ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
            ImGui::TextUnformatted(helpDescription.c_str());
            ImGui::PopTextWrapPos();
            ImGui::EndTooltip();
        }
    }

// Use if you want to reset your rendering device without losing ImGui state.
    void        InvalidateDeviceObjects();
    bool        CreateDeviceObjects();

};