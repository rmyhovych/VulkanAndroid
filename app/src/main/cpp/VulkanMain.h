//
// Created by rmyho on 3/23/2020.
//

#ifndef VULKAN_ANDROID_VULKANMAIN_H
#define VULKAN_ANDROID_VULKANMAIN_H

#include "vulkan_wrapper.h"
#include <android_native_app_glue.h>

#include "camera/FocusedCamera.h"

#include <vector>
#include <array>

struct QueueFamilyIndexes
{
	uint32_t graphical;
	uint32_t present;
};

struct SwapChainSupportDetails
{
	VkSurfaceCapabilitiesKHR capabilities;

	VkSurfaceFormatKHR surfaceFormat;
	VkPresentModeKHR presentMode;
	VkExtent2D extent;
};

struct UniformBufferObject {
	alignas(16) glm::mat4 model;
	alignas(16) glm::mat4 view;
	alignas(16) glm::mat4 projection;
};

struct Vertex
{
	glm::vec3 position;
	glm::vec3 color;

	static VkVertexInputBindingDescription getBindingDescription()
	{
		VkVertexInputBindingDescription bindingDescription = {};
		bindingDescription.binding = 0;
		bindingDescription.stride = sizeof(Vertex);
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		return bindingDescription;
	}

	static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions()
	{
		std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions = {};

		// POSITION
		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[0].offset = offsetof(Vertex, position);

		// COLOR
		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[1].offset = offsetof(Vertex, color);

		return attributeDescriptions;
	}
};

class VulkanMain
{
public:
	VulkanMain();

	void init(android_app* pApp);
	void destroy();

	void draw();

	bool m_isReady = false;

private:
	void createInstance();
	void createSurface();
	void createDevice();
	void createSwapChain();

	std::vector<VkImageView> createImageViews(VkDevice logicalDevice, std::vector<VkImage>& images, SwapChainSupportDetails& swapchainSupportDetails) const;
	void createGraphicsPipeline(const char* vertexPath, const char* fragmentPath);

	void createRenderPass();
	void createDescriptorSetLayout();

	void createFramebuffers();
	void createCommandPool();

	void createVertexBuffer();
	void createIndexBuffer();
	void createUniformBuffers();

	void createDescriptorPool();
	void createDescriptorSets();

	void createCommandBuffers();
	void createSyncObjects();


	void cleanupSwapChain();
	void recreateSwapChain();

private:
	// DEVICE
	bool isDeviceSuitable(VkPhysicalDevice physicalDevice, VkSurfaceKHR surfaceHandle);

	// QUEUES
	QueueFamilyIndexes getQueueFamilyIndexes(VkPhysicalDevice physicalDevice);

	bool getQueueGraphicsFamilyIndex(VkPhysicalDevice physicalDevice, uint32_t* index);
	bool getQueuePresentFamilyIndex(VkPhysicalDevice physicalDevice, VkSurfaceKHR surfaceHandle, uint32_t* index, uint32_t priorityIndex);
	void setQueueCreateInfo(VkDeviceQueueCreateInfo& queueCreateInfo, uint32_t index, float priority);


	// SWAPCHAIN
	VkSwapchainKHR createSwapchain(VkSwapchainKHR oldSwapchain, const SwapChainSupportDetails& swapChainSupportDetails, VkDevice logicalDevice, VkSurfaceKHR surfaceHandle, QueueFamilyIndexes& familyIndexes) const;

	SwapChainSupportDetails getSwapChainSupportDetails(VkPhysicalDevice physicalDevice, VkSurfaceKHR surfaceHandle, uint32_t width, uint32_t height);
	VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& swapChainFormats);
	VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& presentModes);
	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& swapChainCapabilities, uint32_t width, uint32_t height);




	// MEMORY SHIT
	uint32_t findMemoryType(VkPhysicalDevice physicalDevice, uint32_t memoryTypeFilter, VkMemoryPropertyFlags memoryPropertyFlags);
	void createBuffer(VkDeviceSize size, VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags propertyFlags, VkBuffer* buffer, VkDeviceMemory* bufferMemory);

	void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);


	VkShaderModule createShaderModule(const char* shaderPath);

	VkPipelineShaderStageCreateInfo getCreateShaderPipelineInfo(VkShaderModule shaderModule, VkShaderStageFlagBits shaderStage);

	VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags featureFlags);
	VkFormat findDepthFormat();
	bool hasStencilComponent(VkFormat format);



private:
	android_app* m_pApp;

	FocusedCamera m_camera;
	VkSurfaceKHR m_surface;

	VkInstance m_instance;

	VkPhysicalDevice m_physicalDevice;
	VkDevice m_logicalDevice;

	QueueFamilyIndexes m_queueFamilyIndexes;

	VkQueue m_graphicsQueue;
	VkQueue m_presentQueue;

	SwapChainSupportDetails m_swapchainSupportDetails;

	VkSwapchainKHR m_swapchain;
	std::vector<VkImage> m_images;
	std::vector<VkImageView> m_imageViews;

	VkRenderPass m_renderPass;
	VkPipeline m_graphicsPipeline;
	VkPipelineLayout m_pipelineLayout;

	std::vector<VkFramebuffer> m_framebuffers;

	VkCommandPool m_commandPool;
	std::vector<VkCommandBuffer> m_commandBuffers;

	std::vector<VkSemaphore> m_semaphoresImageAvailable;
	std::vector<VkSemaphore> m_semaphoresRenderFinished;

	std::vector<VkFence> m_inFlightFences;
	std::vector<VkFence> m_imagesInFlight;

	uint32_t m_currentFrameIndex;
	bool m_framebufferResized;


	std::vector<Vertex> m_vertices;
	std::vector<uint16_t> m_indexes;

	VkBuffer m_vertexBuffer;
	VkDeviceMemory m_vertexBufferMemory;

	VkBuffer m_indexBuffer;
	VkDeviceMemory m_indexBufferMemory;


	VkDescriptorSetLayout m_uboDescriptorSetLayout;
	VkDescriptorPool m_descriptorPool;
	std::vector<VkDescriptorSet> m_descriptorSets;

	std::vector<VkBuffer> m_uniformBuffers;
	std::vector<VkDeviceMemory> m_uniformBuffersMemory;

#ifndef NDEBUG
	VkDebugUtilsMessengerEXT m_debugMessenger = VK_NULL_HANDLE;
#endif // !NDEBUG

};


#endif //VULKAN_ANDROID_VULKANMAIN_H
