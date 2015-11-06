/*
    This file is part of Android File Transfer For Linux.
    Copyright (C) 2015  Vladimir Menshakov

    Android File Transfer For Linux is free software: you can redistribute
    it and/or modify it under the terms of the GNU General Public License
    as published by the Free Software Foundation, either version 3 of the
    License, or (at your option) any later version.

    Android File Transfer For Linux is distributed in the hope that it will
    be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
    of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Android File Transfer For Linux.
    If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdexcept>

#include <usb/Context.h>

#include <cli/CommandLine.h>
#include <cli/Session.h>

#include <mtp/log.h>

#include <getopt.h>

int main(int argc, char **argv)
{
	using namespace mtp;
	bool forceInteractive = false;
	bool showHelp = false;

	static struct option long_options[] =
	{
		{"verbose",			no_argument,		0,	'v' },
		{"interactive",		no_argument,		0,	'i' },
		{"help",			no_argument,		0,	'h' },
		{0,					0,					0,	 0	}
	};

	while(true)
	{
		int optionIndex = 0; //index of matching option
		int c = getopt_long(argc, argv, "ihv", long_options, &optionIndex);
		if (c == -1)
			break;
		switch(c)
		{
		case 'i':
			forceInteractive = true;
			break;
		case 'v':
			g_debug = true;
			break;
		case '?':
		case 'h':
		default:
			showHelp = true;
		}
	}

	if (showHelp)
	{
		error(
			"usage:\n"
			"-h\tshow this help\n"
			"-v\tshow debug output\n"
			"-i\tforce interactive mode"
			);
		exit(0);
	}

	DevicePtr mtp(Device::Find());
	if (!mtp)
	{
		error("no mtp device found");
		exit(1);
	}

	try
	{
		cli::Session session(mtp);

		if (forceInteractive || (session.IsInteractive() && optind >= argc))
		{
			session.InteractiveInput();
		}
		else
		{
			cli::Tokens tokens;
			for(int i = optind; i < argc; ++i)
			{
				tokens.push_back(argv[i]);
			}
			if (!tokens.empty())
				session.ProcessCommand(std::move(tokens));
		}

		exit(0);
	}
	catch (const std::exception &ex)
	{ error("error: ", ex.what()); exit(1); }
}
