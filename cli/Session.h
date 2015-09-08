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
	struct Path : public std::string		{ Path(const std::string &path): std::string(path) { } };
	struct LocalPath : public std::string	{ LocalPath(const std::string &path): std::string(path) { } };

	class Session
	{
		mtp::DevicePtr				_device;
		mtp::SessionPtr				_session;
		mtp::msg::DeviceInfo		_gdi;
		mtp::u32					_cd;
		bool						_running;

		std::map<std::string, ICommandPtr> _commands;

		char ** CompletionCallback(const char *text, int start, int end);

	public:
		Session(const mtp::DevicePtr &device);

		void InteractiveInput();

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

		void List()
		{ return List(_cd); }

		void List(const Path &path)
		{ return List(Resolve(path)); }

		void Put(const LocalPath &src)
		{ Put(_cd, src); }

		void Get(const LocalPath &dst, const Path &src)
		{ Get(dst, Resolve(src)); }

		void MakeDirectory(const std::string &name)
		{ MakeDirectory(_cd, name); }

		void Delete(const Path &path)
		{ Delete(Resolve(path)); }

		void ListProperties(const Path &path)
		{ ListProperties(Resolve(path)); }

		template <typename ...Args>
		void AddCommand(const std::string &name, std::function<void(Args...)> && callback)
		{ _commands.insert(std::make_pair(name, ICommandPtr(new Command<Args...>(std::move(callback))))); }
	};
}

#endif

