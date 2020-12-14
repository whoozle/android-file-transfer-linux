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

#ifndef AFTL_CLI_SESSION_H
#define AFTL_CLI_SESSION_H

#include <mtp/ptp/Device.h>
#include <mtp/ptp/Session.h>
#include <mtp/ptp/Messages.h>

#include <cli/Command.h>

#include <atomic>
#include <functional>
#include <map>
#include <set>

namespace mtp
{
	class TrustedApp;
	DECLARE_PTR(TrustedApp);
	class Library;
	DECLARE_PTR(Library);
}

namespace cli
{
	class Session
	{
		mtp::DevicePtr				_device;
		mtp::SessionPtr				_session;
		mtp::TrustedAppPtr			_trustedApp;
		mtp::LibraryPtr				_library;
		mtp::msg::DeviceInfo		_gdi;
		mtp::StorageId				_cs; //current storage
		std::string					_csName; //current storage name
		mtp::ObjectId				_cd; //current directory
		std::atomic<bool>			_running;
		bool						_interactive;
		bool						_showEvents;
		bool						_showPrompt;
		std::string					_prompt;
		unsigned					_terminalWidth;
		bool						_batterySupported;
		bool						_deviceFriendlyNameSupported;

		std::multimap<std::string, ICommandPtr> _commands;

		char ** CompletionCallback(const char *text, int start, int end);

		mtp::ObjectId ResolvePath(const std::string &path, std::string &file);
		mtp::ObjectId ResolveObjectChild(mtp::ObjectId parent, const std::string &entity);

		static std::string GetFilename(const std::string &path);
		static std::string GetDirname(const std::string &path);
		static std::string FormatTime(const std::string &timespec);

		void GetObjectPropertyList(mtp::ObjectId parent, const std::set<mtp::ObjectId> &originalObjectList, const mtp::ObjectProperty property);

		mtp::StorageId GetUploadStorageId()
		{ return _cs == mtp::Session::AllStorages? mtp::Session::AnyStorage: _cs; }

		static std::string GetMtpzDataPath();

	public:
		Session(const mtp::SessionPtr &session, bool showPrompt);
		~Session();

		bool SetFirstStorage();

		void UpdatePrompt();
		bool IsInteractive() const
		{ return _interactive; }
		void ShowEvents(bool show)
		{ _showEvents = show; }
		void InteractiveInput();
		void ProcessCommand(const std::string &input);
		void ProcessCommand(Tokens &&tokens);

		mtp::ObjectId Resolve(const Path &path, bool create = false);

		void Help();
		void Quit();

		void CompletePath(const Path &path, CompletionResult &result);
		void CompleteStoragePath(const StoragePath &path, CompletionResult &result);

		mtp::StorageId GetStorageByPath(const StoragePath &path, mtp::msg::StorageInfo &si, bool allowAll);
		void DisplayDeviceInfo();
		void List(mtp::ObjectId parent, bool extended, bool recursive, const std::string &prefix = std::string());
		void ListStorages();
		void ChangeStorage(const StoragePath &path);
		void DisplayStorageInfo(const StoragePath &path);
		void Get(const LocalPath &dst, mtp::ObjectId srcId, bool thumb = false);
		void Get(const mtp::ObjectId srcId);
		void GetThumb(const mtp::ObjectId srcId);
		void GetThumb(const LocalPath &dst, mtp::ObjectId srcId)
		{ Get(dst, srcId, true); }
		void Cat(const Path &path);
		void Rename(const Path & path, const std::string & newName);
		void Put(mtp::ObjectId parentId, const LocalPath &src, const std::string &targetFilename = std::string());
		void Put(const LocalPath &src, const Path &dst);
		mtp::ObjectId MakeDirectory(mtp::ObjectId parentId, const std::string & name);
		void ListObjects(const std::string & format);
		void ListObjects(mtp::ObjectFormat format);
		void ListProperties(mtp::ObjectId id);
		void ListDeviceProperties();
		void TestObjectPropertyList(const Path &path);
		void Cancel();

		void ChangeDirectory(const Path &path)
		{ _cd = Resolve(path); }
		void CurrentDirectory();

		void List(bool extended, bool recursive)
		{ return List(_cd, extended, recursive); }

		void List(const Path &path, bool extended, bool recursive)
		{ return List(Resolve(path), extended, recursive); }

		void Put(const LocalPath &src)
		{ Put(_cd, src); }

		void Get(const Path &src)
		{ Get(Resolve(src)); }

		void Get(const LocalPath &dst, const Path &src)
		{ Get(dst, Resolve(src)); }

		void GetThumb(const Path &src)
		{ GetThumb(Resolve(src)); }

		void GetThumb(const LocalPath &dst, const Path &src)
		{ GetThumb(dst, Resolve(src)); }

		void MakeDirectory(const Path &path)
		{
			std::string name;
			mtp::ObjectId parent = ResolvePath(path, name);
			MakeDirectory(parent, name);
		}

		void MakePath(const Path &path)
		{ Resolve(path, true); }
		void MakeArtist(const std::string & name);

		void Delete(const Path &path)
		{ Delete(Resolve(path)); }

		void Delete(mtp::ObjectId id)
		{ _session->DeleteObject(id); }

		void ListProperties(const Path &path)
		{ ListProperties(Resolve(path)); }
		void GetObjectReferences(const Path & src);

		static void ShowType(const LocalPath &src);

		void ZuneInit();
		void ZuneImport(const LocalPath & path);

		template <typename ...Args>
		void AddCommand(const std::string &name, const std::string &help, std::function<void(Args...)> && callback)
		{ _commands.insert(std::make_pair(name, ICommandPtr(new Command<Args...>(help, std::move(callback))))); }
	};
	DECLARE_PTR(Session);
}

#endif

