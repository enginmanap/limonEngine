//
// Created by engin on 14.11.2026.
//

#include "TagInput.h"
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <unordered_map>
#include "ImGui/imgui.h"
#include "ImGui/imgui_internal.h"//BringWindowToDisplayFront, refocusing the input would otherwise raise the panel over the suggestions

namespace {
    struct TagInputState {
        char buffer[128] = {0};
        int highlighted = -1;//-1 means Enter takes the typed text
        bool refocus = false;
        bool scrollToHighlighted = false;
        bool suggestionsHovered = false;//a click on a suggestion deactivates the input first, this keeps the list up until the release
        std::vector<std::string> suggestions;
    };

    //one per material the editor showed, never cleared
    std::unordered_map<ImGuiID, TagInputState> states;

    const size_t MAX_SUGGESTION_ROWS = 8;

    int inputCallback(ImGuiInputTextCallbackData *data) {
        TagInputState *state = static_cast<TagInputState *>(data->UserData);
        const int suggestionCount = static_cast<int>(state->suggestions.size());
        switch (data->EventFlag) {
            case ImGuiInputTextFlags_CallbackHistory: {
                if (suggestionCount == 0) {
                    break;
                }
                if (data->EventKey == ImGuiKey_DownArrow) {
                    state->highlighted = (state->highlighted + 1) % suggestionCount;
                } else if (data->EventKey == ImGuiKey_UpArrow) {
                    state->highlighted = state->highlighted <= 0 ? suggestionCount - 1 : state->highlighted - 1;
                }
                state->scrollToHighlighted = true;
                break;
            }
            case ImGuiInputTextFlags_CallbackCompletion: {
                if (suggestionCount == 0) {
                    break;
                }
                const int chosen = state->highlighted < 0 ? 0 : state->highlighted;
                data->DeleteChars(0, data->BufTextLen);
                data->InsertChars(0, state->suggestions[chosen].c_str());
                state->highlighted = -1;
                break;
            }
            case ImGuiInputTextFlags_CallbackEdit: {
                state->highlighted = -1;
                break;
            }
            default:
                break;
        }
        return 0;
    }

    std::string toLower(const std::string &text) {
        std::string lowered = text;
        for (char &character : lowered) {
            character = static_cast<char>(std::tolower(static_cast<unsigned char>(character)));
        }
        return lowered;
    }
}

bool TagInput::isSuggestable(const std::string &knownTag, const std::string &typed, const std::vector<TagChip> &chips) {
    for (const TagChip &chip : chips) {
        //a partial one can still be typed, adding it goes to the rest of the meshes
        if (chip.kind != TagChip::Kind::REMOVED && !chip.isPartial() && chip.text == knownTag) {
            return false;
        }
    }
    return typed.empty() || toLower(knownTag).find(toLower(typed)) != std::string::npos;
}

std::string TagInput::countText(const TagChip &chip) {
    if (!chip.isPartial()) {
        return std::string();
    }
    return " " + std::to_string(chip.meshCount) + "/" + std::to_string(chip.totalMeshCount);
}

float TagInput::chipWidth(const TagChip &chip) {
    const ImGuiStyle &style = ImGui::GetStyle();
    float width = ImGui::CalcTextSize(chip.text.c_str()).x + ImGui::CalcTextSize(countText(chip).c_str()).x + style.FramePadding.x * 2.0f;
    if (chip.kind != TagChip::Kind::REMOVED) {
        width += ImGui::GetFontSize();
    }
    if (chip.isPartial()) {
        width += ImGui::GetFontSize();
    }
    return width;
}

bool TagInput::drawChip(const TagChip &chip, bool &closeClicked, bool &addToAllClicked) {
    const ImGuiStyle &style = ImGui::GetStyle();
    const float fontSize = ImGui::GetFontSize();
    const float width = chipWidth(chip);
    const float height = ImGui::GetFrameHeight();
    const std::string count = countText(chip);
    const float textWidth = ImGui::CalcTextSize(chip.text.c_str()).x;
    const float countWidth = ImGui::CalcTextSize(count.c_str()).x;
    const ImVec2 topLeft = ImGui::GetCursorScreenPos();
    const ImVec2 bottomRight(topLeft.x + width, topLeft.y + height);

    const bool clicked = ImGui::InvisibleButton("##chip", ImVec2(width, height));
    const bool hovered = ImGui::IsItemHovered();
    const bool removable = chip.kind != TagChip::Kind::REMOVED;
    const bool partial = chip.isPartial();
    //plus first, then the cross, each one font size wide
    const float buttonsLeft = topLeft.x + style.FramePadding.x + textWidth + countWidth + style.FramePadding.x * 0.5f;
    const float plusLeft = buttonsLeft;
    const float closeLeft = buttonsLeft + (partial ? fontSize : 0.0f);
    const float mouseX = ImGui::GetIO().MousePos.x;
    const bool plusHovered = partial && hovered && mouseX >= plusLeft && mouseX < plusLeft + fontSize;
    const bool closeHovered = removable && hovered && mouseX >= closeLeft && mouseX < closeLeft + fontSize;
    closeClicked = clicked && closeHovered;
    addToAllClicked = clicked && plusHovered;
    const bool bodyHovered = hovered && !closeHovered && !plusHovered;

    //told apart by shape, not hue: filled is the object's own, outlined comes from below, struck through is removed
    ImDrawList *drawList = ImGui::GetWindowDrawList();
    const float rounding = style.FrameRounding;
    ImU32 textColor = ImGui::GetColorU32(ImGuiCol_Text);
    switch (chip.kind) {
        case TagChip::Kind::ADDED:
            drawList->AddRectFilled(topLeft, bottomRight, ImGui::GetColorU32(bodyHovered ? ImGuiCol_ButtonHovered : ImGuiCol_Button), rounding);
            break;
        case TagChip::Kind::INHERITED:
            if (bodyHovered) {
                drawList->AddRectFilled(topLeft, bottomRight, ImGui::GetColorU32(ImGuiCol_FrameBgHovered), rounding);
            }
            drawList->AddRect(topLeft, bottomRight, ImGui::GetColorU32(ImGuiCol_Text, 0.45f), rounding);
            break;
        case TagChip::Kind::REMOVED:
            if (hovered) {
                drawList->AddRectFilled(topLeft, bottomRight, ImGui::GetColorU32(ImGuiCol_FrameBgHovered), rounding);
            }
            drawList->AddRect(topLeft, bottomRight, ImGui::GetColorU32(ImGuiCol_TextDisabled, 0.6f), rounding);
            textColor = ImGui::GetColorU32(ImGuiCol_TextDisabled);
            break;
    }
    const ImVec2 textPosition(topLeft.x + style.FramePadding.x, topLeft.y + (height - fontSize) * 0.5f);
    drawList->AddText(textPosition, textColor, chip.text.c_str());
    if (partial) {
        drawList->AddText(ImVec2(textPosition.x + textWidth, textPosition.y), ImGui::GetColorU32(ImGuiCol_TextDisabled), count.c_str());
    }
    if (chip.kind == TagChip::Kind::REMOVED) {
        const float strikeY = textPosition.y + fontSize * 0.55f;
        drawList->AddLine(ImVec2(textPosition.x, strikeY), ImVec2(textPosition.x + textWidth, strikeY), textColor, 1.0f);
    }
    const float halfMark = fontSize * 0.22f;
    const float centerY = topLeft.y + height * 0.5f;
    if (partial) {
        const ImVec2 plusCenter(plusLeft + fontSize * 0.5f, centerY);
        if (plusHovered) {
            drawList->AddCircleFilled(plusCenter, fontSize * 0.45f, ImGui::GetColorU32(ImGuiCol_ButtonHovered));
        }
        const ImU32 plusColor = ImGui::GetColorU32(plusHovered ? ImGuiCol_Text : ImGuiCol_TextDisabled);
        drawList->AddLine(ImVec2(plusCenter.x - halfMark, plusCenter.y), ImVec2(plusCenter.x + halfMark, plusCenter.y), plusColor, 1.5f);
        drawList->AddLine(ImVec2(plusCenter.x, plusCenter.y - halfMark), ImVec2(plusCenter.x, plusCenter.y + halfMark), plusColor, 1.5f);
    }
    if (removable) {
        const ImVec2 closeCenter(closeLeft + fontSize * 0.5f, centerY);
        if (closeHovered) {
            drawList->AddCircleFilled(closeCenter, fontSize * 0.45f, ImGui::GetColorU32(ImGuiCol_ButtonHovered));
        }
        const ImU32 crossColor = ImGui::GetColorU32(closeHovered ? ImGuiCol_Text : ImGuiCol_TextDisabled);
        drawList->AddLine(ImVec2(closeCenter.x - halfMark, closeCenter.y - halfMark), ImVec2(closeCenter.x + halfMark, closeCenter.y + halfMark), crossColor, 1.5f);
        drawList->AddLine(ImVec2(closeCenter.x - halfMark, closeCenter.y + halfMark), ImVec2(closeCenter.x + halfMark, closeCenter.y - halfMark), crossColor, 1.5f);
    }
    if (hovered) {
        ImGui::BeginTooltip();
        if (!chip.tooltip.empty()) {
            ImGui::TextUnformatted(chip.tooltip.c_str());
        }
        if (closeHovered) {
            ImGui::TextDisabled(chip.totalMeshCount > 1 ? "Click to remove from every mesh" : "Click to remove");
        } else if (plusHovered) {
            ImGui::TextDisabled("Click to add to every mesh");
        } else if (chip.kind == TagChip::Kind::REMOVED) {
            ImGui::TextDisabled("Click to add it back");
        }
        ImGui::EndTooltip();
    }
    return clicked;
}

TagInputResult TagInput::draw(const char *label, const std::vector<TagChip> &chips, std::set<std::string> *knownTags) {
    TagInputResult result;
    ImGui::PushID(label);
    TagInputState &state = states[ImGui::GetID("##state")];
    const ImGuiStyle &style = ImGui::GetStyle();
    const float leftEdge = ImGui::GetCursorScreenPos().x;
    const float rightEdge = leftEdge + ImGui::GetContentRegionAvail().x;

    float lineEnd = leftEdge;
    bool firstItem = true;
    for (size_t chipIndex = 0; chipIndex < chips.size(); ++chipIndex) {
        const TagChip &chip = chips[chipIndex];
        if (!firstItem && lineEnd + style.ItemSpacing.x + chipWidth(chip) <= rightEdge) {
            ImGui::SameLine();
        }
        ImGui::PushID(static_cast<int>(chipIndex));
        bool closeClicked = false;
        bool addToAllClicked = false;
        const bool clicked = drawChip(chip, closeClicked, addToAllClicked);
        ImGui::PopID();
        lineEnd = ImGui::GetItemRectMax().x;
        firstItem = false;
        if (closeClicked) {
            result.action = TagInputResult::Action::REMOVE;
            result.tag = chip.text;
        } else if (addToAllClicked || (clicked && chip.kind == TagChip::Kind::REMOVED)) {
            result.action = TagInputResult::Action::ADD;
            result.tag = chip.text;
        }
    }

    const float minimumInputWidth = ImGui::GetFontSize() * 8.0f;
    if (!firstItem && lineEnd + style.ItemSpacing.x + minimumInputWidth <= rightEdge) {
        ImGui::SameLine();
    }
    ImGui::SetNextItemWidth(rightEdge - ImGui::GetCursorScreenPos().x);

    //from last frame's text, the key callback picks from these while the input runs
    state.suggestions.clear();
    if (knownTags != nullptr) {
        const std::string typed = state.buffer;
        for (const std::string &knownTag : *knownTags) {
            if (isSuggestable(knownTag, typed, chips)) {
                state.suggestions.push_back(knownTag);
            }
        }
    }
    if (state.highlighted >= static_cast<int>(state.suggestions.size())) {
        state.highlighted = -1;
    }

    if (state.refocus) {
        ImGui::SetKeyboardFocusHere();
        state.refocus = false;
    }
    const bool wasEmpty = state.buffer[0] == 0;
    const ImGuiInputTextFlags inputFlags = ImGuiInputTextFlags_CharsNoBlank | ImGuiInputTextFlags_EnterReturnsTrue |
                                           ImGuiInputTextFlags_CallbackHistory | ImGuiInputTextFlags_CallbackCompletion |
                                           ImGuiInputTextFlags_CallbackEdit;
    const bool entered = ImGui::InputTextWithHint("##newTag", "add tag", state.buffer, sizeof(state.buffer), inputFlags, inputCallback, &state);
    const bool inputActive = ImGui::IsItemActive();
    const ImVec2 inputMin = ImGui::GetItemRectMin();
    const ImVec2 inputMax = ImGui::GetItemRectMax();

    if (inputActive && wasEmpty && result.action == TagInputResult::Action::NONE && ImGui::IsKeyPressed(ImGuiKey_Backspace, false)) {
        for (size_t chipIndex = chips.size(); chipIndex > 0; --chipIndex) {
            if (chips[chipIndex - 1].kind != TagChip::Kind::REMOVED) {
                result.action = TagInputResult::Action::REMOVE;
                result.tag = chips[chipIndex - 1].text;
                break;
            }
        }
    }
    if (entered) {
        const std::string chosen = state.highlighted >= 0 ? state.suggestions[state.highlighted] : std::string(state.buffer);
        if (!chosen.empty()) {
            result.action = TagInputResult::Action::ADD;
            result.tag = chosen;
            state.refocus = true;//Enter deactivates the input, the next tag would need another click
        }
        state.buffer[0] = 0;
        state.highlighted = -1;
    }

    const bool showSuggestions = (inputActive || state.suggestionsHovered) && !entered && !state.suggestions.empty();
    state.suggestionsHovered = false;
    if (showSuggestions) {
        const size_t rowCount = std::min(state.suggestions.size(), MAX_SUGGESTION_ROWS);
        const float listHeight = static_cast<float>(rowCount) * ImGui::GetTextLineHeightWithSpacing() + style.WindowPadding.y * 2.0f;
        ImGui::SetNextWindowPos(ImVec2(inputMin.x, inputMax.y));
        ImGui::SetNextWindowSize(ImVec2(std::max(inputMax.x - inputMin.x, ImGui::GetFontSize() * 12.0f), listHeight));
        char windowName[48];
        std::snprintf(windowName, sizeof(windowName), "##tagSuggestions%08X", ImGui::GetID("##suggestions"));
        const ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                                             ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing |
                                             ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoCollapse;
        if (ImGui::Begin(windowName, nullptr, windowFlags)) {
            ImGui::BringWindowToDisplayFront(ImGui::GetCurrentWindow());
            for (size_t suggestionIndex = 0; suggestionIndex < state.suggestions.size(); ++suggestionIndex) {
                const bool highlighted = static_cast<int>(suggestionIndex) == state.highlighted;
                if (ImGui::Selectable(state.suggestions[suggestionIndex].c_str(), highlighted)) {
                    result.action = TagInputResult::Action::ADD;
                    result.tag = state.suggestions[suggestionIndex];
                    state.buffer[0] = 0;
                    state.highlighted = -1;
                    state.refocus = true;
                }
                if (highlighted && state.scrollToHighlighted) {
                    ImGui::SetScrollHereY();
                }
            }
            state.scrollToHighlighted = false;
            state.suggestionsHovered = ImGui::IsWindowHovered();
        }
        ImGui::End();
    }

    if (result.action == TagInputResult::Action::ADD && knownTags != nullptr) {
        knownTags->insert(result.tag);
    }
    ImGui::PopID();
    return result;
}
