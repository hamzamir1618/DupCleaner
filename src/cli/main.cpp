#include <iostream>
#include <filesystem>
#include "CLI/CLI.hpp"
#include "scanner.h"
#include "duplicate_finder.h"
#include "deleter.h"
#include "cli_app.h"
#include "thread_pool.h"
#include "interactive_review.h"
#include "perceptual_hash.h"
#include "nlohmann/json.hpp"
#include <chrono>
#include <future>
#include <thread>
#include <iomanip>

template<typename T>
T run_with_progress(
    const std::string& phase_name, 
    bool quiet,
    std::atomic<uintmax_t>* current_items, 
    std::atomic<uintmax_t>* total_items, 
    std::atomic<uintmax_t>* current_bytes,
    std::future<T> fut
) {
    auto start_time = std::chrono::steady_clock::now();
    
    while (fut.wait_for(std::chrono::milliseconds(250)) != std::future_status::ready) {
        if (!quiet && current_items) {
            uintmax_t cur = current_items->load(std::memory_order_relaxed);
            
            std::cerr << "\r\033[K" << phase_name << "... " << cur;
            if (total_items) {
                uintmax_t tot = total_items->load(std::memory_order_relaxed);
                if (tot > 0) std::cerr << " / " << tot;
            }
            std::cerr << " items";
            
            if (current_bytes) {
                auto now = std::chrono::steady_clock::now();
                auto sec = std::chrono::duration_cast<std::chrono::duration<double>>(now - start_time).count();
                if (sec > 0.5) {
                    uintmax_t cur_b = current_bytes->load(std::memory_order_relaxed);
                    double mbps = (cur_b / 1024.0 / 1024.0) / sec;
                    std::cerr << " (" << std::fixed << std::setprecision(1) << mbps << " MB/s)";
                }
            }
            std::cerr << std::flush;
        }
    }
    
    if (!quiet && current_items) {
        std::cerr << "\r\033[K" << std::flush;
    }
    return fut.get();
}
using json = nlohmann::json;
namespace fs = std::filesystem;
using namespace dupcleaner;
using namespace dupcleaner::cli;

int main(int argc, char** argv) {
    try {
    CLI::App app{"dupcleaner - Find and manage duplicate files"};
    CliOptions opts;
    setup_cli(app, opts);

    try {
        app.parse(argc, argv);
    } catch (const CLI::ParseError& e) {
        return app.exit(e);
    }

    if (opts.hash_threads > 0) {
        dupcleaner::initGlobalThreadPool(opts.hash_threads);
    }

    if (opts.has_undo_command) {
        fs::path target(opts.path);
        SafeDeleter deleter(target);
        if (deleter.undoLastDeletion()) {
            std::cout << "Successfully restored the most recent deletion batch.\n";
            return 0;
        } else {
            std::cerr << "Error: No valid undo manifest found in " << (target / ".dupcleaner_trash").u8string() << " or restore failed.\n";
            return 1;
        }
    }

    if (!opts.has_scan_command && !opts.has_clean_command) {
        return 0;
    }

    fs::path target(opts.path);
    if (!fs::exists(target) || !fs::is_directory(target)) {
        if (opts.json_output) {
            json j;
            j["error"] = "Provided path is not a valid directory: " + opts.path;
            std::cout << j.dump(4) << "\n";
        } else {
            std::cerr << "Error: Provided path is not a valid directory: " << opts.path << "\n";
        }
        return 1;
    }

    bool quiet = opts.quiet || opts.json_output;

    if (!quiet) {
        std::cerr << "Scanning directory: " << fs::absolute(target).u8string() << "...\n";
    }

    // --- PHASE 1: Traversal ---
    DirectoryScanner scanner;
    ScannerOptions scan_opts;
    scan_opts.min_size = opts.min_size;
    
    std::atomic<uintmax_t> tr_files{0}, tr_bytes{0};
    DirectoryScanner::ProgressTracker tr_prog{&tr_files, &tr_bytes};
    
    auto t1 = std::chrono::steady_clock::now();
    auto scan_fut = std::async(std::launch::async, [&]() {
        return scanner.scan(target, scan_opts, nullptr, tr_prog);
    });
    
    auto result = run_with_progress("Traversal", quiet, &tr_files, nullptr, nullptr, std::move(scan_fut));
    auto t2 = std::chrono::steady_clock::now();
    
    if (!quiet) {
        std::cerr << "Traversal: " << result.stats.files_visited << " files, " 
                  << result.stats.directories_visited << " directories in " 
                  << std::fixed << std::setprecision(1) << std::chrono::duration_cast<std::chrono::duration<double>>(t2-t1).count() << "s\n";
    }

    // --- PHASE 1.5: Size Bucketing ---
    auto t3 = std::chrono::steady_clock::now();
    auto size_buckets = DuplicateFinder::groupBySize(result.entries);
    DuplicateFinder::filterUniqueSizes(size_buckets);
    auto t4 = std::chrono::steady_clock::now();
    
    size_t candidates = 0;
    for (const auto& [s, g] : size_buckets) candidates += g.size();
    
    if (!quiet) {
        std::cerr << "Size-Bucketing: " << candidates << " candidate files in " 
                  << std::fixed << std::setprecision(3) << std::chrono::duration_cast<std::chrono::duration<double>>(t4-t3).count() << "s\n";
    }

    // --- PHASE 2: Fingerprinting ---
    std::atomic<uintmax_t> fp_curr{0}, fp_tot{0}, fp_bytes{0};
    DuplicateFinder::PhaseProgressTracker fp_prog;
    fp_prog.current_items = &fp_curr;
    fp_prog.total_items = &fp_tot;
    fp_prog.current_bytes = &fp_bytes;
    
    auto fp_fut = std::async(std::launch::async, [&]() {
        return DuplicateFinder::fingerprintCandidates(size_buckets, fp_prog);
    });
    
    auto hash_buckets = run_with_progress("Fingerprinting", quiet, &fp_curr, &fp_tot, &fp_bytes, std::move(fp_fut));
    auto t5 = std::chrono::steady_clock::now();
    if (!quiet) {
        std::cerr << "Fingerprinting: " << fp_curr.load() << " files in " 
                  << std::fixed << std::setprecision(1) << std::chrono::duration_cast<std::chrono::duration<double>>(t5-t4).count() << "s\n";
    }

    // --- PHASE 3: Byte Verification ---
    std::atomic<uintmax_t> bv_curr{0}, bv_tot{0}, bv_bytes{0};
    DuplicateFinder::PhaseProgressTracker bv_prog;
    bv_prog.current_items = &bv_curr;
    bv_prog.total_items = &bv_tot;
    bv_prog.current_bytes = &bv_bytes;

    auto bv_fut = std::async(std::launch::async, [&]() {
        return DuplicateFinder::verifyExactDuplicates(hash_buckets, bv_prog);
    });

    auto dup_groups = run_with_progress("Byte-Verification", quiet, &bv_curr, &bv_tot, &bv_bytes, std::move(bv_fut));
    auto t6 = std::chrono::steady_clock::now();
    if (!quiet) {
        std::cerr << "Byte-Verification: " << bv_curr.load() << " files in " 
                  << std::fixed << std::setprecision(1) << std::chrono::duration_cast<std::chrono::duration<double>>(t6-t5).count() << "s\n";
    }

    // --- PHASE 4: Perceptual Hashing (Optional) ---
    DuplicateFinder::NearDuplicateResult near_dup_result;
    if ((opts.has_scan_command || opts.has_clean_command) && opts.include_near_duplicates) {
        std::atomic<uintmax_t> ph_curr{0}, ph_tot{0};
        DuplicateFinder::PhaseProgressTracker ph_prog;
        ph_prog.current_items = &ph_curr;
        ph_prog.total_items = &ph_tot;

        auto t7 = std::chrono::steady_clock::now();
        auto ph_fut = std::async(std::launch::async, [&]() {
            return DuplicateFinder::findNearDuplicateImages(result.entries, opts.similarity_threshold, ph_prog);
        });

        near_dup_result = run_with_progress("Perceptual-Hashing", quiet, &ph_curr, &ph_tot, nullptr, std::move(ph_fut));
        auto t8 = std::chrono::steady_clock::now();
        if (!quiet) {
            std::cerr << "Perceptual-Hashing: " << ph_curr.load() << " images in " 
                      << std::fixed << std::setprecision(1) << std::chrono::duration_cast<std::chrono::duration<double>>(t8-t7).count() << "s\n";
        }
    }

    if (opts.has_scan_command) {
        if (opts.json_output) {
            json j;
            j["scan_stats"] = {
                {"files_visited", result.stats.files_visited},
                {"bytes_visited", result.stats.bytes_visited},
                {"directories_visited", result.stats.directories_visited},
                {"items_skipped", result.stats.items_skipped},
                {"duration_ms", result.stats.duration.count()}
            };
            
            if (opts.verbose) {
                j["skipped_paths"] = result.skipped_paths;
            }

            json groups = json::array();
            uintmax_t total_wasted_space = 0;

            for (const auto& group : dup_groups) {
                uintmax_t file_size = group[0].size;
                uintmax_t group_wasted = file_size * (group.size() - 1);
                total_wasted_space += group_wasted;

                json g;
                g["size_bytes"] = file_size;
                g["wasted_bytes"] = group_wasted;
                json paths = json::array();
                for (const auto& entry : group) {
                    paths.push_back(entry.path.u8string());
                }
                g["files"] = paths;
                groups.push_back(g);
            }

            j["groups"] = groups;
            j["total_wasted_bytes"] = total_wasted_space;

            if (opts.include_near_duplicates) {
                json nd_groups = json::array();
                for (const auto& group : near_dup_result.groups) {
                    json g;
                    uint64_t ref_hash = group.members[0].second;
                    json files_arr = json::array();
                    for (const auto& member : group.members) {
                        json f;
                        f["path"] = member.first.path.u8string();
                        f["distance_from_reference"] = PerceptualHash::hammingDistance(ref_hash, member.second);
                        files_arr.push_back(f);
                    }
                    g["files"] = files_arr;
                    nd_groups.push_back(g);
                }
                j["near_duplicate_groups"] = nd_groups;

                if (opts.verbose) {
                    json nd_skipped = json::array();
                    for (const auto& sp : near_dup_result.skipped_paths) {
                        nd_skipped.push_back(sp.u8string());
                    }
                    j["near_duplicate_skipped_paths"] = nd_skipped;
                }
            }

            std::cout << j.dump(2) << "\n";
        } else {
            if (opts.verbose) {
                std::cout << "\n--- Scan Complete ---\n";
                std::cout << "Total files visited: " << result.stats.files_visited << "\n";
                std::cout << "Total bytes visited: " << result.stats.bytes_visited << " bytes\n";
                std::cout << "Directories visited: " << result.stats.directories_visited << "\n";
                std::cout << "Items skipped:       " << result.stats.items_skipped << "\n";
                std::cout << "Scan duration:       " << result.stats.duration.count() << " ms\n";
                
                if (!result.skipped_paths.empty()) {
                    std::cout << "\nWarning: " << result.skipped_paths.size() << " items were skipped.\n";
                    for (const auto& sp : result.skipped_paths) {
                        std::cout << "  " << sp << "\n";
                    }
                }
            }

            if (dup_groups.empty()) {
                std::cout << "No exact duplicates found.\n";
            } else {
                uintmax_t total_wasted_space = 0;
                std::cout << "\nFound " << dup_groups.size() << " exact duplicate groups:\n\n";

                for (size_t i = 0; i < dup_groups.size(); ++i) {
                    const auto& group = dup_groups[i];
                    uintmax_t file_size = group[0].size;
                    uintmax_t group_wasted = file_size * (group.size() - 1);
                    total_wasted_space += group_wasted;

                    std::cout << "Group " << (i + 1) << " (Size: " << file_size << " bytes, Wasted: " << group_wasted << " bytes):\n";
                    for (const auto& entry : group) {
                        std::cout << "  - " << entry.path.u8string() << "\n";
                    }
                    std::cout << "\n";
                }
                std::cout << "Total wasted space: " << total_wasted_space << " bytes.\n";
            }

            if (opts.include_near_duplicates) {
                if (near_dup_result.groups.empty()) {
                    std::cout << "\nNo near-duplicate images found.\n";
                } else {
                    std::cout << "\nFound " << near_dup_result.groups.size() << " near-duplicate image groups (Threshold: " << opts.similarity_threshold << "):\n\n";
                    for (size_t i = 0; i < near_dup_result.groups.size(); ++i) {
                        const auto& group = near_dup_result.groups[i];
                        std::cout << "Near-Duplicate Group " << (i + 1) << ":\n";
                        
                        uint64_t ref_hash = group.members[0].second;
                        
                        for (size_t j = 0; j < group.members.size(); ++j) {
                            int dist = PerceptualHash::hammingDistance(ref_hash, group.members[j].second);
                            int sim = 100 - (dist * 100 / 64);
                            std::cout << "  - " << group.members[j].first.path.u8string();
                            if (j == 0) {
                                std::cout << " (Reference Image)\n";
                            } else {
                                std::cout << " (Similarity: " << sim << "% | Distance: " << dist << ")\n";
                            }
                        }
                        std::cout << "\n";
                    }
                }
                if (opts.verbose && !near_dup_result.skipped_paths.empty()) {
                    std::cout << "Skipped " << near_dup_result.skipped_paths.size() << " non-decodable images during perceptual hashing.\n";
                }
            }
        }
    } else if (opts.has_clean_command) {
        if (dup_groups.empty() && (!opts.include_near_duplicates || near_dup_result.groups.empty())) {
            std::cout << "No exact or near duplicates found to clean.\n";
            return 0;
        }

        KeepStrategy strat = KeepStrategy::KeepOldest;
        if (opts.strategy == "newest") strat = KeepStrategy::KeepNewest;
        else if (opts.strategy == "alpha-first") strat = KeepStrategy::KeepFirstAlphabetically;

        SafeDeleter deleter(target);
        DeletionPlan plan;

        if (opts.interactive) {
            auto process_groups = [&](const std::vector<std::vector<FileEntry>>& groups, const std::string& type_name) {
                for (size_t i = 0; i < groups.size(); ++i) {
                    const auto& g = groups[i];
                    std::cout << "\n--- " << type_name << " Group " << (i + 1) << " of " << groups.size() << " ---\n";
                    
                    std::vector<std::vector<FileEntry>> single_group = { g };
                    DeletionPlan single_plan = deleter.planDeletion(single_group, strat);
                    size_t suggested_idx = 0;
                    for (size_t j = 0; j < g.size(); ++j) {
                        if (!single_plan.keep_files.empty() && g[j].path == single_plan.keep_files[0]) {
                            suggested_idx = j;
                            break;
                        }
                    }

                    auto decision = resolveGroupInteractively(g, suggested_idx, std::cin, std::cout);
                    if (!decision.skip) {
                        for (const auto& f : decision.keep_files) plan.keep_files.push_back(f.path);
                        for (const auto& f : decision.delete_files) plan.delete_files.push_back(f.path);
                    }
                }
            };

            process_groups(dup_groups, "Exact Duplicate");
            
            if (opts.include_near_duplicates) {
                std::vector<std::vector<FileEntry>> nd_raw_groups;
                for (const auto& g : near_dup_result.groups) {
                    std::vector<FileEntry> members;
                    members.reserve(g.members.size());
                    for (const auto& m : g.members) members.push_back(m.first);
                    nd_raw_groups.push_back(members);
                }
                process_groups(nd_raw_groups, "Near-Duplicate");
            }
        } else {
            plan = deleter.planDeletion(dup_groups, strat);
            if (opts.include_near_duplicates) {
                std::vector<std::vector<FileEntry>> nd_raw_groups;
                for (const auto& g : near_dup_result.groups) {
                    std::vector<FileEntry> members;
                    members.reserve(g.members.size());
                    for (const auto& m : g.members) members.push_back(m.first);
                    nd_raw_groups.push_back(members);
                }
                DeletionPlan nd_plan = deleter.planDeletion(nd_raw_groups, strat);
                plan.keep_files.insert(plan.keep_files.end(), nd_plan.keep_files.begin(), nd_plan.keep_files.end());
                plan.delete_files.insert(plan.delete_files.end(), nd_plan.delete_files.begin(), nd_plan.delete_files.end());
            }
        }

        uintmax_t total_reclaimed = 0;
        for (const auto& f : plan.delete_files) {
            std::error_code ec;
            total_reclaimed += fs::file_size(f, ec);
        }
        
        std::cout << "\nDeletion Plan:\n";
        std::cout << "Files to delete: " << plan.delete_files.size() << "\n";
        std::cout << "Files to keep: " << plan.keep_files.size() << "\n";
        std::cout << "Total space to reclaim: " << total_reclaimed << " bytes\n";

        if (opts.dry_run) {
            std::cout << "\n[Dry-run] Exiting without making changes.\n";
            return 0;
        }

        bool do_delete = opts.yes;
        if (!do_delete) {
            std::cout << "\nProceed with deletion? [y/N]: ";
            std::string resp;
            if (std::cin >> resp && (resp == "y" || resp == "Y")) {
                do_delete = true;
            }
        }

        if (do_delete) {
            bool moveToTrash = opts.trash && !opts.permanent;
            auto res = deleter.execute(plan, moveToTrash);
            if (!res.success) {
                std::cerr << "Failed to execute deletion plan.\n";
            } else if (!res.failed_files.empty()) {
                std::cerr << "Deletion plan completed but some files failed to delete:\n";
                for (const auto& f : res.failed_files) {
                    std::cerr << "  - " << f.u8string() << "\n";
                }
            } else {
                std::cout << "Deletion plan executed successfully.\n";
            }
        } else {
            std::cout << "Aborted by user.\n";
        }
    }

    } catch (const std::exception& e) {
        std::cerr << "Fatal exception: " << e.what() << "\n";
        return 1;
    } catch (...) {
        std::cerr << "Fatal unknown exception.\n";
        return 1;
    }
}
