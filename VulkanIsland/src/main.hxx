#pragma once


#include <iostream>
#include <memory>
#include <vector>
#include <array>
#include <tuple>
#include <set>
#include <optional>
#include <string>
#include <string_view>

using namespace std::string_literals;
using namespace std::string_view_literals;

#include <fstream>
#include <filesystem>
namespace fs = std::filesystem;


#if !defined(_WIN32)
    #include <thread>
    #include <csignal>
    #include <execinfo.h>

    void posix_signal_handler(int signum)
    {
        using namespace std::string_literals;

        auto current_thread = std::this_thread::get_id();

        auto name = "unknown"s;

        switch (signum) {
            case SIGABRT: name = "SIGABRT"s;  break;
            case SIGSEGV: name = "SIGSEGV"s;  break;
            case SIGBUS:  name = "SIGBUS"s;   break;
            case SIGILL:  name = "SIGILL"s;   break;
            case SIGFPE:  name = "SIGFPE"s;   break;
            case SIGTRAP: name = "SIGTRAP"s;  break;
        }

        std::array<void *, 32> callStack;

        auto size = backtrace(std::data(callStack), std::size(callStack));

        std::cerr << fmt::format("Error: signal {}\n"s, name);

        auto symbollist = backtrace_symbols(std::data(callStack), size);

        for (auto i = 0; i < size; ++i)
            std::cerr << fmt::format("{} {} {}\n"s, i, current_thread, symbollist[i]);

        free(symbollist);

        exit(1);
    }
#endif

#include "utility/helpers.hxx"

#define USE_WIN32 0

#include "config.hxx"





auto constexpr kREVERSED_DEPTH = true;
