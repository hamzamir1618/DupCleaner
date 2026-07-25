#include "app.h"
#include "formatting.h"
#include "scanner.h"
#include "duplicate_finder.h"
#include "perceptual_hash.h"
#include "thumbnail_logic.h"
#include "selection_logic.h"
#include "deleter.h"
#include "scan_controller.h"
#include <set>
#include <optional>
#include "tinyfiledialogs.h"
#include "imgui.h"
#include "theme.h"
#include "../../third_party/IconsFontAwesome6.h"
#include <iostream>
#include <algorithm>

namespace dupcleaner::gui {

DupCleanerApp::DupCleanerApp() {
}

DupCleanerApp::~DupCleanerApp() {
}

void DupCleanerApp::startScan(const std::string& path) {
    exact_duplicates.clear();
    near_duplicates.groups.clear();
    exact_wasted_space = 0;
    selected_for_deletion.clear();
    
    deleter = std::make_unique<SafeDeleter>(std::filesystem::u8path(path));
    
    ScannerOptions opts;
    opts.min_size = min_size_bytes;
    scan_controller.startScan(path, opts, enable_near_duplicates, similarity_threshold);
}

void DupCleanerApp::render() {
    static bool request_delete_modal = false;
    static ScanState last_state = ScanState::Idle;
    auto state = scan_controller.getState();
    
    // Check if we just transitioned to Done
    if (state == ScanState::Done && last_state != ScanState::Done) {
        exact_duplicates = scan_controller.getExactDuplicates();
        near_duplicates = scan_controller.getNearDuplicates();
        exact_wasted_space = scan_controller.getExactWastedSpace();
        
        KeepStrategy strat = KeepStrategy::KeepOldest;
        if (keep_strategy_idx == 1) strat = KeepStrategy::KeepNewest;
        else if (keep_strategy_idx == 2) strat = KeepStrategy::KeepFirstAlphabetically;
        
        selected_for_deletion = calculateInitialSelections(exact_duplicates, near_duplicates, strat);
    }
    last_state = state;
    
    auto openFolderDialog = []() -> std::optional<std::string> {
        const char* folder = tinyfd_selectFolderDialog("Select Target Directory", "");
        if (folder != nullptr) return std::string(folder);
        return std::nullopt;
    };

    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("Duplicate Photo Cleaner", nullptr, window_flags);
    ImGui::PopStyleVar();

    // Define child window background color slightly offset from main
    ImVec4 child_bg = ImGui::GetStyleColorVec4(ImGuiCol_WindowBg);
    child_bg.x *= 0.95f; child_bg.y *= 0.95f; child_bg.z *= 0.95f; // darken slightly

    ImGui::PushStyleColor(ImGuiCol_ChildBg, child_bg);
    
    // Define margins
    float margin = 12.0f;
    ImGui::SetCursorPos(ImVec2(margin, margin));

    // SIDEBAR
    float sidebar_width = 280.0f;
    float content_height = ImGui::GetContentRegionAvail().y - margin;
    
    ImGui::BeginChild("Sidebar", ImVec2(sidebar_width, content_height), true, ImGuiWindowFlags_AlwaysUseWindowPadding);
    
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
    ImGui::Text("SOURCE");
    ImGui::PopStyleColor();
    ImGui::Separator();
    ImGui::Spacing();
    
    ImGui::PushItemWidth(-1.0f);
    ImGui::InputText("##Directory", target_dir, sizeof(target_dir));
    if (ImGui::Button(ICON_FA_FOLDER " Browse...", ImVec2(-1.0f, 0.0f))) {
        if (auto selected = openFolderDialog()) {
#ifdef _MSC_VER
            strncpy_s(target_dir, sizeof(target_dir), selected->c_str(), _TRUNCATE);
#else
            strncpy(target_dir, selected->c_str(), sizeof(target_dir) - 1);
            target_dir[sizeof(target_dir) - 1] = '\0';
#endif
        }
    }
    ImGui::PopItemWidth();
    
    ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing();
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
    ImGui::Text("OPTIONS");
    ImGui::PopStyleColor();
    ImGui::Separator();
    ImGui::Spacing();
    
    ImGui::Checkbox("Find Near-Duplicates", &enable_near_duplicates);
    if (enable_near_duplicates) {
        ImGui::Text("Similarity Threshold");
        ImGui::SetNextItemWidth(-1.0f);
        ImGui::SliderInt("##Similarity Threshold", &similarity_threshold, 0, 64);
    }
    ImGui::Spacing();
    
    ImGui::Text("Min Size (bytes)");
    ImGui::SetNextItemWidth(-1.0f);
    ImGui::InputInt("##Min Size (bytes)", &min_size_bytes);
    if (min_size_bytes < 0) min_size_bytes = 0;
    
    ImGui::Spacing();
    const char* strategies[] = { "Keep Oldest", "Keep Newest", "Keep First (Alpha)" };
    ImGui::Text("Auto-Select Strategy");
    ImGui::SetNextItemWidth(-1.0f);
    ImGui::Combo("##Auto-Select Strategy", &keep_strategy_idx, strategies, IM_ARRAYSIZE(strategies));
    
    ImGui::Spacing(); ImGui::Spacing();
    
    // Push button to bottom of sidebar
    float button_height = ImGui::GetFrameHeight() * 1.5f;
    ImGui::SetCursorPosY(ImGui::GetWindowHeight() - button_height - ImGui::GetStyle().WindowPadding.y);
    
    if (state == ScanState::Scanning) {
        PushDangerButton();
        if (ImGui::Button("Cancel Scan", ImVec2(-1.0f, button_height))) {
            scan_controller.requestCancel();
        }
        PopDangerButton();
    } else {
        PushPrimaryButton();
        if (ImGui::Button(ICON_FA_MAGNIFYING_GLASS " Start Scan", ImVec2(-1.0f, button_height))) {
            std::string path(target_dir);
            if (!path.empty()) {
                startScan(path);
            }
        }
        PopPrimaryButton();
    }
    
    ImGui::EndChild(); // Sidebar
    
    ImGui::SameLine(0.0f, margin);
    
    // MAIN CONTENT AREA
    float main_width = ImGui::GetContentRegionAvail().x - margin;
    ImGui::BeginChild("MainContent", ImVec2(main_width, content_height), false);
    
    // Status Area (Pinned to top)
    ImGui::BeginChild("StatusArea", ImVec2(0, ImGui::GetFrameHeightWithSpacing() * 3.0f), true, ImGuiWindowFlags_AlwaysUseWindowPadding);
    if (state == ScanState::Scanning) {
        PipelinePhase phase = scan_controller.getCurrentPhase();
        if (phase == PipelinePhase::Scanning) {
            ImGui::Text("Scanning... (%llu files found, %s)", (unsigned long long)scan_controller.getFilesScanned(), formatBytes(scan_controller.getBytesScanned()).c_str());
            ImGui::ProgressBar(0.0f, ImVec2(-1.0f, 0.0f));
        } else {
            const char* phase_name = "Processing";
            switch (phase) {
                case PipelinePhase::GroupingBySize: phase_name = "Grouping by Size"; break;
                case PipelinePhase::Fingerprinting: phase_name = "Hashing Files"; break;
                case PipelinePhase::Verifying: phase_name = "Verifying Byte-for-Byte"; break;
                case PipelinePhase::ComputingPerceptualHashes: phase_name = "Computing Image Hashes"; break;
                case PipelinePhase::ClusteringNearDuplicates: phase_name = "Clustering Similar Images"; break;
                case PipelinePhase::Scanning:
                case PipelinePhase::Done: break;
            }
            uintmax_t cur = scan_controller.getPhaseProgress();
            uintmax_t tot = scan_controller.getPhaseTotal();
            float fraction = (tot > 0) ? (float)cur / tot : 0.0f;
            ImGui::Text("%s... (%llu / %llu)", phase_name, (unsigned long long)cur, (unsigned long long)tot);
            ImGui::ProgressBar(fraction, ImVec2(-1.0f, 0.0f));
        }
    } else if (state == ScanState::Failed) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.4f, 0.4f, 1.0f));
        ImGui::TextWrapped("Scan Failed: %s", scan_controller.getErrorMessage().c_str());
        ImGui::PopStyleColor();
    } else if (state == ScanState::Done) {
        PushSuccessText();
        ImGui::PushFont(font_headline);
        ImGui::Text("Scan complete: %zu exact, %zu near duplicate groups found.", exact_duplicates.size(), near_duplicates.groups.size());
        ImGui::PopFont();
        PopSuccessText();
        ImGui::Text("Reclaimable space: %s", formatBytes(exact_wasted_space).c_str());
    } else {
        ImGui::Text("Ready. Adjust options in the sidebar and start a scan.");
    }
    ImGui::EndChild(); // StatusArea
    
    ImGui::Spacing();
    
    // Results Area
    float action_bar_height = (state == ScanState::Done && (!exact_duplicates.empty() || !near_duplicates.groups.empty())) ? ImGui::GetFrameHeightWithSpacing() * 2.5f : 0.0f;
    float results_height = ImGui::GetContentRegionAvail().y - action_bar_height - (action_bar_height > 0 ? margin : 0.0f);
    
    ImGui::BeginChild("ResultsArea", ImVec2(0, results_height), true);
    
    if (state == ScanState::Idle || (state == ScanState::Done && exact_duplicates.empty() && near_duplicates.groups.empty())) {
        // Centered placeholder
        std::string text = (state == ScanState::Idle) ? "Choose a folder and click Start Scan to find duplicates" : "No duplicates found in the selected folder.";
        ImVec2 text_size = ImGui::CalcTextSize(text.c_str());
        ImVec2 pos = ImGui::GetCursorPos();
        ImVec2 avail = ImGui::GetContentRegionAvail();
        ImGui::SetCursorPos(ImVec2(pos.x + (avail.x - text_size.x) * 0.5f, pos.y + (avail.y - text_size.y) * 0.5f));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
        ImGui::Text("%s", text.c_str());
        ImGui::PopStyleColor();
    } else if (state == ScanState::Done) {
        if (ImGui::BeginTabBar("ResultsTabs")) {
            if (ImGui::BeginTabItem("Exact Duplicates")) {
                renderExactDuplicates();
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Near Duplicates")) {
                renderNearDuplicates();
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }
    } else if (state == ScanState::Scanning) {
        ImVec2 text_size = ImGui::CalcTextSize("Scanning in progress...");
        ImVec2 pos = ImGui::GetCursorPos();
        ImVec2 avail = ImGui::GetContentRegionAvail();
        ImGui::SetCursorPos(ImVec2(pos.x + (avail.x - text_size.x) * 0.5f, pos.y + (avail.y - text_size.y) * 0.5f));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
        ImGui::Text("Scanning in progress...");
        ImGui::PopStyleColor();
    }
    
    ImGui::EndChild(); // ResultsArea
    
    // Action Bar
    if (action_bar_height > 0) {
        ImGui::Spacing();
        ImGui::BeginChild("ActionBar", ImVec2(0, 0), true, ImGuiWindowFlags_AlwaysUseWindowPadding);
        
        uintmax_t selected_wasted = calculateSelectedWastedSpace(selected_for_deletion, exact_duplicates, near_duplicates);
        
        // Vertically center the text
        float text_offset = (ImGui::GetContentRegionAvail().y - ImGui::GetTextLineHeight()) * 0.5f;
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + text_offset);
        ImGui::Text("%zu files selected, %s will be reclaimed", selected_for_deletion.size(), formatBytes(selected_wasted).c_str());
        
        float buttons_width = 160.0f + 130.0f + 160.0f + ImGui::GetStyle().ItemSpacing.x * 2.0f;
        ImGui::SameLine(std::max(10.0f, ImGui::GetWindowWidth() - buttons_width - 15.0f));
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() - text_offset); // Reset offset for buttons
        
        if (ImGui::Button(ICON_FA_ROTATE_LEFT " Undo Last Cleanup", ImVec2(160.0f, 0))) {
            if (deleter) {
                deleter->undoLastDeletion();
            }
        }
        ImGui::SameLine();
        if (ImGui::Button(ICON_FA_TRASH " Empty Trash", ImVec2(130.0f, 0))) {
            if (deleter) {
                deleter->emptyTrash();
            }
        }
        ImGui::SameLine();
        PushDangerButton();
        if (ImGui::Button(ICON_FA_TRASH " Clean Selected", ImVec2(160.0f, 0))) {
            if (!selected_for_deletion.empty()) {
                request_delete_modal = true;
            }
        }
        PopDangerButton();
        
        ImGui::EndChild(); // ActionBar
    }
    
    ImGui::EndChild(); // MainContent
    
    ImGui::PopStyleColor(); // ChildBg

    // Handle Popups globally but INSIDE the main window so it renders on top
    if (request_delete_modal) {
        ImGui::OpenPopup("Confirm Deletion");
        request_delete_modal = false;
    }
    
    if (ImGui::BeginPopupModal("Confirm Deletion", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        static bool move_to_trash = true;
        uintmax_t selected_wasted = calculateSelectedWastedSpace(selected_for_deletion, exact_duplicates, near_duplicates);
        ImGui::Text("Are you sure you want to delete %zu files?", selected_for_deletion.size());
        ImGui::Text("Reclaiming: %s", formatBytes(selected_wasted).c_str());
        ImGui::Separator();
        
        ImGui::Checkbox("Move to Trash (Undoable)", &move_to_trash);
        if (!move_to_trash) {
            ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "Warning: Files will be permanently deleted!");
        }
        ImGui::Spacing();
        
        if (ImGui::Button("Confirm", ImVec2(120, 0))) {
            if (deleter) {
                DeletionPlan plan;
                for (const auto& path_str : selected_for_deletion) plan.delete_files.push_back(std::filesystem::u8path(path_str));
                (void)deleter->execute(plan, move_to_trash);
                
                // Update visuals for Exact Duplicates
                for (auto it = exact_duplicates.begin(); it != exact_duplicates.end(); ) {
                    auto& group = *it;
                    group.erase(std::remove_if(group.begin(), group.end(), [&](const FileEntry& f) {
                        return selected_for_deletion.count(f.path.u8string()) > 0;
                    }), group.end());
                    
                    if (group.size() <= 1) {
                        it = exact_duplicates.erase(it);
                    } else {
                        ++it;
                    }
                }
                
                // Update visuals for Near Duplicates
                for (auto it = near_duplicates.groups.begin(); it != near_duplicates.groups.end(); ) {
                    auto& group = *it;
                    group.members.erase(std::remove_if(group.members.begin(), group.members.end(), [&](const auto& pair) {
                        return selected_for_deletion.count(pair.first.path.u8string()) > 0;
                    }), group.members.end());
                    
                    if (group.members.size() <= 1) {
                        it = near_duplicates.groups.erase(it);
                    } else {
                        ++it;
                    }
                }
                
                selected_for_deletion.clear();
            }
            ImGui::CloseCurrentPopup();
        }
        ImGui::SetItemDefaultFocus();
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    ImGui::End(); // Main Window
}
void DupCleanerApp::renderExactDuplicates() {
    float cell_width = 120.0f;
    float card_width = 380.0f;
    float cell_spacing = ImGui::GetStyle().ItemSpacing.x;
    float card_padding = ImGui::GetStyle().WindowPadding.y;
    int cols = std::max(1, (int)((card_width - card_padding * 2 + cell_spacing) / (cell_width + cell_spacing)));
    
    ImGuiStyle& style = ImGui::GetStyle();
    float window_visible_x2 = ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x;
    
    for (size_t i = 0; i < exact_duplicates.size(); ++i) {
        auto& group = exact_duplicates[i];
        uintmax_t wasted = calculateWastedSpace(group);
        bool is_image = isImageGroup(group);
        
        GLuint tex = 0;
        if (is_image && !group.empty()) {
            tex = thumbnail_cache.getTexture(group[0].path);
        }
        
        int rows = (group.size() + cols - 1) / cols;
        float item_height = cell_width + ImGui::GetFrameHeight() + ImGui::GetTextLineHeight() * 2.5f;
        float card_height = ImGui::GetTextLineHeight() * 2.0f + 35.0f + (rows * item_height) + (rows - 1) * cell_spacing + card_padding * 2;
        
        ImGui::PushID((int)i);
        ImGui::BeginChild("##GroupCard", ImVec2(card_width, card_height), true, ImGuiWindowFlags_NoScrollbar);
        
        ImGui::TextDisabled("Group %zu", i + 1);
        ImGui::SameLine(card_width - 130.0f);
        ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "Wasted: %s", formatBytes(wasted).c_str());
        ImGui::Separator();
        ImGui::Spacing();
        
        for (size_t j = 0; j < group.size(); ++j) {
            const auto& file = group[j];
            std::string path_str = file.path.u8string();
            bool is_selected = selected_for_deletion.count(path_str) > 0;
            
            ImGui::PushID(path_str.c_str());
            ImGui::BeginGroup();
            
            if (tex != 0) {
                ImGui::Image((void*)(intptr_t)tex, ImVec2(cell_width, cell_width));
            } else {
                ImGui::Dummy(ImVec2(cell_width, cell_width));
            }
            
            if (ImGui::Checkbox("##sel", &is_selected)) {
                if (is_selected) selected_for_deletion.insert(path_str);
                else selected_for_deletion.erase(path_str);
            }
            ImGui::SameLine();
            ImGui::Text("%s", formatBytes(file.size).c_str());
            
            ImGui::PushFont(font_small);
            ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + cell_width);
            std::string filename = file.path.filename().u8string();
            ImGui::Text("%s", filename.c_str());
            ImGui::PopTextWrapPos();
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("%s", path_str.c_str());
            }
            ImGui::PopFont();
            
            ImGui::EndGroup();
            ImGui::PopID();
            
            if ((j + 1) % cols != 0 && j + 1 < group.size()) {
                ImGui::SameLine();
            }
        }
        
        ImGui::EndChild();
        ImGui::PopID();
        
        float last_card_x2 = ImGui::GetItemRectMax().x;
        float next_card_x2 = last_card_x2 + style.ItemSpacing.x + card_width;
        if (i + 1 < exact_duplicates.size() && next_card_x2 < window_visible_x2) {
            ImGui::SameLine();
        } else {
            ImGui::Spacing(); // Next row of cards
        }
    }
}

void DupCleanerApp::renderNearDuplicates() {
    float cell_width = 120.0f;
    float card_width = 380.0f;
    float cell_spacing = ImGui::GetStyle().ItemSpacing.x;
    float card_padding = ImGui::GetStyle().WindowPadding.y;
    int cols = std::max(1, (int)((card_width - card_padding * 2 + cell_spacing) / (cell_width + cell_spacing)));
    
    ImGuiStyle& style = ImGui::GetStyle();
    float window_visible_x2 = ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x;
    
    for (size_t i = 0; i < near_duplicates.groups.size(); ++i) {
        auto& group = near_duplicates.groups[i];
        
        std::vector<FileEntry> entries;
        for (const auto& m : group.members) entries.push_back(m.first);
        bool is_image = isImageGroup(entries);
        
        int rows = (group.members.size() + cols - 1) / cols;
        float item_height = cell_width + ImGui::GetFrameHeight() + ImGui::GetTextLineHeight() * 2.5f;
        float card_height = ImGui::GetTextLineHeight() * 2.0f + 35.0f + (rows * item_height) + (rows - 1) * cell_spacing + card_padding * 2;
        
        ImGui::PushID((int)i);
        ImGui::BeginChild("##NearCard", ImVec2(card_width, card_height), true, ImGuiWindowFlags_NoScrollbar);
        
        ImGui::TextDisabled("Similar Images Group %zu", i + 1);
        ImGui::Separator();
        ImGui::Spacing();
        
        for (size_t j = 0; j < group.members.size(); ++j) {
            const auto& member = group.members[j];
            std::string path_str = member.first.path.u8string();
            bool is_selected = selected_for_deletion.count(path_str) > 0;
            
            ImGui::PushID(path_str.c_str());
            ImGui::BeginGroup();
            
            if (is_image) {
                GLuint tex = thumbnail_cache.getTexture(member.first.path);
                if (tex != 0) {
                    ImGui::Image((void*)(intptr_t)tex, ImVec2(cell_width, cell_width));
                } else {
                    ImGui::Dummy(ImVec2(cell_width, cell_width));
                }
            } else {
                ImGui::Dummy(ImVec2(cell_width, cell_width));
            }
            
            if (ImGui::Checkbox("##sel", &is_selected)) {
                if (is_selected) selected_for_deletion.insert(path_str);
                else selected_for_deletion.erase(path_str);
            }
            ImGui::SameLine();
            
            if (j == 0) {
                ImGui::Text("Reference");
            } else {
                int dist = PerceptualHash::hammingDistance(group.members[0].second, member.second);
                int sim = 100 - (dist * 100 / 64);
                ImGui::Text("%d%% Sim", sim);
            }
            
            ImGui::PushFont(font_small);
            ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + cell_width);
            std::string filename = member.first.path.filename().u8string();
            ImGui::Text("%s", filename.c_str());
            ImGui::PopTextWrapPos();
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("%s", path_str.c_str());
            }
            ImGui::PopFont();
            
            ImGui::EndGroup();
            ImGui::PopID();
            
            if ((j + 1) % cols != 0 && j + 1 < group.members.size()) {
                ImGui::SameLine();
            }
        }
        
        ImGui::EndChild();
        ImGui::PopID();
        
        float last_card_x2 = ImGui::GetItemRectMax().x;
        float next_card_x2 = last_card_x2 + style.ItemSpacing.x + card_width;
        if (i + 1 < near_duplicates.groups.size() && next_card_x2 < window_visible_x2) {
            ImGui::SameLine();
        } else {
            ImGui::Spacing();
        }
    }
}

} // namespace dupcleaner::gui
