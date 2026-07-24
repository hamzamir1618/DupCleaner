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
    
    deleter = std::make_unique<SafeDeleter>(path);
    
    scan_controller.startScan(path);
}

void DupCleanerApp::render() {
    auto state = scan_controller.getState();
    
    // Check if we just transitioned to Done
    if (state == ScanState::Done && exact_duplicates.empty() && near_duplicates.groups.empty() && scan_controller.getExactWastedSpace() > 0) {
        // Fetch results once (since exact_wasted_space is > 0 or we just assume they are available)
        exact_duplicates = scan_controller.getExactDuplicates();
        near_duplicates = scan_controller.getNearDuplicates();
        exact_wasted_space = scan_controller.getExactWastedSpace();
        
        selected_for_deletion = calculateInitialSelections(exact_duplicates, near_duplicates, KeepStrategy::KeepOldest);
    }
    
    // Handle empty results specifically
    if (state == ScanState::Done && exact_duplicates.empty() && near_duplicates.groups.empty()) {
        // It could just be an empty scan, fetch to make sure.
        exact_duplicates = scan_controller.getExactDuplicates();
        near_duplicates = scan_controller.getNearDuplicates();
        exact_wasted_space = scan_controller.getExactWastedSpace();
    }
    
    auto openFolderDialog = []() -> std::optional<std::string> {
        const char* folder = tinyfd_selectFolderDialog("Select Target Directory", "");
        if (folder != nullptr) {
            return std::string(folder);
        }
        return std::nullopt;
    };

    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse;

    ImGui::Begin("Duplicate Photo Cleaner", nullptr, window_flags);

    // Top Bar
    ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x - 200.0f);
    ImGui::InputText("##Directory", target_dir, sizeof(target_dir));
    ImGui::PopItemWidth();
    
    ImGui::SameLine();
    if (ImGui::Button("Browse...")) {
        if (auto selected = openFolderDialog()) {
#ifdef _MSC_VER
            strncpy_s(target_dir, sizeof(target_dir), selected->c_str(), _TRUNCATE);
#else
            strncpy(target_dir, selected->c_str(), sizeof(target_dir) - 1);
            target_dir[sizeof(target_dir) - 1] = '\0';
#endif
        }
    }
    ImGui::SameLine();
    
    if (state == ScanState::Scanning) {
        if (ImGui::Button("Cancel Scan")) {
            scan_controller.requestCancel();
        }
    } else {
        if (ImGui::Button("Scan")) {
            std::string path(target_dir);
            if (!path.empty()) {
                startScan(path);
            }
        }
    }
    
    ImGui::Spacing();
    
    // Status Bar
    if (state == ScanState::Scanning) {
        ImGui::Text("Scanning... (%llu files found, %s)", (unsigned long long)scan_controller.getFilesScanned(), formatBytes(scan_controller.getBytesScanned()).c_str());
    } else if (state == ScanState::Failed) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.4f, 0.4f, 1.0f));
        ImGui::TextWrapped("Scan Failed: %s", scan_controller.getErrorMessage().c_str());
        ImGui::PopStyleColor();
    } else if (state == ScanState::Done) {
        ImGui::Text("Scan complete: %zu exact, %zu near duplicate groups found. %s reclaimable.", exact_duplicates.size(), near_duplicates.groups.size(), formatBytes(exact_wasted_space).c_str());
    } else {
        ImGui::Text("Ready. Select a directory and click Scan.");
    }
    
    ImGui::Separator();
    
    // Main Content Area (Leave room for Action Bar)
    float bottom_bar_height = ImGui::GetFrameHeightWithSpacing() * 2.5f;
    ImGui::BeginChild("MainContent", ImVec2(0, -bottom_bar_height), true);

    if (state == ScanState::Done && (!exact_duplicates.empty() || !near_duplicates.groups.empty())) {
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
    }
    ImGui::EndChild();
    
    ImGui::Separator();
    
    // Bottom Action Bar
    if (state == ScanState::Done && (!exact_duplicates.empty() || !near_duplicates.groups.empty())) {
        uintmax_t selected_wasted = calculateSelectedWastedSpace(selected_for_deletion, exact_duplicates, near_duplicates);
        ImGui::Text("%zu files selected, %s will be reclaimed", selected_for_deletion.size(), formatBytes(selected_wasted).c_str());
        
        if (ImGui::Button("Clean Selected") && !selected_for_deletion.empty()) {
            ImGui::OpenPopup("Confirm Deletion");
        }
        ImGui::SameLine();
        if (ImGui::Button("Undo Last Cleanup")) {
            if (deleter) {
                deleter->undoLastDeletion();
                // Refresh scan? For now just rely on undo.
            }
        }
        
        // Confirmation Modal
        if (ImGui::BeginPopupModal("Confirm Deletion", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("Are you sure you want to delete %zu files?", selected_for_deletion.size());
            ImGui::Text("Reclaiming: %s", formatBytes(selected_wasted).c_str());
            ImGui::Separator();
            
            if (ImGui::Button("Confirm", ImVec2(120, 0))) {
                if (deleter) {
                    DeletionPlan plan;
                    for (const auto& path_str : selected_for_deletion) {
                        plan.delete_files.push_back(std::filesystem::path(path_str));
                    }
                    (void)deleter->execute(plan, true); // moveToTrash = true
                    // Ignore failure details in basic GUI for now
                    
                    // Clear state after deletion
                    exact_duplicates.clear();
                    near_duplicates.groups.clear();
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

        ImGui::Separator();

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
    }

    ImGui::End();
}

static void HandleTableSort(std::vector<FileEntry>& entries) {
    if (ImGuiTableSortSpecs* sorts_specs = ImGui::TableGetSortSpecs()) {
        if (sorts_specs->SpecsDirty && sorts_specs->SpecsCount > 0) {
            const ImGuiTableColumnSortSpecs* sort_spec = &sorts_specs->Specs[0];
            bool descending = (sort_spec->SortDirection == ImGuiSortDirection_Descending);
            
            TableColumn col = TableColumn::Path;
            if (sort_spec->ColumnIndex == 2) col = TableColumn::Size;
            // Column 3 is Last Modified (could implement it if needed, currently string format)

            SortFileEntries(entries, col, descending);
            sorts_specs->SpecsDirty = false;
        }
    }
}

void DupCleanerApp::renderExactDuplicates() {
    for (size_t i = 0; i < exact_duplicates.size(); ++i) {
        auto& group = exact_duplicates[i];
        uintmax_t wasted = calculateWastedSpace(group);
        
        if (ImGui::CollapsingHeader(("Group " + std::to_string(i + 1) + " - Wasted: " + formatBytes(wasted)).c_str())) {
            
            static ImGuiTableFlags flags = ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable | ImGuiTableFlags_Sortable | ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerH;
            
            if (ImGui::BeginTable(("ExactTable" + std::to_string(i)).c_str(), 4, flags)) {
                ImGui::TableSetupColumn("Keep", ImGuiTableColumnFlags_NoSort | ImGuiTableColumnFlags_WidthFixed, 40.0f);
                ImGui::TableSetupColumn("File Path", ImGuiTableColumnFlags_DefaultSort | ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableSetupColumn("Size", ImGuiTableColumnFlags_WidthFixed, 80.0f);
                ImGui::TableSetupColumn("Thumbnail", ImGuiTableColumnFlags_NoSort | ImGuiTableColumnFlags_WidthFixed, 100.0f);
                ImGui::TableHeadersRow();

                HandleTableSort(group);

                bool is_image = isImageGroup(group);
                auto thumbnail_paths = getPathsForThumbnails(group, false);

                for (const auto& file : group) {
                    ImGui::TableNextRow();
                    
                    std::string path_str = file.path.string();
                    bool is_selected = selected_for_deletion.count(path_str) > 0;
                    
                    ImGui::TableSetColumnIndex(0);
                    if (ImGui::Checkbox(("##" + path_str).c_str(), &is_selected)) {
                        if (is_selected) selected_for_deletion.insert(path_str);
                        else selected_for_deletion.erase(path_str);
                    }
                    
                    ImGui::TableSetColumnIndex(1);
                    ImGui::TextWrapped("%s", path_str.c_str());
                    
                    ImGui::TableSetColumnIndex(2);
                    ImGui::Text("%s", formatBytes(file.size).c_str());
                    
                    ImGui::TableSetColumnIndex(3);
                    if (is_image && std::find(thumbnail_paths.begin(), thumbnail_paths.end(), file.path) != thumbnail_paths.end()) {
                        GLuint tex = thumbnail_cache.getTexture(file.path);
                        if (tex != 0) {
                            ImGui::Image((void*)(intptr_t)tex, ImVec2(100, 100));
                        }
                    }
                }
                ImGui::EndTable();
            }
        }
    }
}

void DupCleanerApp::renderNearDuplicates() {
    for (size_t i = 0; i < near_duplicates.groups.size(); ++i) {
        auto& group = near_duplicates.groups[i];
        if (ImGui::CollapsingHeader(("Near Duplicate Group " + std::to_string(i + 1)).c_str())) {
            
            static ImGuiTableFlags flags = ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable | ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerH;
            
            if (ImGui::BeginTable(("NearTable" + std::to_string(i)).c_str(), 4, flags)) {
                ImGui::TableSetupColumn("Keep", ImGuiTableColumnFlags_WidthFixed, 40.0f);
                ImGui::TableSetupColumn("File Path", ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableSetupColumn("Similarity", ImGuiTableColumnFlags_WidthFixed, 80.0f);
                ImGui::TableSetupColumn("Thumbnail", ImGuiTableColumnFlags_WidthFixed, 100.0f);
                ImGui::TableHeadersRow();

                std::vector<FileEntry> entries;
                for (const auto& m : group.members) entries.push_back(m.first);
                
                bool is_image = isImageGroup(entries);
                auto thumbnail_paths = getPathsForThumbnails(entries, true);

                for (size_t j = 0; j < group.members.size(); ++j) {
                    ImGui::TableNextRow();
                    const auto& member = group.members[j];
                    std::string path_str = member.first.path.string();
                    
                    bool is_selected = selected_for_deletion.count(path_str) > 0;
                    
                    ImGui::TableSetColumnIndex(0);
                    if (ImGui::Checkbox(("##" + path_str).c_str(), &is_selected)) {
                        if (is_selected) selected_for_deletion.insert(path_str);
                        else selected_for_deletion.erase(path_str);
                    }
                    
                    ImGui::TableSetColumnIndex(1);
                    ImGui::TextWrapped("%s", path_str.c_str());
                    
                    ImGui::TableSetColumnIndex(2);
                    if (j == 0) {
                        ImGui::Text("Reference");
                    } else {
                        int dist = PerceptualHash::hammingDistance(group.members[0].second, member.second);
                        int sim = 100 - (dist * 100 / 64);
                        ImGui::Text("%d%%", sim);
                    }
                    
                    ImGui::TableSetColumnIndex(3);
                    if (is_image && std::find(thumbnail_paths.begin(), thumbnail_paths.end(), member.first.path) != thumbnail_paths.end()) {
                        GLuint tex = thumbnail_cache.getTexture(member.first.path);
                        if (tex != 0) {
                            ImGui::Image((void*)(intptr_t)tex, ImVec2(100, 100));
                        }
                    }
                }
                ImGui::EndTable();
            }
        }
    }
}

} // namespace dupcleaner::gui
