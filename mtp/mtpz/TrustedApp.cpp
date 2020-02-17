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

#include <mtp/mtpz/TrustedApp.h>
#include <mtp/ptp/Session.h>
#include <mtp/log.h>

namespace mtp
{
	TrustedApp::TrustedApp(const SessionPtr & session, const std::string &mtpzDataPath):
		_session(session), _keys(LoadKeys(mtpzDataPath))
	{ }

	TrustedApp::~TrustedApp()
	{ }

#ifdef MTPZ_ENABLED

	bool TrustedApp::Probe(const SessionPtr & session)
	{
		auto & di = session->GetDeviceInfo();
		bool supported =
			di.Supports(OperationCode::SendWMDRMPDAppRequest) &&
			di.Supports(OperationCode::GetWMDRMPDAppResponse) &&
			di.Supports(OperationCode::EnableTrustedFilesOperations) &&
			di.Supports(OperationCode::DisableTrustedFilesOperations) &&
			di.Supports(OperationCode::EndTrustedAppSession);

		debug("MTPZ supported: " , supported? "yes": "no");
		return supported;
	}

	bool TrustedApp::Authenticate()
	{
		return false;
	}

	TrustedApp::KeysPtr TrustedApp::LoadKeys(const std::string & path)
	{ return nullptr; }

#else

	bool TrustedApp::Probe(const SessionPtr & session)
	{ return false; }

	TrustedApp::KeysPtr TrustedApp::LoadKeys(const std::string & path)
	{ return nullptr; }

	bool TrustedApp::Authenticate()
	{ return false; }

#endif
}
