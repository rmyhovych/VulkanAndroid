cmake_minimum_required(VERSION 3.4.1)


set(SRC_PATH src/main/cpp)


set(VULKAN_ANDROID_HDRS

		${SRC_PATH}/vulkan_wrapper.h

		${SRC_PATH}/VulkanMain.h
		${SRC_PATH}/FileReader.h
		${SRC_PATH}/camera/Camera.h
		${SRC_PATH}/camera/FocusedCamera.h)


set(VULKAN_ANDROID_SRC

		${SRC_PATH}/Main.cpp

		${SRC_PATH}/VulkanMain.cpp
		${SRC_PATH}/FileReader.cpp
		${SRC_PATH}/camera/Camera.cpp
		${SRC_PATH}/camera/FocusedCamera.cpp)


add_library(VulkanAndroid
		SHARED

		${VULKAN_ANDROID_HDRS}
		${VULKAN_ANDROID_SRC})


set(APP_GLUE_DIR ${ANDROID_NDK}/sources/android/native_app_glue)
include_directories(${APP_GLUE_DIR})
add_library(app-glue STATIC ${APP_GLUE_DIR}/android_native_app_glue.c)


include_directories(libs)

set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -u ANativeActivity_onCreate")
target_link_libraries(VulkanAndroid

		android
		vulkan

		app-glue
		log)
