/*
 * Android File Transfer for Linux: MTP client for android devices
 * Copyright (C) 2015  Vladimir Menshakov

 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */
#include <stdio.h>
#include <stdexcept>

#include <usb/Context.h>

#include <cli/CommandLine.h>
#include <cli/Session.h>

int main(int argc, char **argv)
{
	using namespace mtp;

	DevicePtr mtp(Device::Find());
	if (!mtp)
	{
		printf("no mtp device found\n");
		return 1;
	}

	try
	{
		cli::Session session(mtp);

		if (argc >= 2)
		{
			cli::Tokens tokens;
			for(int i = 1; i < argc; ++i)
			{
				tokens.push_back(argv[i]);
			}
			session.ProcessCommand(std::move(tokens));
		}
		else
			if (session.IsInteractive())
				session.InteractiveInput();

		return 0;
	}
	catch (const std::exception &ex)
	{ fprintf(stderr, "error: %s\n", ex.what()); return 1; }
}
