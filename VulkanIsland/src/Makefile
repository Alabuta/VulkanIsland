VULKAN_SDK_PATH = /home/almaz/VulkanSDK/1.1.70.1/x86_64

CFLAGS = -std=c++17 -I$(VULKAN_SDK)/include
LDFLAGS = -L$(LD_LIBRARY_PATH) `pkg-config --static --libs glfw3` -lvulkan -lboost_system -lboost_filesystem

VulkanIsland: main.cxx debug.cxx device.cxx instance.cxx
	g++ $(CFLAGS) -o VulkanIsland main.cxx debug.cxx device.cxx instance.cxx $(LDFLAGS)

.PHONY: test clean

test: VulkanIsland
	./VulkanIsland

clean:
	rm -f VulkanIsland
