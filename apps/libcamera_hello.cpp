/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (C) 2020, Raspberry Pi (Trading) Ltd.
 *
 * libcamera_hello.cpp - libcamera "hello world" app.
 */

#include <chrono>
#include <poll.h>
#include <signal.h>
#include <sys/signalfd.h>
#include <sys/stat.h>

#include "core/libcamera_app.hpp"
#include "core/hello_options.hpp"
#include "../SignalServer/SignalServer.hpp"

using namespace std::placeholders;

class LibcameraHelloApp : public LibcameraApp
{
public:
	LibcameraHelloApp()
		: LibcameraApp(std::make_unique<HelloOptions>())
	{
	}

	HelloOptions *GetOptions() const
	{
		return static_cast<HelloOptions *>(options_.get());
	}
};

// Some keypress/signal handling.

static int signal_received;
static void default_signal_handler(int signal_number)
{
	signal_received = signal_number;
	LOG(1, "Received signal " << signal_number);
}
static int get_key_or_signal(HelloOptions const *options, pollfd p[1])
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

// The main event loop for the application.

static void event_loop(LibcameraHelloApp &app)
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

	HelloOptions const *options = app.GetOptions();

	app.OpenCamera();
	app.ConfigureViewfinder();
	app.StartCamera();

	auto start_time = std::chrono::high_resolution_clock::now();

	for (unsigned int count = 0; ; count++)
	{
		LibcameraApp::Msg msg = app.Wait();
		if (msg.type == LibcameraApp::MsgType::Timeout)
		{
			LOG_ERROR("ERROR: Device timeout detected, attempting a restart!!!");
			app.StopCamera();
			app.StartCamera();
			continue;
		}
		if (msg.type == LibcameraApp::MsgType::Quit)
			return;
		else if (msg.type != LibcameraApp::MsgType::RequestComplete)
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

		LOG(2, "Viewfinder frame " << count);
		auto now = std::chrono::high_resolution_clock::now();
		if (options->timeout && now - start_time > std::chrono::milliseconds(options->timeout))
			return;

		CompletedRequestPtr &completed_request = std::get<CompletedRequestPtr>(msg.payload);
		app.ShowPreview(completed_request, app.ViewfinderStream());
	}
}

int main(int argc, char *argv[])
{
	try
	{
		LibcameraHelloApp app;
		HelloOptions *options = app.GetOptions();
		if (options->Parse(argc, argv))
		{
			if (options->verbose >= 2)
				options->Print();

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
