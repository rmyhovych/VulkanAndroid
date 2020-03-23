//
// Created by rmyho on 3/23/2020.
//

#include "VulkanMain.h"

#include <jni.h>
#include <string>


#include <android/native_window.h>

#include <exception>

inline void CHECK_RESULT(VkResult result)
{
	if (result != VK_SUCCESS)
		throw std::runtime_error("Failed to perform vulkan action.");
}

class InstanceWrapper
{
public:
	InstanceWrapper()
	{
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

		instanceCreateInfo.enabledLayerCount = 0;
		instanceCreateInfo.ppEnabledLayerNames = nullptr;

		instanceCreateInfo.enabledExtensionCount = 0;
		instanceCreateInfo.ppEnabledExtensionNames = nullptr;

		instanceCreateInfo.pApplicationInfo = &applicationInfo;

		CHECK_RESULT(vkCreateInstance(&instanceCreateInfo, nullptr, &m_instance));
	}

	~InstanceWrapper()
	{
		vkDestroyInstance(m_instance, nullptr);
	}

	VkInstance getInstance()
	{
		return m_instance;
	}

private:
	VkInstance m_instance;
};

extern "C" JNIEXPORT jstring JNICALL
Java_com_example_vulkanandroid_MainActivity_stringFromJNI(
		JNIEnv *env,
		jobject /* this */)
{
	InstanceWrapper instanceWrapper;


	VkAndroidSurfaceCreateInfoKHR androidSurfaceCreateInfo = {};
	androidSurfaceCreateInfo.sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR;
	androidSurfaceCreateInfo.flags = 0;
	androidSurfaceCreateInfo.pNext = nullptr;

	ANativeWindow* nativeWindow;
	ANativeWindow_acquire(nativeWindow);
	androidSurfaceCreateInfo.window = nativeWindow;


	VkSurfaceKHR surface;
	vkCreateAndroidSurfaceKHR(instanceWrapper.getInstance(), &androidSurfaceCreateInfo, nullptr, &surface);

	vkDestroySurfaceKHR(instanceWrapper.getInstance(), surface, nullptr);

	ANativeWindow_release(nativeWindow);
	return env->NewStringUTF("Hello");
}

