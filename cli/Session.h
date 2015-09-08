#ifndef AFT_CLI_SESSION_H
#define AFT_CLI_SESSION_H

#include <mtp/ptp/Device.h>
#include <mtp/ptp/Session.h>
#include <mtp/ptp/Messages.h>

#include <functional>
#include <map>

namespace cli
{
	class Path
	{
		std::string		_path;
	public:
		Path(const std::string &path): _path(path) { }
	};

	class LocalPath
	{
		std::string		_path;
	public:
		LocalPath(const std::string &path): _path(path) { }
	};

	class Session
	{
		mtp::DevicePtr				_device;
		mtp::SessionPtr				_session;
		mtp::msg::DeviceInfo		_gdi;
		mtp::u32					_cd;
		bool						_running;

		struct ICommand { virtual ~ICommand() { } };
		DECLARE_PTR(ICommand);

		template<typename Func>
		struct Command : public ICommand
		{
			Func _func;
			Command(Func && func) : _func(func) { }
		};

		std::map<std::string, ICommandPtr> _commands;

		char ** CompletionCallback(const char *text, int start, int end);

	public:
		Session(const mtp::DevicePtr &device);

		void InteractiveInput();

		mtp::u32 Resolve(const Path &path);

		void Quit() { _running = false; }

		void ListCurrent();
		void List(mtp::u32 parent);
		void List(const Path &path) { return List(Resolve(path)); }

		void ListStorages();

		template <typename ...Args>
		void AddCommand(const std::string &name, std::function<void(Args...)> && callback)
		{
			typedef std::function<void(Args...)> FuncType;
			_commands.insert(std::make_pair(name, ICommandPtr(new Command<FuncType>(std::move(callback)))));
		}
	};
}

#endif

