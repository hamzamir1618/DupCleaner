#include <iostream>
#include "CLI/CLI.hpp"
#include "cli_app.h"

using namespace dupcleaner::cli;

int main() {
    CLI::App app;
    CliOptions opts;
    setup_cli(app, opts);
    
    std::vector<std::string> args = {"scan", "/my/path", "--min-size", "1024"};
    std::reverse(args.begin(), args.end());
    try {
        app.parse(args);
        std::cout << "SUCCESS! path=" << opts.path << std::endl;
    } catch (const CLI::ParseError& e) {
        std::cout << "ERROR: " << e.what() << std::endl;
        return app.exit(e);
    }
    return 0;
}
