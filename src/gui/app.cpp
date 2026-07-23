#include "app.h"
#include "formatting.h"
#include "scanner.h"
#include "duplicate_finder.h"
#include "perceptual_hash.h"
#include "thumbnail_logic.h"
#include "imgui.h"
#include <iostream>

namespace dupcleaner::gui {

DupCleanerApp::DupCleanerApp() {
}

DupCleanerApp::~DupCleanerApp() {
    if (scan_thread.joinable()) {
        scan_thread.join();
    }
}

void DupCleanerApp::startScan(const std::string& path) {
    exact_duplicates.clear();
    near_duplicates.groups.clear();
    exact_wasted_space = 0;
    
    is_scanning = true;
    scan_finished_flag = false;
    
    scan_thread = std::thread([this, path]() {
        DirectoryScanner scanner;
        ScannerOptions scan_opts;
        
        auto result = scanner.scan(path, scan_opts);
        auto exact = DuplicateFinder::findExactDuplicates(result.entries);
        auto near_dup = DuplicateFinder::findNearDuplicateImages(result.entries, 10);
        
        sortGroupsByWastedSpaceDescending(exact);
        
        std::lock_guard<std::mutex> lock(results_mutex);
        this->exact_duplicates = std::move(exact);
        this->near_duplicates = std::move(near_dup);
        this->scan_finished_flag = true;
    });
}

void DupCleanerApp::render() {
    if (is_scanning && scan_finished_flag) {
        if (scan_thread.joinable()) {
            scan_thread.join();
        }
        
        std::lock_guard<std::mutex> lock(results_mutex);
        for (const auto& group : exact_duplicates) {
            exact_wasted_space += calculateWastedSpace(group);
        }
        is_scanning = false;
        scan_finished_flag = false;
    }

    ImGui::Begin("Duplicate Photo Cleaner");

    ImGui::InputText("Directory", target_dir, sizeof(target_dir));
    ImGui::SameLine();
    
    if (is_scanning) {
        ImGui::BeginDisabled();
        ImGui::Button("Scanning...");
        ImGui::EndDisabled();
    } else {
        if (ImGui::Button("Scan")) {
            std::string path(target_dir);
            if (!path.empty()) {
                startScan(path);
            }
        }
    }

    if (is_scanning) {
        ImGui::Text("Scanning in progress... Please wait.");
    } else if (!exact_duplicates.empty() || !near_duplicates.groups.empty()) {
        ImGui::Separator();
        ImGui::Text("Total Wasted Space (Exact): %s", formatBytes(exact_wasted_space).c_str());
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

void DupCleanerApp::renderExactDuplicates() {
    std::lock_guard<std::mutex> lock(results_mutex);
    ImGui::BeginChild("ExactDuplicatesList", ImVec2(0, 0), true);

    for (size_t i = 0; i < exact_duplicates.size(); ++i) {
        const auto& group = exact_duplicates[i];
        uintmax_t wasted = calculateWastedSpace(group);
        
        if (ImGui::CollapsingHeader(("Group " + std::to_string(i + 1) + " - Wasted: " + formatBytes(wasted)).c_str())) {
            bool is_image = isImageGroup(group);
            auto thumbnail_paths = getPathsForThumbnails(group, false);

            for (const auto& file : group) {
                ImGui::BulletText("%s (%s)", file.path.string().c_str(), formatBytes(file.size).c_str());
                
                if (is_image && std::find(thumbnail_paths.begin(), thumbnail_paths.end(), file.path) != thumbnail_paths.end()) {
                    GLuint tex = thumbnail_cache.getTexture(file.path);
                    if (tex != 0) {
                        ImGui::Indent();
                        ImGui::Image((void*)(intptr_t)tex, ImVec2(100, 100));
                        ImGui::Unindent();
                    }
                }
            }
        }
    }
    
    ImGui::EndChild();
}

void DupCleanerApp::renderNearDuplicates() {
    std::lock_guard<std::mutex> lock(results_mutex);
    ImGui::BeginChild("NearDuplicatesList", ImVec2(0, 0), true);

    for (size_t i = 0; i < near_duplicates.groups.size(); ++i) {
        const auto& group = near_duplicates.groups[i];
        if (ImGui::CollapsingHeader(("Near Duplicate Group " + std::to_string(i + 1)).c_str())) {
            std::vector<FileEntry> entries;
            for (const auto& m : group.members) entries.push_back(m.first);
            
            bool is_image = isImageGroup(entries);
            auto thumbnail_paths = getPathsForThumbnails(entries, true);

            for (size_t j = 0; j < group.members.size(); ++j) {
                const auto& member = group.members[j];
                if (j == 0) {
                    ImGui::BulletText("%s (Reference)", member.first.path.string().c_str());
                } else {
                    int dist = PerceptualHash::hammingDistance(group.members[0].second, member.second);
                    int sim = 100 - (dist * 100 / 64);
                    ImGui::BulletText("%s (Similarity: %d%%)", member.first.path.string().c_str(), sim);
                }

                if (is_image && std::find(thumbnail_paths.begin(), thumbnail_paths.end(), member.first.path) != thumbnail_paths.end()) {
                    GLuint tex = thumbnail_cache.getTexture(member.first.path);
                    if (tex != 0) {
                        ImGui::Indent();
                        ImGui::Image((void*)(intptr_t)tex, ImVec2(100, 100));
                        ImGui::Unindent();
                    }
                }
            }
        }
    }
    
    ImGui::EndChild();
}

} // namespace dupcleaner::gui
