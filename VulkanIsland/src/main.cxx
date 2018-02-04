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

template<class C, class = void>
struct is_iterable : std::false_type {};

template<class C>
struct is_iterable<C, std::void_t<decltype(std::cbegin(std::declval<C>()), std::cend(std::declval<C>()))>> : std::true_type {};

template<class T>
constexpr bool is_iterable_v = is_iterable<T>::value;


VkInstance vkInstance;

template<class T, typename std::enable_if_t<is_iterable_v<std::decay_t<T>>>...>
auto CheckRequiredExtensions(T _requiredExtensions)
{
    std::vector<VkExtensionProperties> requiredExtensions;

    std::transform(_requiredExtensions.begin(), _requiredExtensions.end(), std::back_inserter(requiredExtensions), [] (auto &&name)
    {
        VkExtensionProperties prop{'\0', 1};
        std::string_view temp{name};

        std::uninitialized_copy(temp.cbegin(), temp.cend(), prop.extensionName);

        return prop;
    });

    auto extensionsComp = [] (auto &&lhs, auto &&rhs)
    {
        return std::lexicographical_compare(std::cbegin(lhs.extensionName), std::cend(lhs.extensionName), std::cbegin(rhs.extensionName), std::cend(rhs.extensionName));
    };

    std::sort(requiredExtensions.begin(), requiredExtensions.end(), extensionsComp);

    std::uint32_t extensionsCount = 0;
    if (auto result = vkEnumerateInstanceExtensionProperties(nullptr, &extensionsCount, nullptr); result != VK_SUCCESS)
        throw std::runtime_error("failed to retrieve extensions count: "s + std::to_string(result));

    std::vector<VkExtensionProperties> supportedExtensions(extensionsCount);
    if (auto result = vkEnumerateInstanceExtensionProperties(nullptr, &extensionsCount, supportedExtensions.data()); result != VK_SUCCESS)
        throw std::runtime_error("failed to retrieve extensions: "s + std::to_string(result));

    std::sort(supportedExtensions.begin(), supportedExtensions.end(), extensionsComp);

    return std::includes(supportedExtensions.begin(), supportedExtensions.end(), requiredExtensions.begin(), requiredExtensions.end(), extensionsComp);
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

    std::array<const char *, 2> extensions = {
        "VK_KHR_surface", "VK_KHR_win32_surface"
    };

    if (auto supported = CheckRequiredExtensions(extensions); !supported)
        throw std::runtime_error("not all required extensions are supported");

    VkInstanceCreateInfo const createInfo = {
        VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        nullptr, 0,
        &appInfo,
        0, nullptr,
        static_cast<std::uint32_t>(std::size(extensions)), std::data(extensions)
    };

    if (auto result = vkCreateInstance(&createInfo, nullptr, &vkInstance); result != VK_SUCCESS)
        throw std::runtime_error("failed to create instance");
}

int main()
{
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    auto window = glfwCreateWindow(800, 600, "VulkanIsland", nullptr, nullptr);

    InitVulkan();

    while (!glfwWindowShouldClose(window))
        glfwPollEvents();

    if (vkInstance)
        vkDestroyInstance(vkInstance, nullptr);

    glfwDestroyWindow(window);

    glfwTerminate();

    return 0;
}