/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (C) 2020, Raspberry Pi (Trading) Ltd.
 *
 * still_options.hpp - still capture program options
 */

#pragma once

#include <cstdio>

#include "options.hpp"

struct HelloOptions : public Options
{
	HelloOptions() : Options()
	{
		using namespace boost::program_options;
		// clang-format off
		options_.add_options()
			("keypress,k", value<bool>(&keypress)->default_value(false)->implicit_value(true),
			 "Perform capture when ENTER pressed")
			("signal,s", value<bool>(&signal)->default_value(false)->implicit_value(true),
			 "Perform capture when signal received")
			;
		// clang-format on
	}

	bool keypress;
	bool signal;

	virtual bool Parse(int argc, char *argv[]) override
	{
		if (Options::Parse(argc, argv) == false)
			return false;
		return true;
	}
	virtual void Print() const override
	{
		Options::Print();
		std::cerr << "    keypress: " << keypress << std::endl;
		std::cerr << "    signal: " << signal << std::endl;
	}
};
