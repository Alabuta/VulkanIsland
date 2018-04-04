VULKAN_SDK_PATH = /home/almaz/VulkanSDK/1.1.70.1/x86_64

CFLAGS = -I$(VULKAN_SDK)/include -std=c++17 -pedantic -Wall -Wextra -fPIC -DDEBUG -g #-fsanitize=address
LDFLAGS = -L$(LD_LIBRARY_PATH) `pkg-config --static --libs glfw3` -lvulkan -lboost_system -lboost_filesystem

VulkanIsland: debug.cxx device.cxx instance.cxx main.cxx
	g++ $(CFLAGS) -o VulkanIsland debug.cxx device.cxx instance.cxx main.cxx $(LDFLAGS)

.PHONY: test clean temp


test: VulkanIsland
	./VulkanIsland

clean:
	rm -f VulkanIsland

temp: temp.cxx
	g++ $(CFLAGS) -o VulkanIsland temp.cxx $(LDFLAGS)