// CVM++ — File runner and REPL.
//
// Usage:
//   cvmpp                       # interactive REPL
//   cvmpp script.cvm            # run a script file
//   cvmpp --debug script.cvm    # show bytecode disassembly, then run

#include "lexer.hpp"
#include "parser.hpp"
#include "compiler.hpp"
#include "vm.hpp"
#include "debug.hpp"

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace {

bool g_debug = false;

int runSource(const std::string& source) {
    using namespace cvm;
    try {
        Lexer lexer(source);
        auto tokens = lexer.scanTokens();

        Parser parser(std::move(tokens));
        auto program = parser.parse();

        Compiler compiler;
        Chunk chunk = compiler.compile(program);

        if (g_debug) {
            Disassembler::disassemble(chunk, compiler.names, "CVM++ bytecode");
            std::cout << "-- execution --\n";
        }

        VM vm;
        return vm.run(chunk, compiler.names) == VM::Result::OK ? 0 : 70;
    } catch (const std::exception& ex) {
        std::cerr << ex.what() << "\n";
        return 65; // EX_DATAERR
    }
}

int runFile(const std::string& path) {
    std::ifstream f(path);
    if (!f.is_open()) {
        std::cerr << "Cannot open file: " << path << "\n";
        return 74;
    }
    std::stringstream ss;
    ss << f.rdbuf();
    return runSource(ss.str());
}

int runRepl() {
    std::cout << "CVM++ REPL  (Ctrl-D / Ctrl-Z to exit)\n";
    std::cout << "Type complete statements terminated with ';'.\n";
    std::string line;
    while (true) {
        std::cout << "cvm> " << std::flush;
        if (!std::getline(std::cin, line)) { std::cout << "\n"; break; }
        if (line.empty()) continue;
        runSource(line);
    }
    return 0;
}

} // namespace

int main(int argc, char** argv) {
    std::vector<std::string> args;
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "--debug" || a == "-d") g_debug = true;
        else args.push_back(std::move(a));
    }
    if (args.empty())     return runRepl();
    if (args.size() == 1) return runFile(args[0]);
    std::cerr << "Usage: cvmpp [--debug] [script.cvm]\n";
    return 64;
}
