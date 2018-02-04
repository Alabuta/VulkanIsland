#include <iostream>
#include <memory>
#include <vector>
#include <array>
#include <string>
using namespace std::string_literals;

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#pragma comment(lib, "vulkan-1.lib")
#pragma comment(lib, "glfw3.lib")

namespace detail {
template <class T, std::size_t N, std::size_t... I>
constexpr std::array<std::remove_cv_t<T>, N>
to_array_impl(T(&a)[N], std::index_sequence<I...>)
{
    return {{a[I]...}};
}
}

template <class T, std::size_t N>
constexpr std::array<std::remove_cv_t<T>, N> to_array(T(&a)[N])
{
    return detail::to_array_impl(a, std::make_index_sequence<N>{});
}

VkInstance vkInstance;

auto GetRequiredExtensions()
{
    std::uint32_t extensionsCount = 0;
    if (auto result = vkEnumerateInstanceExtensionProperties(nullptr, &extensionsCount, nullptr); result != VK_SUCCESS)
        throw std::runtime_error("failed to retrieve extensions count: "s + std::to_string(result));

    std::vector<VkExtensionProperties> supportedExtensions(extensionsCount);
    if (auto result = vkEnumerateInstanceExtensionProperties(nullptr, &extensionsCount, supportedExtensions.data()); result != VK_SUCCESS)
        throw std::runtime_error("failed to retrieve extensions: "s + std::to_string(result));

    auto extensionsComp = [] (auto &&lhs, auto &&rhs)
    {
        return std::string(lhs.extensionName) < std::string(rhs.extensionName);
    };

    std::sort(supportedExtensions.begin(), supportedExtensions.end(), extensionsComp);

    std::vector<VkExtensionProperties> requiredExtensions = {{
        {"VK_KHR_surface"}, {"VK_KHR_win32_surface"}
    }};

    std::sort(requiredExtensions.begin(), requiredExtensions.end(), extensionsComp);

    auto supported = std::includes(supportedExtensions.begin(), supportedExtensions.end(), requiredExtensions.begin(), requiredExtensions.end(), extensionsComp);

    if (!supported)
        throw std::runtime_error("not all required extensions are supported");

    std::vector<decltype(VkExtensionProperties::extensionName)> extensions(requiredExtensions.size());

    std::for_each(requiredExtensions.begin(), requiredExtensions.end(), [&extensions, i = 0u] (auto &&prop) mutable
    {
        std::uninitialized_move_n(std::begin(prop.extensionName), std::size(prop.extensionName), extensions.at(i++));
    });

    return extensions;
}

void InitVulkan()
{
    VkApplicationInfo constexpr appInfo = {
        VK_STRUCTURE_TYPE_APPLICATION_INFO,
        nullptr,
        "VulkanIsland", VK_MAKE_VERSION(1, 0, 0),
        "VulkanIsland", VK_MAKE_VERSION(1, 0, 0),
        VK_API_VERSION_1_0
    };

    auto const extensions = GetRequiredExtensions();
    std::vector<const char *> exs;

    for (auto &&extension : extensions)
        exs.emplace_back(extension);

    VkInstanceCreateInfo const createInfo = {
        VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        nullptr, 0,
        &appInfo,
        0, nullptr,
        static_cast<std::uint32_t>(exs.size()), exs.data()
    };

    if (auto result = vkCreateInstance(&createInfo, nullptr, &vkInstance); result != VK_SUCCESS)
        throw std::runtime_error("failed to create instance");
}

int main()
{
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    auto window = glfwCreateWindow(800, 600, "VulkanIsland", nullptr, nullptr);

    std::uint32_t extensionsCount = 0;
    auto const extensions = glfwGetRequiredInstanceExtensions(&extensionsCount);

    for (auto i = 0u; i < extensionsCount; ++i)
        std::cout << extensions[i] << '\n';
    std::cout << '\n';

    InitVulkan();

    while (!glfwWindowShouldClose(window))
        glfwPollEvents();

    if (vkInstance)
        vkDestroyInstance(vkInstance, nullptr);

    glfwDestroyWindow(window);

    glfwTerminate();

    return 0;
}