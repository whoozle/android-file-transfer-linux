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

#ifndef AFTL_CLI_COMMAND_H
#define AFTL_CLI_COMMAND_H

#include <cli/Tokens.h>
#include <cli/MakeTuple.h>
#include <mtp/types.h>
#include <mtp/function_invoker.h>
#include <mtp/Demangle.h>

#include <functional>
#include <string>
#include <list>

namespace cli
{
	class Session;

	struct Path : public std::string		{ Path(const std::string &path = std::string()): std::string(path) { } };
	struct LocalPath : public std::string	{ LocalPath(const std::string &path = std::string()): std::string(path) { } };
	struct StoragePath : public std::string	{ StoragePath(const std::string &path = std::string()): std::string(path) { } };

	inline std::string EscapePath(std::string name)
	{
		if (name.find(' ') != name.npos)
			return '"' + name +'"';
		else
			return name;
	}

	typedef std::list<std::string> CompletionResult;
	struct CompletionContext
	{
		cli::Session &				Session;
		size_t						Index;
		std::string					Prefix;
		CompletionResult &			Result;

		CompletionContext(cli::Session &s, size_t i, const std::string &p, std::list<std::string> & r):
			Session(s), Index(i), Prefix(p), Result(r) { }
	};

	namespace impl
	{
		template<>
		struct ValueConverter<LocalPath>
		{
			static LocalPath Convert(const Path &text)
			{ return text; }
		};

		template<>
		struct ValueConverter<Path>
		{
			static Path Convert(const Path &text)
			{ return text; }
		};

		template<>
		struct ValueConverter<StoragePath>
		{
			static StoragePath Convert(const Path &text)
			{ return text; }
		};

		template<typename Type>
		struct Completer
		{
			static void Complete(CompletionContext & ctx) { }
		};

		template<>
		struct Completer<Path>
		{
			static void Complete(CompletionContext & ctx);
		};

		template<>
		struct Completer<StoragePath>
		{
			static void Complete(CompletionContext & ctx);
		};

		template<typename ... Tail>
		struct CompletionForwarder
		{
			static void Complete(CompletionContext & ctx, size_t index)
			{ }
		};

		template<typename First, typename ... Tail>
		struct CompletionForwarder<First, Tail...>
		{
			static void Complete(CompletionContext & ctx, size_t index)
			{
				if (index == 0)
					Completer<typename std::decay<First>::type>::Complete(ctx);
				else
					CompletionForwarder<Tail...>::Complete(ctx, index - 1);
			}
		};
	};

	struct ICommand
	{
		virtual ~ICommand() = default;

		virtual void Execute(const Tokens &tokens) const = 0;
		virtual size_t GetArgumentCount() const = 0;
		virtual std::string GetHelpString() const = 0;
		virtual void Complete(CompletionContext &ctx) const = 0;
	};
	DECLARE_PTR(ICommand);

	class BaseCommand : public virtual ICommand
	{
		std::string _help;

	public:
		BaseCommand(const std::string &help): _help(help) { }
		virtual std::string GetHelpString() const
		{ return _help; }
	};

	template<typename ... Args>
	struct Command : public BaseCommand
	{
		typedef std::function<void (Args...)> FuncType;

		FuncType		_func;

		Command(const std::string &help, FuncType && func) : BaseCommand(help), _func(func) { }

		template<typename ...FuncArgs>
		static void Execute(std::function<void (FuncArgs...)> func, const Tokens & tokens)
		{
			auto args = cli::MakeTuple<Tokens::const_iterator, FuncArgs...>(tokens.begin(), tokens.end());
			mtp::invoke(func, args);
		}

		virtual void Execute(const Tokens &tokens) const
		{
			Execute(_func, tokens);
		}

		virtual size_t GetArgumentCount() const
		{ return sizeof...(Args); }

		virtual void Complete(CompletionContext &ctx) const
		{ impl::CompletionForwarder<Args...>::Complete(ctx, ctx.Index); }
	};


}

#endif

