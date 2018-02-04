#include <iostream>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#pragma comment(lib, "vulkan-1.lib")
#pragma comment(lib, "glfw3.lib")

VkInstance vkInstance;

void InitVulkan()
{
    VkApplicationInfo constexpr appInfo = {
        VK_STRUCTURE_TYPE_APPLICATION_INFO,
        nullptr,
        "VulkanIsland", VK_MAKE_VERSION(1, 0, 0),
        "VulkanIsland", VK_MAKE_VERSION(1, 0, 0),
        VK_API_VERSION_1_0
    };

    std::uint32_t extensionsCount = 0;
    auto const extensions = glfwGetRequiredInstanceExtensions(&extensionsCount);

    VkInstanceCreateInfo const createInfo = {
        VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        nullptr, 0,
        &appInfo,
        0, nullptr,
        extensionsCount, extensions
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

    std::uint32_t extensionCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

    std::cout << extensionCount << " extensions supported" << std::endl;

    while (!glfwWindowShouldClose(window))
        glfwPollEvents();

    glfwDestroyWindow(window);

    glfwTerminate();

    return 0;
}