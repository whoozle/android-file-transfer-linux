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

#ifndef AFTL_MTPZ_TRUSTEDAPP_H
#define AFTL_MTPZ_TRUSTEDAPP_H

#include <mtp/types.h>
#include <string>

namespace mtp
{
	class Session;
	DECLARE_PTR(Session);

	class TrustedApp;
	DECLARE_PTR(TrustedApp);

	class TrustedApp
	{
	private:
		struct Keys;
		DECLARE_PTR(Keys);

	private:
		SessionPtr	_session;
		KeysPtr		_keys;

	public:
		static bool Supported();
		static bool Probe(const SessionPtr & session);

		~TrustedApp();

		///shortcut for creating trusted app straight from ctor
		static TrustedAppPtr Create(const SessionPtr & session, const std::string &mtpzDataPath)
		{ return Probe(session)? TrustedAppPtr(new TrustedApp(session, mtpzDataPath)): nullptr; }
		void Authenticate();

		bool KeysLoaded() const
		{ return !!_keys; }

	private:
		TrustedApp(const SessionPtr & session, const std::string & mtpzDataPath);
		static KeysPtr LoadKeys(const std::string & path);

	};
}

#endif
