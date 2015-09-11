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
#ifndef AFT_CLI_SESSION_H
#define AFT_CLI_SESSION_H

#include <mtp/ptp/Device.h>
#include <mtp/ptp/Session.h>
#include <mtp/ptp/Messages.h>

#include <cli/Command.h>

#include <functional>
#include <map>

namespace cli
{
	class Session
	{
		mtp::DevicePtr				_device;
		mtp::SessionPtr				_session;
		mtp::msg::DeviceInfo		_gdi;
		mtp::u32					_cd;
		bool						_running;

		std::multimap<std::string, ICommandPtr> _commands;

		char ** CompletionCallback(const char *text, int start, int end);

	public:
		Session(const mtp::DevicePtr &device);

		void InteractiveInput();
		void ProcessCommand(const std::string &input);
		void ProcessCommand(Tokens &&tokens);

		mtp::u32 Resolve(const Path &path);

		void Help();
		void Quit() { _running = false; }

		void List(mtp::u32 parent);

		void ListStorages();
		void Get(const LocalPath &dst, mtp::u32 srcId);
		void Get(const mtp::u32 srcId);
		void Put(mtp::u32 parentId, const LocalPath &src);
		void MakeDirectory(mtp::u32 parentId, const std::string & name);
		void Delete(mtp::u32 id);
		void ListProperties(mtp::u32 id);
		void ListDeviceProperties();

		void ChangeDirectory(const Path &path)
		{ _cd = Resolve(path); }

		void List()
		{ return List(_cd); }

		void List(const Path &path)
		{ return List(Resolve(path)); }

		void Put(const LocalPath &src)
		{ Put(_cd, src); }

		void Put(const LocalPath &src, const Path &dst)
		{ Put(Resolve(dst), src); } //fixme: add dir/file distinction here

		void Get(const Path &src)
		{ Get(Resolve(src)); }

		void Get(const LocalPath &dst, const Path &src)
		{ Get(dst, Resolve(src)); }

		void MakeDirectory(const std::string &name)
		{ MakeDirectory(_cd, name); }

		void Delete(const Path &path)
		{ Delete(Resolve(path)); }

		void ListProperties(const Path &path)
		{ ListProperties(Resolve(path)); }

		template <typename ...Args>
		void AddCommand(const std::string &name, const std::string &help, std::function<void(Args...)> && callback)
		{ _commands.insert(std::make_pair(name, ICommandPtr(new Command<Args...>(help, std::move(callback))))); }
	};
}

#endif

