#include "interactive_review.h"
#include <string>
#include <sstream>

namespace dupcleaner::cli {

GroupDecision resolveGroupInteractively(
    const std::vector<FileEntry>& group,
    size_t suggested_keep_index,
    std::istream& in,
    std::ostream& out
) {
    GroupDecision decision;
    if (group.empty()) {
        decision.skip = true;
        return decision;
    }

    out << "Reviewing Group:\n";
    for (size_t i = 0; i < group.size(); ++i) {
        out << "  [" << (i + 1) << "] " << group[i].path.string() 
            << " (" << group[i].size << " bytes)";
        if (i == suggested_keep_index) {
            out << " (*Suggested*)";
        }
        out << "\n";
    }

    while (true) {
        out << "Action (a=accept suggestion, s=skip group, k<N>=keep file N): ";
        std::string input;
        if (!std::getline(in, input)) {
            // EOF or error
            decision.skip = true;
            return decision;
        }

        // Trim input
        input.erase(0, input.find_first_not_of(" \t\r\n"));
        input.erase(input.find_last_not_of(" \t\r\n") + 1);

        if (input.empty()) continue;

        if (input == "s") {
            decision.skip = true;
            return decision;
        }

        if (input == "a") {
            for (size_t i = 0; i < group.size(); ++i) {
                if (i == suggested_keep_index) {
                    decision.keep_files.push_back(group[i]);
                } else {
                    decision.delete_files.push_back(group[i]);
                }
            }
            return decision;
        }

        if (input[0] == 'k' && input.size() > 1) {
            std::string num_str = input.substr(1);
            try {
                size_t num = std::stoull(num_str);
                if (num > 0 && num <= group.size()) {
                    size_t keep_idx = num - 1;
                    for (size_t i = 0; i < group.size(); ++i) {
                        if (i == keep_idx) {
                            decision.keep_files.push_back(group[i]);
                        } else {
                            decision.delete_files.push_back(group[i]);
                        }
                    }
                    return decision;
                } else {
                    out << "Invalid file index: " << num << "\n";
                }
            } catch (...) {
                out << "Invalid input format.\n";
            }
        } else {
            out << "Invalid input format.\n";
        }
    }
}

} // namespace dupcleaner::cli
