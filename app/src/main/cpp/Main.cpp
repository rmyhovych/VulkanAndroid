//
// Created by rmyho on 3/22/2020.
//

#include <android/log.h>

#include "VulkanMain.h"

#include <android_native_app_glue.h>
#include "FileReader.h"


void handle_cmd(android_app* app, int32_t cmd)
{
	switch (cmd)
	{
		case APP_CMD_INIT_WINDOW:
			static_cast<VulkanMain*>(app->userData)->init(app);
			break;

		case APP_CMD_TERM_WINDOW:
			static_cast<VulkanMain*>(app->userData)->destroy();
			break;

		default:
			__android_log_print(ANDROID_LOG_INFO, "Vulkan", "event [%d] not handled", cmd);
	}
}

int32_t handle_input(android_app* app, AInputEvent* inputEvent)
{
	return 0;
}

void android_main(struct android_app* app)
{
	FileReader::setup(app->activity->assetManager);

	VulkanMain vulkanMain;
	app->userData = &vulkanMain;

	app->onAppCmd = handle_cmd;
	app->onInputEvent = handle_input;

	int events;
	android_poll_source* pollSource;

	do
	{
		if (ALooper_pollAll(vulkanMain.m_isReady ? 1 : 0, nullptr, &events, (void**)&pollSource) >= 0)
		{
			if (pollSource != nullptr)
				pollSource->process(app, pollSource);
		}

		if (vulkanMain.m_isReady)
		{
			vulkanMain.draw();
		}
	} while (app->destroyRequested == 0);
}