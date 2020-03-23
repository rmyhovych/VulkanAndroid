//
// Created by rmyho on 3/22/2020.
//

#include <android/log.h>


#include <android/native_window.h>
#include <android_native_app_glue.h>

void handle_cmd(android_app* app, int32_t cmd)
{
	switch (cmd)
	{
		case APP_CMD_INIT_WINDOW:
			break;

		case APP_CMD_TERM_WINDOW:
			break;

		default:
			__android_log_print(ANDROID_LOG_INFO, "Vulkan", "event [%d] not handled", cmd);
	}
}

void android_main(struct android_app* app)
{
	app->onAppCmd = handle_cmd;

	int events;
	android_poll_source* pollSource;

	do
	{
		if (ALooper_pollAll(0, nullptr, &events, (void**)&pollSource) >= 0)
		{
			if (pollSource != nullptr)
				pollSource->process(app, pollSource);
		}


	} while (app->destroyRequested == 0);
}