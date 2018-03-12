/*
    This file is part of Android File Transfer For Linux.
    Copyright (C) 2015-2018  Vladimir Menshakov

    This library is free software; you can redistribute it and/or modify it
    under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation; either version 2.1 of the License,
    or (at your option) any later version.

    This library is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this library; if not, write to the Free Software Foundation,
    Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
*/

#include <stdio.h>
#include <stdexcept>

#include <usb/Context.h>

#include <cli/CommandLine.h>
#include <cli/Session.h>

#include <mtp/log.h>
#include <mtp/version.h>

#include <getopt.h>
#include <unistd.h>

int main(int argc, char **argv)
{
	using namespace mtp;
	bool forceInteractive = false;
	bool showHelp = false;
	bool showPrompt = true;
	bool showVersion = false;
	bool claimInterface = true;
	if (!isatty(STDIN_FILENO))
		showPrompt = false;

	static struct option long_options[] =
	{
		{"verbose",			no_argument,		0,	'v' },
		{"interactive",		no_argument,		0,	'i' },
		{"batch",			no_argument,		0,	'b' },
		{"help",			no_argument,		0,	'h' },
		{"version",			no_argument,		0,	'V' },
		{"no-claim",		no_argument,		0,	'C' },
		{0,					0,					0,	 0	}
	};

	while(true)
	{
		int optionIndex = 0; //index of matching option
		int c = getopt_long(argc, argv, "ibhvVC", long_options, &optionIndex);
		if (c == -1)
			break;
		switch(c)
		{
		case 'b':
			showPrompt = false; //no break here, prompt = false, interactive = true
		case 'i':
			forceInteractive = true;
			break;
		case 'v':
			g_debug = true;
			break;
		case 'V':
			showVersion = true;
			break;
		case 'C':
			claimInterface = false;
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
			"-i\tforce interactive mode\n"
			"-b\tbatch command processing\n"
			"-C\tno usb interface claim\n"
			"-V\tshow version information"
			);
		exit(0);
	}

	if (showVersion)
	{
		print(GetVersion());
		exit(0);
	}

	auto mtpDevices = Device::Find(claimInterface);
	if (mtpDevices.empty())
	{
		error("no mtp device found");
		exit(1);
	}
	auto mtp = mtpDevices.front();

	try
	{
		bool hasCommands = optind >= argc;
		cli::Session session(mtp, showPrompt);

		if (forceInteractive || (session.IsInteractive() && hasCommands))
		{
			session.InteractiveInput();
		}
		else
		{
			for(int i = optind; i < argc; ++i)
			{
				session.ProcessCommand(argv[i]);
			}
		}

		exit(0);
	}
	catch (const std::exception &ex)
	{ error("error: ", ex.what()); exit(1); }
}
