/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (C) 2020, Raspberry Pi (Trading) Ltd.
 *
 * rpicam_vid.cpp - libcamera video record app.
 */

#include <chrono>
#include <poll.h>
#include <signal.h>
#include <sys/signalfd.h>
#include <sys/stat.h>

#include "core/rpicam_encoder.hpp"
#include "output/output.hpp"
#include "../SignalServer/SignalServer.hpp"

using namespace std::placeholders;

// Some keypress/signal handling.

static int signal_received;
static void default_signal_handler(int signal_number)
{
	signal_received = signal_number;
	LOG(1, "Received signal " << signal_number);
}

static int get_key_or_signal(VideoOptions const *options, pollfd p[1])
{
	int key = 0;
	if (signal_received == SIGINT)
		return 'x';
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
		else if ((signal_received == SIGUSR2) || (signal_received == SIGPIPE))
			key = 'x';
		signal_received = 0;
	}
	return key;
}

static int get_colourspace_flags(std::string const &codec)
{
	if (codec == "mjpeg" || codec == "yuv420")
		return RPiCamEncoder::FLAG_VIDEO_JPEG_COLOURSPACE;
	else
		return RPiCamEncoder::FLAG_VIDEO_NONE;
}

// The main even loop for the application.

static void event_loop(RPiCamEncoder &app)
{
	SignalServer signal_server(8080);
	std::string param;
	signal_server.start();

	float scale = 0.0;
	float offset_x = 0.0;
	float offset_y = 0.0;
	float lens_position = 0;
	float af_step = 1;

	VideoOptions const *options = app.GetOptions();
	std::unique_ptr<Output> output = std::unique_ptr<Output>(Output::Create(options));
	app.SetEncodeOutputReadyCallback(std::bind(&Output::OutputReady, output.get(), _1, _2, _3, _4));
	app.SetMetadataReadyCallback(std::bind(&Output::MetadataReady, output.get(), _1));

	app.OpenCamera();
	app.ConfigureVideo(get_colourspace_flags(options->codec));
	app.StartEncoder();
	app.StartCamera();
	auto start_time = std::chrono::high_resolution_clock::now();

	// Monitoring for keypresses and signals.
	signal(SIGUSR1, default_signal_handler);
	signal(SIGUSR2, default_signal_handler);
	signal(SIGINT, default_signal_handler);
	// SIGPIPE gets raised when trying to write to an already closed socket. This can happen, when
	// you're using TCP to stream to VLC and the user presses the stop button in VLC. Catching the
	// signal to be able to react on it, otherwise the app terminates.
	signal(SIGPIPE, default_signal_handler);
	pollfd p[1] = { { STDIN_FILENO, POLLIN, 0 } };

	for (unsigned int count = 0; ; count++)
	{
		param = signal_server.read();
		RPiCamEncoder::Msg msg = app.Wait();
		if (msg.type == RPiCamApp::MsgType::Timeout)
		{
			LOG_ERROR("ERROR: Device timeout detected, attempting a restart!!!");
			app.StopCamera();
			app.StartCamera();
			continue;
		}
		if (msg.type == RPiCamEncoder::MsgType::Quit)
			return;
		else if (msg.type != RPiCamEncoder::MsgType::RequestComplete)
			throw std::runtime_error("unrecognised message!");
		int key = get_key_or_signal(options, p);
		if (key == '\n')
			output->Signal();
		if (!key)
			key = param[0];
		switch (key)
		{
			case 'w':
			case 'W':
			{
				scale += 0.05;
				break;
			}
			case 'f':
			case 'F': 
			{
				libcamera::ControlList cl;
				cl.set(controls::AfMode, controls::AfModeAuto);
				cl.set(controls::AfTrigger, controls::AfTriggerStart);
				app.SetControls(cl);
				std::cout << "AfTrigger" << std::endl;
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
			case 's':
			case 'S':
			{
				scale -= 0.05;
				break;
			}
			case 'l':
			case 'L':
			{
				offset_x += 0.05;
				break;
			}
			case 'j':
			case 'J':
			{
				offset_x -= 0.05;
				break;
			}
			case 'i':
			case 'I':
			{
				offset_y -= 0.05;
				break;
			}
			case 'k':
			case 'K':
			{
				offset_y += 0.05;
				break;
			}
			case 'm':
			case 'M':
			{
				scale = 0.95;
				break;
			}
			case 'r':
			case 'R':
			{
				scale = 0.0;
				break;
			}
			default:
				(void)0;
		}
		if (scale > 0.95)
			scale = 0.95;
		else if (scale < 0.0)
			scale = 0.0;

		if (offset_x > scale / 2)
			offset_x = scale / 2;
		else if (offset_x < -(scale / 2))
			offset_x = -(scale / 2);

		if (offset_y > scale / 2)
			offset_y = scale / 2;
		else if (offset_y < -(scale / 2))
			offset_y = -(scale / 2);

		if (isalpha(key))
		{
			std::cout << "scale: " << scale << ", offset_x: " << offset_x << std::endl;

			app.SetScalerCrop(scale / 2 + offset_x, scale / 2 + offset_y, 1 - scale, 1 - scale);
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
		bool timeout = !options->frames && options->timeout &&
					   ((now - start_time) > options->timeout.value);
		bool frameout = options->frames && count >= options->frames;
		if (timeout || frameout || key == 'x' || key == 'X')
		{
			if (timeout)
				LOG(1, "Halting: reached timeout of " << options->timeout.get<std::chrono::milliseconds>()
													  << " milliseconds.");
			app.StopCamera(); // stop complains if encoder very slow to close
			app.StopEncoder();
			return;
		}

		CompletedRequestPtr &completed_request = std::get<CompletedRequestPtr>(msg.payload);
		app.EncodeBuffer(completed_request, app.VideoStream());
		app.ShowPreview(completed_request, app.VideoStream());
	}
}

int main(int argc, char *argv[])
{
	try
	{
		RPiCamEncoder app;
		VideoOptions *options = app.GetOptions();
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
