/*
    This file is part of Android File Transfer For Linux.
    Copyright (C) 2015-2020  Vladimir Menshakov

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

#include <atomic>

#include <fcntl.h>
#include <fcntl.h>
#include <getopt.h>
#include <unistd.h>
#include <signal.h>

#include <sys/types.h>
#include <sys/stat.h>

namespace
{
	std::atomic<cli::Session *>		gSession(nullptr);

	static void SigIntHandler(int)
	{
		cli::Session * session = gSession.load();
		if (!session)
			exit(0);

		try
		{
			session->Cancel();
		}
		catch(const std::exception & ex)
		{
			mtp::debug("cancellation failed: ", ex.what());
			session->Quit();
			exit(0); //fixme: use readline alternate interface (manually feeding data from poll loop)
		}
	}
}

int main(int argc, char **argv)
{
	using namespace mtp;
	bool forceInteractive = false;
	bool showHelp = false;
	bool showPrompt = true;
	bool showVersion = false;
	bool claimInterface = true;
	bool showEvents = false;
	bool resetDevice = false;
	bool listDevices = false;
	std::string deviceFilter;
	const char *fileInput = nullptr;

	if (!isatty(STDIN_FILENO))
		showPrompt = false;

	static struct option long_options[] =
	{
		{"verbose",			no_argument,		0,	'v' },
		{"interactive",		no_argument,		0,	'i' },
		{"batch",			no_argument,		0,	'b' },
		{"events",			no_argument,		0,	'e' },
		{"help",			no_argument,		0,	'h' },
		{"version",			no_argument,		0,	'V' },
		{"no-claim",		no_argument,		0,	'C' },
		{"input-file",		required_argument,	0,	'f' },
		{"reset-device",	no_argument	,		0,	'R' },
		{"device-name",		required_argument,	0,	'd' },
		{"device-list",		no_argument,		0,	'l' },
		{0,					0,					0,	 0	}
	};

	struct sigaction newHandler = { };
	newHandler.sa_handler = &SigIntHandler;
	newHandler.sa_flags = SA_RESTART;
	if (sigaction(SIGINT, &newHandler, nullptr) != 0)
		perror("sigaction(SIGINT)");

	while(true)
	{
		int optionIndex = 0; //index of matching option
		int c = getopt_long(argc, argv, "ibehvVCRf:ld:", long_options, &optionIndex);
		if (c == -1)
			break;
		switch(c)
		{
		case 'f':
			fileInput = optarg; //falling back to batch processing here
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
		case 'e':
			showEvents = true;
			break;
		case 'R':
			resetDevice = true;
			break;
		case 'l':
			listDevices = true;
			break;
		case 'd':
			deviceFilter = optarg;
			break;
		case '?':
		case 'h':
		default:
			showHelp = true;
		}
	}
	if (fileInput)
	{
		close(STDIN_FILENO);
		int fd = open(fileInput, O_RDONLY);
		if (fd == -1)
		{
			perror("open");
			exit(1);
		}
		if (fd != STDIN_FILENO)
		{
			fprintf(stderr, "failed to reopen stdin\n");
			exit(1);
		}
	}

	if (showHelp)
	{
		error(
			"usage:\n"
			"-h\t--help\t\tshow this help\n"
			"-v\t--verbose\tshow debug output\n"
			"-i\t--interactive\tforce interactive mode\n"
			"-b\t--batch\t\tbatch command processing\n"
			"-e\t--events\tallow event processing\n"
			"-f\t--input-file\tuse file to read input commands\n"
			"-C\t--no-claim\tno usb interface claim\n"
			"-R\t--reset-device\treset usb device before connecting\n"
			"-d\t--device-name\tuse device name (could be partial name, e.g. model or manufacturer)\n"
			"-l\t--device-list\tlist devices\n"
			"-V\t--version\tshow version information"
			);
		exit(0);
	}

	if (showVersion)
	{
		print(GetVersion());
		exit(0);
	}

	usb::ContextPtr ctx(new usb::Context);
	if (listDevices) {
		for (usb::DeviceDescriptorPtr desc : ctx->GetDevices())
		{
			try
			{
				auto device = Device::Open(ctx, desc, claimInterface, resetDevice);
				if (device) {
					auto di = device->GetInfo();
					if (di.Matches(deviceFilter)) {
						print(di.GetFilesystemFriendlyName());
					}
				}
			}
			catch (const std::exception & ex)
			{ error("Device::Find failed:", ex.what()); }
		}
		return 0;
	}
	cli::SessionPtr session;

	try
	{
		auto device = Device::FindFirst(ctx, deviceFilter, claimInterface, resetDevice);
		if (!device)
		{
			error("device not found (filter = '", deviceFilter, "'");
			exit(1);
		}

		session = std::make_shared<cli::Session>(device->OpenSession(1), showPrompt);
		if (!session->SetFirstStorage())
		{
			error("your device may be locked or does not have any storage available");
			session.reset();
			device.reset();
		}
	}
	catch(const std::exception &ex)
	{ error("Device::Find failed:", ex.what()); }

	ctx.reset();
	if (!session)
		exit(1);

	try
	{
		bool hasCommands = optind >= argc;
		gSession.store(session.get());
		session->ShowEvents(showEvents);

		if (forceInteractive || (session->IsInteractive() && hasCommands))
		{
			session->InteractiveInput();
		}
		else
		{
			for(int i = optind; i < argc; ++i)
			{
				session->ProcessCommand(argv[i]);
			}
		}
		gSession.store(nullptr);
		session.reset();

		exit(0);
	}
	catch (const std::exception &ex)
	{ error("error: ", ex.what()); exit(1); }
}
