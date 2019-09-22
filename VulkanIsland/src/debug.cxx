
#include "main.hxx"
#include "debug.hxx"

#include <iostream>
#include <string>
#include <string_view>

#if !defined(_WIN32) 
#include <thread>
#include <execinfo.h>
#endif

#ifndef GLFW_INCLUDE_VULKAN
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#endif

#include <fmt/format.h>


[[nodiscard]] VKAPI_ATTR VkBool32 VKAPI_CALL
debug_callback([[maybe_unused]] VkDebugReportFlagsEXT flags, [[maybe_unused]] VkDebugReportObjectTypeEXT object_type,
              [[maybe_unused]] std::uint64_t object, [[maybe_unused]] std::size_t location, [[maybe_unused]] std::int32_t message_code,
              [[maybe_unused]] const char *layer_prefix, const char *message, [[maybe_unused]] void *user_data)
{
    std::cerr << message << std::endl;

    return VK_FALSE;
}

[[nodiscard]] VKAPI_ATTR VkResult VKAPI_CALL vkCreateDebugReportCallbackEXT(
    VkInstance instance, VkDebugReportCallbackCreateInfoEXT const *pCreateInfo, VkAllocationCallbacks const *pAllocator, VkDebugReportCallbackEXT *pCallback)
{
    auto func = reinterpret_cast<PFN_vkCreateDebugReportCallbackEXT>(vkGetInstanceProcAddr(instance, "vkCreateDebugReportCallbackEXT"));

    if (func)
        return func(instance, pCreateInfo, pAllocator, pCallback);

    return VK_ERROR_EXTENSION_NOT_PRESENT;
}

VKAPI_ATTR void VKAPI_CALL vkDestroyDebugReportCallbackEXT(
    VkInstance instance, VkDebugReportCallbackEXT callback, VkAllocationCallbacks const *pAllocator)
{
    auto func = reinterpret_cast<PFN_vkDestroyDebugReportCallbackEXT>(vkGetInstanceProcAddr(instance, "vkDestroyDebugReportCallbackEXT"));

    if (func)
        func(instance, callback, pAllocator);
}

void create_debug_report_callback(VkInstance instance, VkDebugReportCallbackEXT &callback)
{
    using namespace std::string_literals;

    VkDebugReportCallbackCreateInfoEXT constexpr create_info{
        VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT,
        nullptr,
        VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT | VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_DEBUG_BIT_EXT,
        debug_callback,
        nullptr
    };

    if (auto result = vkCreateDebugReportCallbackEXT(instance, &create_info, nullptr, &callback); result != VK_SUCCESS)
        throw std::runtime_error(fmt::format("failed to set up debug callback: {0:#x}\n"s, result));
}

#if !defined(_WIN32) 
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
