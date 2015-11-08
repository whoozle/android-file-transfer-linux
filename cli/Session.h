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
		mtp::u32					_cs; //current storage
		mtp::u32					_cd; //current directory
		bool						_running;
		bool						_interactive;
		bool						_showPrompt;
		unsigned					_terminalWidth;

		std::multimap<std::string, ICommandPtr> _commands;

		char ** CompletionCallback(const char *text, int start, int end);

		mtp::u32 ResolvePath(const std::string &path, std::string &file);
		mtp::u32 ResolveObjectChild(mtp::u32 parent, const std::string &entity);
		static std::string GetFilename(const std::string &path);
		static std::string GetDirname(const std::string &path);
		static std::string FormatTime(const std::string &timespec);

	public:
		Session(const mtp::DevicePtr &device, bool showPrompt);

		bool IsInteractive() const
		{ return _interactive; }
		void InteractiveInput();
		void ProcessCommand(const std::string &input);
		void ProcessCommand(Tokens &&tokens);

		mtp::u32 Resolve(const Path &path);

		void Help();
		void Quit() { _running = false; }

		void CompletePath(const Path &path, CompletionResult &result);

		void List(mtp::u32 parent, bool extended);

		void ListStorages();
		void Get(const LocalPath &dst, mtp::u32 srcId);
		void Get(const mtp::u32 srcId);
		void Cat(const Path &path);
		void Put(mtp::u32 parentId, const std::string &dst, const LocalPath &src);
		void MakeDirectory(mtp::u32 parentId, const std::string & name);
		void ListProperties(mtp::u32 id);
		void ListDeviceProperties();

		void ChangeDirectory(const Path &path)
		{ _cd = Resolve(path); }
		void CurrentDirectory();

		void List(bool extended)
		{ return List(_cd, extended); }

		void List(const Path &path, bool extended)
		{ return List(Resolve(path), extended); }

		void Put(const LocalPath &src)
		{ Put(_cd, GetFilename(src), src); }

		void Put(const LocalPath &src, const Path &dst)
		{
			std::string filename;
			mtp::u32 parent = ResolvePath(dst, filename);
			Put(parent, filename, src);
		}

		void Get(const Path &src)
		{ Get(Resolve(src)); }

		void Get(const LocalPath &dst, const Path &src)
		{ Get(dst, Resolve(src)); }

		void MakeDirectory(const std::string &path)
		{
			std::string name;
			mtp::u32 parent = ResolvePath(path, name);
			MakeDirectory(parent, name);
		}

		void Delete(const Path &path)
		{ _session->DeleteObject(Resolve(path)); }

		void ListProperties(const Path &path)
		{ ListProperties(Resolve(path)); }

		static void ShowType(const LocalPath &src);

		template <typename ...Args>
		void AddCommand(const std::string &name, const std::string &help, std::function<void(Args...)> && callback)
		{ _commands.insert(std::make_pair(name, ICommandPtr(new Command<Args...>(help, std::move(callback))))); }
	};
}

#endif

