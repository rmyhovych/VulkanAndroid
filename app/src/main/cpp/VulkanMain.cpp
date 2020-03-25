//
// Created by rmyho on 3/23/2020.
//


#include "VulkanMain.h"

#include "FileReader.h"

#include <string>

#include <android/native_window.h>

#include <android/log.h>
#include <exception>
#include <vector>
#include <set>

const uint32_t MAX_FRAMES_IN_FLIGHT = 5;

#define CALL_VK(result)                                               \
    if (VK_SUCCESS != (result)) {                                     \
        __android_log_print(ANDROID_LOG_ERROR, "Test ",             \
        "Vulkan error. File[%s], line[%d]", __FILE__, __LINE__);    \
        assert(false);                                              \
        }

#ifdef VALIDATION

VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageTypes,
		const VkDebugUtilsMessengerCallbackDataEXT *callbackData,
		void *userData)
{
	const char validation[]  = "Validation";
	const char performance[] = "Performance";
	const char error[]       = "ERROR";
	const char warning[]     = "WARNING";
	const char unknownType[] = "UNKNOWN_TYPE";
	const char unknownSeverity[] = "UNKNOWN_SEVERITY";
	const char* typeString      = unknownType;
	const char* severityString  = unknownSeverity;
	const char* messageIdName   = callbackData->pMessageIdName;
	int32_t messageIdNumber     = callbackData->messageIdNumber;
	const char* message         = callbackData->pMessage;
	android_LogPriority priority = ANDROID_LOG_UNKNOWN;

	if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
		severityString = error;
		priority = ANDROID_LOG_ERROR;
	}
	else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
		severityString = warning;
		priority = ANDROID_LOG_WARN;
	}
	if (messageTypes & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT) {
		typeString = validation;
	}
	else if (messageTypes & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT) {
		typeString = performance;
	}

	__android_log_print(priority,
						"AppName",
						"%s %s: [%s] Code %i : %s",
						typeString,
						severityString,
						messageIdName,
						messageIdNumber,
						message);

	// Returning false tells the layer not to stop when the event occurs, so
	// they see the same behavior with and without validation layers enabled.
	return VK_FALSE;
}


VkResult vkCreateDebugUtilsMessengerEXT_PROXY(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
											  const VkAllocationCallbacks *pAllocator, VkDebugUtilsMessengerEXT *pDebugMessenger)
{

	PFN_vkCreateDebugUtilsMessengerEXT func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance,
																										 "vkCreateDebugUtilsMessengerEXT");
	if (func != nullptr)
	{
		return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
	}

	return VK_ERROR_EXTENSION_NOT_PRESENT;
}

void vkDestroyDebugUtilsMessengerEXT_PROXY(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger,
										   const VkAllocationCallbacks *pAllocator)
{
	PFN_vkDestroyDebugUtilsMessengerEXT func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance,
																										   "vkDestroyDebugUtilsMessengerEXT");

	if (func != nullptr)
	{
		func(instance, debugMessenger, pAllocator);
	}
}

const std::vector<const char *> VALIDATION_LAYERS = {
		"VK_LAYER_KHRONOS_validation"
};

#endif // !VALIDATION


std::vector<const char *> INSTANCE_EXTENSIONS({VK_KHR_SURFACE_EXTENSION_NAME, VK_KHR_ANDROID_SURFACE_EXTENSION_NAME});
const std::vector<const char *> DEVICE_EXTENSIONS({VK_KHR_SWAPCHAIN_EXTENSION_NAME});


VulkanMain::VulkanMain() :
		m_semaphoresImageAvailable(MAX_FRAMES_IN_FLIGHT),
		m_semaphoresRenderFinished(MAX_FRAMES_IN_FLIGHT),
		m_inFlightFences(MAX_FRAMES_IN_FLIGHT),
		m_currentFrameIndex(0),
		m_vertices({
				           {{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}},
				           {{0.5f,  -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}},
				           {{0.5f,  0.5f,  0.0f}, {0.0f, 0.0f, 1.0f}},
				           {{-0.5f, 0.5f,  0.0f}, {1.0f, 1.0f, 1.0f}},
		           }),

		m_indexes({0, 1, 2, 0, 2, 3})
{

}


void VulkanMain::init(android_app *pApp)
{
#ifdef VALIDATION
	uint32_t layerCount;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
	std::vector<VkLayerProperties> avalibleLayers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, avalibleLayers.data());

	for (const char *layerName : VALIDATION_LAYERS)
	{
		bool isLayerFound = false;
		for (VkLayerProperties &layerProperty : avalibleLayers)
		{
			if (strcmp(layerName, layerProperty.layerName) == 0)
			{
				isLayerFound = true;
				break;
			}
		}

		if (!isLayerFound)
		{
			__android_log_assert("Unable to find layer.", nullptr, nullptr);
		}
	}
#endif // !VALIDATION


	m_pApp = pApp;

	createInstance();
	createSurface();
	createDevice();
	createSwapChain();
	m_imageViews = createImageViews(m_logicalDevice, m_images, m_swapchainSupportDetails);
	createRenderPass();
	createDescriptorSetLayout();
	createGraphicsPipeline("triangle.vert.spv", "triangle.frag.spv");

	createFramebuffers();
	createCommandPool();

	createVertexBuffer();
	createIndexBuffer();
	createUniformBuffers();
	createDescriptorPool();
	createDescriptorSets();

	createCommandBuffers();
	createSyncObjects();

	m_isReady = true;
}

void VulkanMain::destroy()
{
	cleanupSwapChain();

	vkDestroyDescriptorSetLayout(m_logicalDevice, m_uboDescriptorSetLayout, nullptr);

	vkDestroyBuffer(m_logicalDevice, m_vertexBuffer, nullptr);
	vkFreeMemory(m_logicalDevice, m_vertexBufferMemory, nullptr);

	vkDestroyBuffer(m_logicalDevice, m_indexBuffer, nullptr);
	vkFreeMemory(m_logicalDevice, m_indexBufferMemory, nullptr);

	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
	{
		vkDestroySemaphore(m_logicalDevice, m_semaphoresImageAvailable[i], nullptr);
		vkDestroySemaphore(m_logicalDevice, m_semaphoresRenderFinished[i], nullptr);
		vkDestroyFence(m_logicalDevice, m_inFlightFences[i], nullptr);
	}

	vkDestroyCommandPool(m_logicalDevice, m_commandPool, nullptr);
	vkDestroyDevice(m_logicalDevice, nullptr);

#ifdef VALIDATION
	vkDestroyDebugUtilsMessengerEXT_PROXY(m_instance, m_debugMessenger, nullptr);
#endif // !VALIDATION

	vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
	vkDestroyInstance(m_instance, nullptr);
}

void VulkanMain::draw()
{
	vkWaitForFences(m_logicalDevice, 1, &m_inFlightFences[m_currentFrameIndex], VK_TRUE, UINT64_MAX);

	uint32_t imageIndex;
	VkResult imageResult = vkAcquireNextImageKHR(m_logicalDevice, m_swapchain, UINT64_MAX,
	                                             m_semaphoresImageAvailable[m_currentFrameIndex], VK_NULL_HANDLE, &imageIndex);

	if (imageResult == VK_ERROR_OUT_OF_DATE_KHR)
	{
		recreateSwapChain();
		return;
	} else if (imageResult != VK_SUCCESS && imageResult != VK_SUBOPTIMAL_KHR)
	{
		__android_log_assert("Failed to acquire next image.", nullptr, nullptr);
	}


	if (m_imagesInFlight[imageIndex] != VK_NULL_HANDLE)
	{
		vkWaitForFences(m_logicalDevice, 1, &m_imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
	}

	m_imagesInFlight[imageIndex] = m_inFlightFences[m_currentFrameIndex];

	/////////////////////////////////
	UniformBufferObject ubo = {};
	ubo.model = glm::mat4(1);
	ubo.view = m_camera.getView();
	ubo.projection = m_camera.getProjection();
	ubo.projection[1][1] *= -1;

	void *data;
	vkMapMemory(m_logicalDevice, m_uniformBuffersMemory[(imageIndex * 2)], 0, sizeof(ubo), 0, &data);
	memcpy(data, &ubo, sizeof(ubo));
	vkUnmapMemory(m_logicalDevice, m_uniformBuffersMemory[(imageIndex * 2)]);
	/////////////////////////////////
	/////////////////////////////////
	ubo.model = glm::mat4(1);
	ubo.model = glm::translate(ubo.model, {1, 2, -1});

	vkMapMemory(m_logicalDevice, m_uniformBuffersMemory[(imageIndex * 2) + 1], 0, sizeof(ubo), 0, &data);
	memcpy(data, &ubo, sizeof(ubo));
	vkUnmapMemory(m_logicalDevice, m_uniformBuffersMemory[(imageIndex * 2) + 1]);
	/////////////////////////////////

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	VkPipelineStageFlags waitFlag = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = &m_semaphoresImageAvailable[m_currentFrameIndex];
	submitInfo.pWaitDstStageMask = &waitFlag;

	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &m_commandBuffers[imageIndex];

	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &m_semaphoresRenderFinished[m_currentFrameIndex];

	vkResetFences(m_logicalDevice, 1, &m_inFlightFences[m_currentFrameIndex]);
	CALL_VK(vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, m_inFlightFences[m_currentFrameIndex]));

	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = &m_semaphoresRenderFinished[m_currentFrameIndex];

	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &m_swapchain;
	presentInfo.pImageIndices = &imageIndex;

	imageResult = vkQueuePresentKHR(m_presentQueue, &presentInfo);

	if (imageResult == VK_ERROR_OUT_OF_DATE_KHR || imageResult == VK_SUBOPTIMAL_KHR || m_framebufferResized)
	{
		m_framebufferResized = false;
		recreateSwapChain();
	} else
	{
		CALL_VK(imageResult);
	}

	m_currentFrameIndex = (m_currentFrameIndex + 1) % MAX_FRAMES_IN_FLIGHT;
}


void VulkanMain::createInstance()
{
#ifdef VALIDATION
	VkDebugUtilsMessengerCreateInfoEXT messengerInfo;
	constexpr VkDebugUtilsMessageSeverityFlagsEXT kSeveritiesToLog =
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;

	constexpr VkDebugUtilsMessageTypeFlagsEXT kMessagesToLog =
			VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;

	messengerInfo.sType           = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	messengerInfo.pNext           = nullptr;
	messengerInfo.flags           = 0;
	messengerInfo.messageSeverity = kSeveritiesToLog;
	messengerInfo.messageType     = kMessagesToLog;
	messengerInfo.pfnUserCallback = &debugCallback; // Callback example below
	messengerInfo.pUserData       = nullptr; // Custom user data passed to callback

#endif // !VALIDATION

	VkApplicationInfo applicationInfo = {};
	applicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	applicationInfo.apiVersion = VK_API_VERSION_1_0;

	applicationInfo.pEngineName = "No Engine";
	applicationInfo.engineVersion = VK_MAKE_VERSION(0, 0, 1);

	applicationInfo.pApplicationName = "Vulkan Test";
	applicationInfo.applicationVersion = VK_MAKE_VERSION(0, 0, 1);


	VkInstanceCreateInfo instanceCreateInfo = {};
	instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instanceCreateInfo.pNext = nullptr;
	instanceCreateInfo.flags = 0;

#ifdef VALIDATION
	INSTANCE_EXTENSIONS.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

	instanceCreateInfo.enabledLayerCount = static_cast<uint32_t>(VALIDATION_LAYERS.size());
	instanceCreateInfo.ppEnabledLayerNames = VALIDATION_LAYERS.data();

	instanceCreateInfo.pNext = nullptr;
#else
	instanceCreateInfo.enabledLayerCount = 0;
	instanceCreateInfo.ppEnabledLayerNames = nullptr;
#endif // !VALIDATION

	instanceCreateInfo.enabledExtensionCount = INSTANCE_EXTENSIONS.size();
	instanceCreateInfo.ppEnabledExtensionNames = INSTANCE_EXTENSIONS.data();

	instanceCreateInfo.pApplicationInfo = &applicationInfo;

	CALL_VK(vkCreateInstance(&instanceCreateInfo, nullptr, &m_instance));

#ifdef VALIDATION
	CALL_VK(vkCreateDebugUtilsMessengerEXT_PROXY(m_instance, &messengerInfo, nullptr, &m_debugMessenger));
#endif // !VALIDATION
}

void VulkanMain::createSurface()
{
	VkAndroidSurfaceCreateInfoKHR androidSurfaceCreateInfo = {};
	androidSurfaceCreateInfo.sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR;
	androidSurfaceCreateInfo.flags = 0;
	androidSurfaceCreateInfo.pNext = nullptr;
	androidSurfaceCreateInfo.window = m_pApp->window;

	CALL_VK(vkCreateAndroidSurfaceKHR(m_instance, &androidSurfaceCreateInfo, nullptr, &m_surface));
}

void VulkanMain::createDevice()
{
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(m_instance, &deviceCount, nullptr);

	if (deviceCount == 0)
	{
		__android_log_assert("No physical devices detected.", nullptr, nullptr);
	}

	std::vector<VkPhysicalDevice> physicalDevices(deviceCount);
	vkEnumeratePhysicalDevices(m_instance, &deviceCount, physicalDevices.data());

	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;

	VkPhysicalDeviceType deviceTypes[] = {
			VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU,
			VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU
	};

	VkPhysicalDeviceProperties deviceProperties;
	for (VkPhysicalDevice &pd : physicalDevices)
	{
		vkGetPhysicalDeviceProperties(pd, &deviceProperties);
		if (deviceProperties.deviceType == deviceTypes[1])
		{
			if (isDeviceSuitable(pd, m_surface))
			{
				physicalDevice = pd;
				break;
			}
		}
	}

	if (physicalDevice == VK_NULL_HANDLE)
	{
		physicalDevice = physicalDevices[0];

		if (!isDeviceSuitable(physicalDevice, m_surface))
		{
			__android_log_assert("No suitable device detected.", nullptr, nullptr);
		}
	}
	m_physicalDevice = physicalDevice;


	// Logical Device
	m_queueFamilyIndexes = getQueueFamilyIndexes(physicalDevice);

	std::set<uint32_t> uniqueQueueFamilies = {m_queueFamilyIndexes.graphical, m_queueFamilyIndexes.present};
	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;

	for (uint32_t queueFamilyIndex : uniqueQueueFamilies)
	{
		queueCreateInfos.push_back(VkDeviceQueueCreateInfo());
		setQueueCreateInfo(queueCreateInfos[queueCreateInfos.size() - 1], queueFamilyIndex, 1.0f);
	}


	VkDeviceCreateInfo deviceCreateInfo = {};
	deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

	deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
	deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();

	VkPhysicalDeviceFeatures physicalDeviceFeatures = {};
	deviceCreateInfo.pEnabledFeatures = &physicalDeviceFeatures;

	deviceCreateInfo.ppEnabledExtensionNames = DEVICE_EXTENSIONS.data();
	deviceCreateInfo.enabledExtensionCount = (uint32_t) DEVICE_EXTENSIONS.size();

	deviceCreateInfo.enabledLayerCount = 0;
	deviceCreateInfo.ppEnabledLayerNames = nullptr;

	CALL_VK(vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &m_logicalDevice))

	// Queues
	vkGetDeviceQueue(m_logicalDevice, m_queueFamilyIndexes.graphical, 0, &m_graphicsQueue);
	vkGetDeviceQueue(m_logicalDevice, m_queueFamilyIndexes.present, 0, &m_presentQueue);
}

void VulkanMain::createSwapChain()
{
	// Swapchain
	m_swapchainSupportDetails = getSwapChainSupportDetails(m_physicalDevice, m_surface, ANativeWindow_getWidth(m_pApp->window), ANativeWindow_getHeight(m_pApp->window));

	m_swapchain = createSwapchain(VK_NULL_HANDLE, m_swapchainSupportDetails, m_logicalDevice, m_surface, m_queueFamilyIndexes);

	// Images TODO
	uint32_t nImages = 0;
	vkGetSwapchainImagesKHR(m_logicalDevice, m_swapchain, &nImages, nullptr);
	m_images.resize(nImages);
	vkGetSwapchainImagesKHR(m_logicalDevice, m_swapchain, &nImages, m_images.data());
}

std::vector<VkImageView> VulkanMain::createImageViews(VkDevice logicalDevice, std::vector<VkImage> &images, SwapChainSupportDetails &swapchainSupportDetails) const
{
	std::vector<VkImageView> imageViews(images.size());

	VkImageViewCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;

	createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	createInfo.format = swapchainSupportDetails.surfaceFormat.format;

	createInfo.components = {
			VK_COMPONENT_SWIZZLE_IDENTITY,
			VK_COMPONENT_SWIZZLE_IDENTITY,
			VK_COMPONENT_SWIZZLE_IDENTITY,
			VK_COMPONENT_SWIZZLE_IDENTITY
	};

	createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	createInfo.subresourceRange.baseMipLevel = 0;
	createInfo.subresourceRange.levelCount = 1;
	createInfo.subresourceRange.baseArrayLayer = 0;
	createInfo.subresourceRange.layerCount = 1;

	for (int i = imageViews.size() - 1; i >= 0; --i)
	{
		createInfo.image = images[i];

		CALL_VK(vkCreateImageView(logicalDevice, &createInfo, nullptr, &imageViews[i]))
	}

	return imageViews;
}

void VulkanMain::createGraphicsPipeline(const char *vertexPath, const char *fragmentPath)
{
	VkShaderModule vertexModule = createShaderModule(vertexPath);
	VkShaderModule fragmentModule = createShaderModule(fragmentPath);

	std::vector<VkPipelineShaderStageCreateInfo> shaderStages = {
			getCreateShaderPipelineInfo(vertexModule, VK_SHADER_STAGE_VERTEX_BIT),
			getCreateShaderPipelineInfo(fragmentModule, VK_SHADER_STAGE_FRAGMENT_BIT)
	};

	///////////////////////////////
	////// VERTEX ATTRIBUTES //////
	///////////////////////////////

	VkVertexInputBindingDescription bindingDescription = Vertex::getBindingDescription();
	std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions = Vertex::getAttributeDescriptions();

	VkPipelineVertexInputStateCreateInfo vertexInputCreateInfo = {};
	vertexInputCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputCreateInfo.vertexBindingDescriptionCount = 1;
	vertexInputCreateInfo.pVertexBindingDescriptions = &bindingDescription;
	vertexInputCreateInfo.vertexAttributeDescriptionCount = attributeDescriptions.size();
	vertexInputCreateInfo.pVertexAttributeDescriptions = attributeDescriptions.data();


	VkPipelineInputAssemblyStateCreateInfo inputAssemblyCreateInfo = {};
	inputAssemblyCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssemblyCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssemblyCreateInfo.primitiveRestartEnable = VK_FALSE;

	VkViewport viewport = {};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = (float) m_swapchainSupportDetails.extent.width;
	viewport.height = (float) m_swapchainSupportDetails.extent.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor = {};
	scissor.offset = {0, 0};
	scissor.extent = m_swapchainSupportDetails.extent;

	VkPipelineViewportStateCreateInfo viewportStateCreateInfo = {};
	viewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportStateCreateInfo.viewportCount = 1;
	viewportStateCreateInfo.pViewports = &viewport;
	viewportStateCreateInfo.scissorCount = 1;
	viewportStateCreateInfo.pScissors = &scissor;

	VkPipelineRasterizationStateCreateInfo rasterizerCreateInfo = {};
	rasterizerCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizerCreateInfo.depthClampEnable = VK_FALSE;
	rasterizerCreateInfo.rasterizerDiscardEnable = VK_FALSE;
	rasterizerCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizerCreateInfo.lineWidth = 1.0f;
	rasterizerCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizerCreateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	// SHADOW MAPPING SHIT
	rasterizerCreateInfo.depthBiasEnable = VK_FALSE;
	rasterizerCreateInfo.depthBiasConstantFactor = 0.0f;
	rasterizerCreateInfo.depthBiasClamp = 0.0f;
	rasterizerCreateInfo.depthBiasSlopeFactor = 0.0f;

	VkPipelineMultisampleStateCreateInfo multisamplingCreateInfo = {};
	multisamplingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisamplingCreateInfo.sampleShadingEnable = VK_FALSE;
	multisamplingCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisamplingCreateInfo.minSampleShading = 1.0f;
	multisamplingCreateInfo.pSampleMask = nullptr;
	multisamplingCreateInfo.alphaToCoverageEnable = VK_FALSE;
	multisamplingCreateInfo.alphaToOneEnable = VK_FALSE;

	VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
	colorBlendAttachment.colorWriteMask =
			VK_COLOR_COMPONENT_R_BIT |
			VK_COLOR_COMPONENT_G_BIT |
			VK_COLOR_COMPONENT_B_BIT |
			VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_TRUE;
	colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
	colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

	VkPipelineColorBlendStateCreateInfo colorBlendingCreateInfo = {};
	colorBlendingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlendingCreateInfo.logicOpEnable = VK_FALSE;
	colorBlendingCreateInfo.logicOp = VK_LOGIC_OP_COPY;
	colorBlendingCreateInfo.attachmentCount = 1;
	colorBlendingCreateInfo.pAttachments = &colorBlendAttachment;
	colorBlendingCreateInfo.blendConstants[0] = 0.0f;
	colorBlendingCreateInfo.blendConstants[1] = 0.0f;
	colorBlendingCreateInfo.blendConstants[2] = 0.0f;
	colorBlendingCreateInfo.blendConstants[3] = 0.0f;

	VkDynamicState dynamicState = VK_DYNAMIC_STATE_VIEWPORT;
	VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo = {};
	dynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicStateCreateInfo.dynamicStateCount = 1;
	dynamicStateCreateInfo.pDynamicStates = &dynamicState;

	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
	pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutCreateInfo.setLayoutCount = 1;
	pipelineLayoutCreateInfo.pSetLayouts = &m_uboDescriptorSetLayout;
	pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
	pipelineLayoutCreateInfo.pPushConstantRanges = nullptr;
	CALL_VK(vkCreatePipelineLayout(m_logicalDevice, &pipelineLayoutCreateInfo, nullptr, &m_pipelineLayout));

	//////////////////////
	////// PIPELINE //////
	//////////////////////

	VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo = {};
	graphicsPipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	graphicsPipelineCreateInfo.stageCount = shaderStages.size();
	graphicsPipelineCreateInfo.pStages = shaderStages.data();
	graphicsPipelineCreateInfo.pVertexInputState = &vertexInputCreateInfo;
	graphicsPipelineCreateInfo.pInputAssemblyState = &inputAssemblyCreateInfo;
	graphicsPipelineCreateInfo.pViewportState = &viewportStateCreateInfo;
	graphicsPipelineCreateInfo.pRasterizationState = &rasterizerCreateInfo;
	graphicsPipelineCreateInfo.pMultisampleState = &multisamplingCreateInfo;
	graphicsPipelineCreateInfo.pDepthStencilState = nullptr;
	graphicsPipelineCreateInfo.pColorBlendState = &colorBlendingCreateInfo;
	graphicsPipelineCreateInfo.pDynamicState = nullptr;
	graphicsPipelineCreateInfo.layout = m_pipelineLayout;
	graphicsPipelineCreateInfo.renderPass = m_renderPass;
	graphicsPipelineCreateInfo.subpass = 0;
	graphicsPipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
	graphicsPipelineCreateInfo.basePipelineIndex = -1;

	CALL_VK(vkCreateGraphicsPipelines(m_logicalDevice, VK_NULL_HANDLE, 1, &graphicsPipelineCreateInfo, nullptr, &m_graphicsPipeline));

	vkDestroyShaderModule(m_logicalDevice, vertexModule, nullptr);
	vkDestroyShaderModule(m_logicalDevice, fragmentModule, nullptr);
}

void VulkanMain::createFramebuffers()
{
	m_framebuffers.resize(m_imageViews.size());

	for (int i = m_imageViews.size() - 1; i >= 0; --i)
	{
		VkFramebufferCreateInfo framebufferCreateInfo = {};
		framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferCreateInfo.renderPass = m_renderPass;
		framebufferCreateInfo.attachmentCount = 1;
		framebufferCreateInfo.pAttachments = &m_imageViews[i];
		framebufferCreateInfo.width = m_swapchainSupportDetails.extent.width;
		framebufferCreateInfo.height = m_swapchainSupportDetails.extent.height;
		framebufferCreateInfo.layers = 1;

		CALL_VK(vkCreateFramebuffer(m_logicalDevice, &framebufferCreateInfo, nullptr, &m_framebuffers[i]));
	}
}

void VulkanMain::createCommandPool()
{
	VkCommandPoolCreateInfo commandPoolCreateInfo = {};
	commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	commandPoolCreateInfo.queueFamilyIndex = m_queueFamilyIndexes.graphical;
	commandPoolCreateInfo.flags = 0;

	CALL_VK(vkCreateCommandPool(m_logicalDevice, &commandPoolCreateInfo, nullptr, &m_commandPool));
}

void VulkanMain::createVertexBuffer()
{
	VkDeviceSize bufferSize = sizeof(m_vertices[0]) * m_vertices.size();

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;

	createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
	             VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
	             &stagingBuffer, &stagingBufferMemory);

	void *data;
	vkMapMemory(m_logicalDevice, stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, m_vertices.data(), (size_t) bufferSize);
	vkUnmapMemory(m_logicalDevice, stagingBufferMemory);


	createBuffer(bufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
	             VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
	             &m_vertexBuffer, &m_vertexBufferMemory);

	copyBuffer(stagingBuffer, m_vertexBuffer, bufferSize);

	vkDestroyBuffer(m_logicalDevice, stagingBuffer, nullptr);
	vkFreeMemory(m_logicalDevice, stagingBufferMemory, nullptr);
}

void VulkanMain::createIndexBuffer()
{
	VkDeviceSize bufferSize = sizeof(m_indexes[0]) * m_indexes.size();

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;

	createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
	             VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
	             &stagingBuffer, &stagingBufferMemory);

	void *data;
	vkMapMemory(m_logicalDevice, stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, m_indexes.data(), (size_t) bufferSize);
	vkUnmapMemory(m_logicalDevice, stagingBufferMemory);

	createBuffer(bufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
	             VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &m_indexBuffer, &m_indexBufferMemory);

	copyBuffer(stagingBuffer, m_indexBuffer, bufferSize);

	vkDestroyBuffer(m_logicalDevice, stagingBuffer, nullptr);
	vkFreeMemory(m_logicalDevice, stagingBufferMemory, nullptr);
}

void VulkanMain::createUniformBuffers()
{
	VkDeviceSize bufferSize = sizeof(UniformBufferObject);

	m_uniformBuffers.resize(m_images.size() * 2);
	m_uniformBuffersMemory.resize(m_uniformBuffers.size());

	for (int i = 0; i < m_uniformBuffers.size(); ++i)
	{
		createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		             VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		             &m_uniformBuffers[i], &m_uniformBuffersMemory[i]);
	}
}

void VulkanMain::createDescriptorPool()
{
	VkDescriptorPoolSize descriptorPoolSize = {};
	descriptorPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorPoolSize.descriptorCount = (uint32_t) m_uniformBuffers.size();

	VkDescriptorPoolCreateInfo descriptorPoolCreateInfo = {};
	descriptorPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	descriptorPoolCreateInfo.poolSizeCount = 1;
	descriptorPoolCreateInfo.pPoolSizes = &descriptorPoolSize;
	descriptorPoolCreateInfo.maxSets = (uint32_t) m_uniformBuffers.size();
	descriptorPoolCreateInfo.flags = 0;

	CALL_VK(vkCreateDescriptorPool(m_logicalDevice, &descriptorPoolCreateInfo, nullptr, &m_descriptorPool));
}

void VulkanMain::createDescriptorSets()
{
	std::vector<VkDescriptorSetLayout> layouts(m_uniformBuffers.size(), m_uboDescriptorSetLayout);

	VkDescriptorSetAllocateInfo descriptorSetAllocateInfo = {};
	descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	descriptorSetAllocateInfo.descriptorPool = m_descriptorPool;
	descriptorSetAllocateInfo.descriptorSetCount = (uint32_t) layouts.size();
	descriptorSetAllocateInfo.pSetLayouts = layouts.data();

	m_descriptorSets.resize(layouts.size());
	CALL_VK(vkAllocateDescriptorSets(m_logicalDevice, &descriptorSetAllocateInfo, m_descriptorSets.data()));

	for (int i = m_descriptorSets.size() - 1; i >= 0; --i)
	{
		VkDescriptorBufferInfo descriptorBufferInfo = {};
		descriptorBufferInfo.buffer = m_uniformBuffers[i];
		descriptorBufferInfo.offset = 0;
		descriptorBufferInfo.range = sizeof(UniformBufferObject);

		VkWriteDescriptorSet descriptorWrite = {};
		descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrite.dstSet = m_descriptorSets[i];
		descriptorWrite.dstBinding = 0;
		descriptorWrite.dstArrayElement = 0;

		descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrite.descriptorCount = 1;
		descriptorWrite.pBufferInfo = &descriptorBufferInfo;
		descriptorWrite.pImageInfo = nullptr;
		descriptorWrite.pTexelBufferView = nullptr;

		vkUpdateDescriptorSets(m_logicalDevice, 1, &descriptorWrite, 0, nullptr);
	}
}

void VulkanMain::createCommandBuffers()
{
	m_commandBuffers.resize(m_framebuffers.size());
	VkCommandBufferAllocateInfo commandBufferAllocateInfo = {};
	commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	commandBufferAllocateInfo.commandPool = m_commandPool;
	commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	commandBufferAllocateInfo.commandBufferCount = m_commandBuffers.size();

	CALL_VK(vkAllocateCommandBuffers(m_logicalDevice, &commandBufferAllocateInfo, m_commandBuffers.data()));


	VkCommandBufferBeginInfo commandBufferBeginInfo = {};
	commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	commandBufferBeginInfo.flags = 0;
	commandBufferBeginInfo.pInheritanceInfo = nullptr;

	VkRenderPassBeginInfo renderPassBeginInfo = {};
	renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassBeginInfo.renderPass = m_renderPass;
	renderPassBeginInfo.renderArea.offset = {0, 0};
	renderPassBeginInfo.renderArea.extent = m_swapchainSupportDetails.extent;
	VkClearValue clearColor = {0.0f, 0.0f, 0.0f, 1.0f};
	renderPassBeginInfo.clearValueCount = 1;
	renderPassBeginInfo.pClearValues = &clearColor;


	for (int i = m_commandBuffers.size() - 1; i >= 0; --i)
	{
		CALL_VK(vkBeginCommandBuffer(m_commandBuffers[i], &commandBufferBeginInfo));

		renderPassBeginInfo.framebuffer = m_framebuffers[i];

		vkCmdBeginRenderPass(m_commandBuffers[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
		vkCmdBindPipeline(m_commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipeline);

		VkBuffer vertexBuffers[] = {m_vertexBuffer};
		VkDeviceSize offsets[] = {0};
		vkCmdBindVertexBuffers(m_commandBuffers[i], 0, 1, vertexBuffers, offsets);
		vkCmdBindIndexBuffer(m_commandBuffers[i], m_indexBuffer, 0, VK_INDEX_TYPE_UINT16);

		vkCmdBindDescriptorSets(m_commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0, 1, &m_descriptorSets[(i * 2)], 0, nullptr);
		vkCmdDrawIndexed(m_commandBuffers[i], (uint32_t) m_indexes.size(), 1, 0, 0, 0);

		vkCmdBindDescriptorSets(m_commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0, 1, &m_descriptorSets[(i * 2) + 1], 0, nullptr);
		vkCmdDrawIndexed(m_commandBuffers[i], (uint32_t) m_indexes.size(), 1, 0, 0, 0);

		vkCmdEndRenderPass(m_commandBuffers[i]);

		CALL_VK(vkEndCommandBuffer(m_commandBuffers[i]))
	}
}

void VulkanMain::createSyncObjects()
{
	////////////////////////
	////// SEMAPHORES //////
	////////////////////////

	m_semaphoresImageAvailable.resize(MAX_FRAMES_IN_FLIGHT);
	m_semaphoresRenderFinished.resize(MAX_FRAMES_IN_FLIGHT);

	VkSemaphoreCreateInfo semaphoreCreateInfo = {};
	semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
	{
		CALL_VK(vkCreateSemaphore(m_logicalDevice, &semaphoreCreateInfo, nullptr, &m_semaphoresImageAvailable[i]));
		CALL_VK(vkCreateSemaphore(m_logicalDevice, &semaphoreCreateInfo, nullptr, &m_semaphoresRenderFinished[i]));
	}

	////////////////////
	////// FENCES //////
	////////////////////

	m_inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

	VkFenceCreateInfo fenceCreateInfo = {};
	fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
	{
		CALL_VK(vkCreateFence(m_logicalDevice, &fenceCreateInfo, nullptr, &m_inFlightFences[i]));
	}

	//// IMAGE FENCES ////
	m_imagesInFlight.resize(m_images.size(), VK_NULL_HANDLE);
}

void VulkanMain::createRenderPass()
{
	VkAttachmentDescription colorAttachment = {};
	colorAttachment.format = m_swapchainSupportDetails.surfaceFormat.format;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference colorAttachmentReference = {};
	colorAttachmentReference.attachment = 0;
	colorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentReference;

	VkRenderPassCreateInfo renderPassCreateInfo = {};
	renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassCreateInfo.attachmentCount = 1;
	renderPassCreateInfo.pAttachments = &colorAttachment;
	renderPassCreateInfo.subpassCount = 1;
	renderPassCreateInfo.pSubpasses = &subpass;

	VkSubpassDependency dependency = {};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	renderPassCreateInfo.dependencyCount = 1;
	renderPassCreateInfo.pDependencies = &dependency;

	CALL_VK(vkCreateRenderPass(m_logicalDevice, &renderPassCreateInfo, nullptr, &m_renderPass));
}

void VulkanMain::createDescriptorSetLayout()
{
	VkDescriptorSetLayoutBinding uboLayoutBinding = {};
	uboLayoutBinding.binding = 0;
	uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uboLayoutBinding.descriptorCount = 1;
	uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	uboLayoutBinding.pImmutableSamplers = nullptr;

	VkDescriptorSetLayoutCreateInfo descriptorSetCreateInfo = {};
	descriptorSetCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	descriptorSetCreateInfo.bindingCount = 1;
	descriptorSetCreateInfo.pBindings = &uboLayoutBinding;

	CALL_VK(vkCreateDescriptorSetLayout(m_logicalDevice, &descriptorSetCreateInfo, nullptr, &m_uboDescriptorSetLayout));
}


void VulkanMain::cleanupSwapChain()
{
	for (VkFramebuffer framebuffer : m_framebuffers)
	{
		vkDestroyFramebuffer(m_logicalDevice, framebuffer, nullptr);
	}

	vkFreeCommandBuffers(m_logicalDevice, m_commandPool, static_cast<uint32_t>(m_commandBuffers.size()), m_commandBuffers.data());

	vkDestroyPipeline(m_logicalDevice, m_graphicsPipeline, nullptr);
	vkDestroyPipelineLayout(m_logicalDevice, m_pipelineLayout, nullptr);
	vkDestroyRenderPass(m_logicalDevice, m_renderPass, nullptr);

	for (VkImageView imageView : m_imageViews)
	{
		vkDestroyImageView(m_logicalDevice, imageView, nullptr);
	}

	vkDestroySwapchainKHR(m_logicalDevice, m_swapchain, nullptr);

	for (int i = 0; i < m_uniformBuffers.size(); ++i)
	{
		vkDestroyBuffer(m_logicalDevice, m_uniformBuffers[i], nullptr);
		vkFreeMemory(m_logicalDevice, m_uniformBuffersMemory[i], nullptr);
	}

	vkDestroyDescriptorPool(m_logicalDevice, m_descriptorPool, nullptr);
}

void VulkanMain::recreateSwapChain()
{
	vkDeviceWaitIdle(m_logicalDevice);

	cleanupSwapChain();
	createSwapChain();
	m_imageViews = createImageViews(m_logicalDevice, m_images, m_swapchainSupportDetails);
	createRenderPass();
	createGraphicsPipeline("triangle.vert.spv", "triangle.frag.spv");

	createFramebuffers();
	createUniformBuffers();
	createDescriptorPool();
	createDescriptorSets();

	createCommandBuffers();
}

bool VulkanMain::isDeviceSuitable(VkPhysicalDevice physicalDevice, VkSurfaceKHR surfaceHandle)
{
	bool isPhysicalDeviceSuitable = true;

	uint32_t extentionCount;
	vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extentionCount, nullptr);

	std::vector<VkExtensionProperties> extentionProperties(extentionCount);
	vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extentionCount, extentionProperties.data());

	std::set<std::string> m_extentionsLeft(DEVICE_EXTENSIONS.begin(), DEVICE_EXTENSIONS.end());
	for (VkExtensionProperties &ep : extentionProperties)
	{
		m_extentionsLeft.erase(ep.extensionName);
	}
	isPhysicalDeviceSuitable = isPhysicalDeviceSuitable && m_extentionsLeft.empty();

	if (isPhysicalDeviceSuitable)
	{
		isPhysicalDeviceSuitable = isPhysicalDeviceSuitable &&
		                           getQueueGraphicsFamilyIndex(physicalDevice, nullptr) &&
		                           getQueuePresentFamilyIndex(physicalDevice, m_surface, nullptr, 0);
	}

	if (isPhysicalDeviceSuitable)
	{
		SwapChainSupportDetails swapChainDetails = getSwapChainSupportDetails(physicalDevice, m_surface, ANativeWindow_getWidth(m_pApp->window), ANativeWindow_getHeight(m_pApp->window));

		uint32_t nSurfaceFormats = 0;
		vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surfaceHandle, &nSurfaceFormats, nullptr);

		uint32_t nPresentModes = 0;
		vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surfaceHandle, &nPresentModes, nullptr);

		isPhysicalDeviceSuitable = isPhysicalDeviceSuitable && nSurfaceFormats != 0 && nPresentModes != 0;
	}

	return isPhysicalDeviceSuitable;
}

QueueFamilyIndexes VulkanMain::getQueueFamilyIndexes(VkPhysicalDevice physicalDevice)
{
	QueueFamilyIndexes familyIndexes = {0, 0};
	if (!getQueueGraphicsFamilyIndex(physicalDevice, &familyIndexes.graphical))
	{
		__android_log_assert("Failed to get graphics family index.", nullptr, nullptr);
	}

	if (!getQueuePresentFamilyIndex(physicalDevice, m_surface, &familyIndexes.present, familyIndexes.graphical))
	{
		__android_log_assert("Failed to get present family index.", nullptr, nullptr);
	}

	return familyIndexes;
}

bool VulkanMain::getQueueGraphicsFamilyIndex(VkPhysicalDevice physicalDevice, uint32_t *index)
{
	uint32_t nQueueFamilies;
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &nQueueFamilies, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilyProperties(nQueueFamilies);
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &nQueueFamilies, queueFamilyProperties.data());

	bool found = false;
	for (uint32_t i = 0; i < nQueueFamilies; ++i)
	{
		if (queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
		{
			if (index != nullptr)
			{
				*index = i;
			}

			return true;
		}
	}

	return found;
}

bool VulkanMain::getQueuePresentFamilyIndex(VkPhysicalDevice physicalDevice, VkSurfaceKHR surfaceHandle, uint32_t *index, uint32_t priorityIndex)
{
	uint32_t nQueueFamilies;
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &nQueueFamilies, nullptr);

	VkBool32 supported;
	vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, priorityIndex, surfaceHandle, &supported);
	if (supported)
	{
		if (index != nullptr)
		{
			*index = priorityIndex;
		}

		return true;
	}

	for (uint32_t i = 0; i < nQueueFamilies; ++i)
	{
		if (i == priorityIndex)
			continue;

		vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surfaceHandle, &supported);
		if (supported)
		{
			if (index != nullptr)
			{
				*index = i;
			}

			return true;
		}
	}

	return false;
}

void VulkanMain::setQueueCreateInfo(VkDeviceQueueCreateInfo &queueCreateInfo, uint32_t index, float priority)
{
	queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queueCreateInfo.queueFamilyIndex = index;
	queueCreateInfo.queueCount = 1;
	queueCreateInfo.pQueuePriorities = &priority;
}

VkSwapchainKHR VulkanMain::createSwapchain(VkSwapchainKHR oldSwapchain, const SwapChainSupportDetails &swapChainSupportDetails, VkDevice logicalDevice, VkSurfaceKHR surfaceHandle, QueueFamilyIndexes &familyIndexes) const
{
	// minImageCount + 1 to avoid waiting
	uint32_t nImages = swapChainSupportDetails.capabilities.minImageCount + 1;

	// VkSurfaceCapabilitiesKHR::maxImageCount == 0 --> No max
	if (swapChainSupportDetails.capabilities.maxImageCount > 0 && nImages > swapChainSupportDetails.capabilities.maxImageCount)
	{
		nImages = swapChainSupportDetails.capabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = surfaceHandle;
	createInfo.minImageCount = nImages;
	createInfo.imageFormat = swapChainSupportDetails.surfaceFormat.format;
	createInfo.imageColorSpace = swapChainSupportDetails.surfaceFormat.colorSpace;
	createInfo.imageExtent = swapChainSupportDetails.extent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	if (familyIndexes.graphical != familyIndexes.present)
	{
		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		createInfo.queueFamilyIndexCount = 2;
		createInfo.pQueueFamilyIndices = (uint32_t *) &familyIndexes;
	} else
	{
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		createInfo.queueFamilyIndexCount = 0;
		createInfo.pQueueFamilyIndices = nullptr;
	}

	createInfo.preTransform = swapChainSupportDetails.capabilities.currentTransform;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createInfo.presentMode = swapChainSupportDetails.presentMode;
	createInfo.clipped = VK_TRUE;
	createInfo.oldSwapchain = oldSwapchain;

	VkSwapchainKHR swapchain;
	CALL_VK(vkCreateSwapchainKHR(logicalDevice, &createInfo, nullptr, &swapchain));

	return swapchain;
}

SwapChainSupportDetails VulkanMain::getSwapChainSupportDetails(VkPhysicalDevice physicalDevice, VkSurfaceKHR surfaceHandle, uint32_t width, uint32_t height)
{
	SwapChainSupportDetails details;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surfaceHandle, &details.capabilities);

	std::vector<VkSurfaceFormatKHR> swapChainFormats;
	uint32_t nSurfaceFormats = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surfaceHandle, &nSurfaceFormats, nullptr);
	if (nSurfaceFormats != 0)
	{
		swapChainFormats.resize(nSurfaceFormats);
		vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surfaceHandle, &nSurfaceFormats, swapChainFormats.data());
	}

	std::vector<VkPresentModeKHR> presentModes;
	uint32_t nPresentModes = 0;
	vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surfaceHandle, &nPresentModes, nullptr);
	if (nPresentModes != 0)
	{
		presentModes.resize(nPresentModes);
		vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surfaceHandle, &nPresentModes, presentModes.data());
	}

	details.surfaceFormat = chooseSwapSurfaceFormat(swapChainFormats);
	details.presentMode = chooseSwapPresentMode(presentModes);
	details.extent = chooseSwapExtent(details.capabilities, width, height);

	return details;
}

VkSurfaceFormatKHR VulkanMain::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &swapChainFormats)
{
	for (const VkSurfaceFormatKHR &format : swapChainFormats)
	{
		if (format.format == VK_FORMAT_B8G8R8A8_UNORM && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
		{
			return format;
		}
	}

	return swapChainFormats[0];
}

VkPresentModeKHR VulkanMain::chooseSwapPresentMode(const std::vector<VkPresentModeKHR> &presentModes)
{
	for (const VkPresentModeKHR &presentMode : presentModes)
	{
		if (presentMode == VK_PRESENT_MODE_MAILBOX_KHR)
		{
			return presentMode;
		}
	}

	// always avalible
	return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D VulkanMain::chooseSwapExtent(const VkSurfaceCapabilitiesKHR &swapChainCapabilities, uint32_t width, uint32_t height)
{
	if (swapChainCapabilities.currentExtent.width != UINT32_MAX)
	{
		return swapChainCapabilities.currentExtent;
	}

	VkExtent2D wantedExtent = {width, height};
	VkExtent2D extentToUse = {};

	// Average extent
	extentToUse.width = std::max(swapChainCapabilities.maxImageExtent.width,
	                             std::min(swapChainCapabilities.maxImageExtent.width, wantedExtent.width));
	extentToUse.height = std::max(swapChainCapabilities.maxImageExtent.height,
	                              std::min(swapChainCapabilities.maxImageExtent.height, wantedExtent.height));

	return extentToUse;
}

uint32_t VulkanMain::findMemoryType(VkPhysicalDevice physicalDevice, uint32_t memoryTypeFilter, VkMemoryPropertyFlags memoryPropertyFlags)
{
	VkPhysicalDeviceMemoryProperties memoryProperties;
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);

	for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; ++i)
	{
		if (memoryTypeFilter & (1 << i) && (memoryProperties.memoryTypes[i].propertyFlags & memoryPropertyFlags) == memoryPropertyFlags)
		{
			return i;
		}
	}

	__android_log_assert("Failed to find memory type.", nullptr, nullptr);
}

void VulkanMain::createBuffer(VkDeviceSize size, VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags propertyFlags, VkBuffer *buffer, VkDeviceMemory *bufferMemory)
{
	VkBufferCreateInfo bufferCreateInfo = {};
	bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferCreateInfo.size = size;
	bufferCreateInfo.usage = usageFlags;
	bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	CALL_VK(vkCreateBuffer(m_logicalDevice, &bufferCreateInfo, nullptr, buffer));

	VkMemoryRequirements memoryRequirements;
	vkGetBufferMemoryRequirements(m_logicalDevice, *buffer, &memoryRequirements);


	VkMemoryAllocateInfo memoryAllocateInfo = {};
	memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memoryAllocateInfo.allocationSize = memoryRequirements.size;
	memoryAllocateInfo.memoryTypeIndex = findMemoryType(
			m_physicalDevice,
			memoryRequirements.memoryTypeBits,
			propertyFlags
	);

	CALL_VK(vkAllocateMemory(m_logicalDevice, &memoryAllocateInfo, nullptr, bufferMemory));

	vkBindBufferMemory(m_logicalDevice, *buffer, *bufferMemory, 0);
}

void VulkanMain::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
{
	VkCommandBufferAllocateInfo commandBufferAllocateInfo = {};
	commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	commandBufferAllocateInfo.commandPool = m_commandPool;
	commandBufferAllocateInfo.commandBufferCount = 1;

	VkCommandBuffer transferCommandBuffer;
	vkAllocateCommandBuffers(m_logicalDevice, &commandBufferAllocateInfo, &transferCommandBuffer);

	VkCommandBufferBeginInfo commandBufferBeginInfo = {};
	commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(transferCommandBuffer, &commandBufferBeginInfo);

	VkBufferCopy copyRegion = {};
	copyRegion.srcOffset = 0;
	copyRegion.dstOffset = 0;
	copyRegion.size = size;
	vkCmdCopyBuffer(transferCommandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

	vkEndCommandBuffer(transferCommandBuffer);

	VkSubmitInfo queueSubmitInfo = {};
	queueSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	queueSubmitInfo.commandBufferCount = 1;
	queueSubmitInfo.pCommandBuffers = &transferCommandBuffer;

	vkQueueSubmit(m_graphicsQueue, 1, &queueSubmitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(m_graphicsQueue);

	vkFreeCommandBuffers(m_logicalDevice, m_commandPool, 1, &transferCommandBuffer);
}

VkShaderModule VulkanMain::createShaderModule(const char *shaderPath)
{
	std::vector<char> shaderData = FileReader::readData(shaderPath);

	VkShaderModuleCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = shaderData.size();
	createInfo.pCode = reinterpret_cast<const uint32_t *>(shaderData.data());

	VkShaderModule shaderModule;
	CALL_VK(vkCreateShaderModule(m_logicalDevice, &createInfo, nullptr, &shaderModule));
	return shaderModule;
}


VkPipelineShaderStageCreateInfo VulkanMain::getCreateShaderPipelineInfo(VkShaderModule shaderModule, VkShaderStageFlagBits shaderStage)
{
	VkPipelineShaderStageCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	createInfo.stage = shaderStage;
	createInfo.module = shaderModule;
	createInfo.pName = "main";

	return createInfo;
}

VkFormat VulkanMain::findSupportedFormat(const std::vector<VkFormat> &candidates, VkImageTiling tiling, VkFormatFeatureFlags featureFlags)
{
	for (VkFormat format : candidates)
	{
		VkFormatProperties properties;
		vkGetPhysicalDeviceFormatProperties(m_physicalDevice, format, &properties);

		if (tiling == VK_IMAGE_TILING_LINEAR && (properties.linearTilingFeatures & featureFlags) == featureFlags)
		{
			return format;
		} else if (tiling == VK_IMAGE_TILING_OPTIMAL && (properties.optimalTilingFeatures & featureFlags) == featureFlags)
		{
			return format;
		}

		__android_log_assert("Failed to find supported format.", nullptr, nullptr);
	}
}

VkFormat VulkanMain::findDepthFormat()
{
	return findSupportedFormat(
			{VK_FORMAT_D24_UNORM_S8_UINT, VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT},
			VK_IMAGE_TILING_OPTIMAL,
			VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
	);
}

bool VulkanMain::hasStencilComponent(VkFormat format)
{
	return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}