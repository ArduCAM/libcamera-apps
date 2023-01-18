/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (C) 2020, Raspberry Pi (Trading) Ltd.
 *
 * rpicam_jpeg.cpp - minimal libcamera jpeg capture app.
 */

#include <chrono>
#include <poll.h>
#include <signal.h>
#include <sys/signalfd.h>
#include <sys/stat.h>

#include "core/rpicam_app.hpp"
#include "core/still_options.hpp"

#include "image/image.hpp"
#include "../SignalServer/SignalServer.hpp"

using namespace std::placeholders;
using libcamera::Stream;

static int signal_received;
static void default_signal_handler(int signal_number)
{
	signal_received = signal_number;
	LOG(1, "Received signal " << signal_number);
}
static int get_key_or_signal(StillOptions const *options, pollfd p[1])
{
	int key = 0;
	if (options->keypress)
	{
		poll(p, 1, 0);
		if (p[0].revents & POLLIN)
		{
			char *user_string = nullptr;
			size_t len;
			[[maybe_unused]] size_t r = getline(&user_string, &len, stdin);
			key = user_string[0];
		}
	}
	if (options->signal)
	{
		if (signal_received == SIGUSR1)
			key = '\n';
		else if (signal_received == SIGUSR2)
			key = 'x';
		signal_received = 0;
	}
	return key;
}

class RPiCamJpegApp : public RPiCamApp
{
public:
	RPiCamJpegApp()
		: RPiCamApp(std::make_unique<StillOptions>())
	{
	}

	StillOptions *GetOptions() const
	{
		return static_cast<StillOptions *>(options_.get());
	}
};

// The main even loop for the application.

static void event_loop(RPiCamJpegApp &app)
{
	float lens_position = 0;
	float af_step = 1;
	SignalServer signal_server(8080);
	std::string param;
	signal_server.start();

	// Monitoring for keypresses and signals.
	signal(SIGUSR1, default_signal_handler);
	signal(SIGUSR2, default_signal_handler);
	pollfd p[1] = { { STDIN_FILENO, POLLIN, 0 } };

	StillOptions const *options = app.GetOptions();
	app.OpenCamera();
	app.ConfigureViewfinder();
	app.StartCamera();
	auto start_time = std::chrono::high_resolution_clock::now();

	for (;;)
	{
		RPiCamApp::Msg msg = app.Wait();
		if (msg.type == RPiCamApp::MsgType::Timeout)
		{
			LOG_ERROR("ERROR: Device timeout detected, attempting a restart!!!");
			app.StopCamera();
			app.StartCamera();
			continue;
		}
		if (msg.type == RPiCamApp::MsgType::Quit)
			return;
		else if (msg.type != RPiCamApp::MsgType::RequestComplete)
			throw std::runtime_error("unrecognised message!");
		
		param = signal_server.read();
		int key = get_key_or_signal(options, p);
		if (!key) 
			key = param[0];
		switch (key)
		{
		case 'x':
		case 'X':
			return;
		case 'f':
		case 'F':{
			libcamera::ControlList controls;
			controls.set(controls::AfMode, controls::AfModeAuto);
			controls.set(controls::AfTrigger, controls::AfTriggerStart);
			app.SetControls(controls);
			break;
		}
		case 'd':
		case 'D':
		{
			lens_position -= af_step;
			break;
		}
		case 'a':
		case 'A':
		{
			lens_position += af_step;
			break;
		}
		default:
			(void)0;
		}

		if (key == 'a' || key == 'A' || key == 'd' || key == 'D') {
			if (options->afMode_index == controls::AfModeManual) {
				libcamera::ControlList controls;
				controls.set(controls::AfMode, controls::AfModeManual);
				controls.set(controls::LensPosition, lens_position);
				app.SetControls(controls);
				std::cout << "target_lens_position: " << lens_position << std::endl;
			} else {
				std::cout << "Please switch the focus mode to manual focus mode." << std::endl;
			}
		}

		// In viewfinder mode, simply run until the timeout. When that happens, switch to
		// capture mode.
		if (app.ViewfinderStream())
		{
			auto now = std::chrono::high_resolution_clock::now();
			if (options->timeout && (now - start_time) > options->timeout.value)
			{
				app.StopCamera();
				app.Teardown();
				app.ConfigureStill();
				app.StartCamera();
			}
			else
			{
				CompletedRequestPtr &completed_request = std::get<CompletedRequestPtr>(msg.payload);
				app.ShowPreview(completed_request, app.ViewfinderStream());
			}
		}
		// In still capture mode, save a jpeg and quit.
		else if (app.StillStream())
		{
			app.StopCamera();
			LOG(1, "Still capture image received");

			Stream *stream = app.StillStream();
			StreamInfo info = app.GetStreamInfo(stream);
			CompletedRequestPtr &payload = std::get<CompletedRequestPtr>(msg.payload);
			BufferReadSync r(&app, payload->buffers[stream]);
			const std::vector<libcamera::Span<uint8_t>> mem = r.Get();
			jpeg_save(mem, info, payload->metadata, options->output, app.CameraModel(), options);
			return;
		}
	}
}

int main(int argc, char *argv[])
{
	try
	{
		RPiCamJpegApp app;
		StillOptions *options = app.GetOptions();
		if (options->Parse(argc, argv))
		{
			if (options->verbose >= 2)
				options->Print();
			if (options->output.empty())
				throw std::runtime_error("output file name required");

			event_loop(app);
		}
	}
	catch (std::exception const &e)
	{
		LOG_ERROR("ERROR: *** " << e.what() << " ***");
		return -1;
	}
	return 0;
}
