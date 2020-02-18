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
#include <mutex>
#include <tuple>

#ifdef MTPZ_ENABLED
#	include <openssl/bio.h>
#	include <openssl/bn.h>
#	include <openssl/crypto.h>
#	include <openssl/rsa.h>
#	include <openssl/rand.h>
#	include <openssl/sha.h>
#endif

namespace mtp
{
	std::once_flag crypto_init;

	TrustedApp::TrustedApp(const SessionPtr & session, const std::string &mtpzDataPath):
		_session(session), _keys(LoadKeys(mtpzDataPath))
	{}

	TrustedApp::~TrustedApp()
	{ }

#ifdef MTPZ_ENABLED

	struct TrustedApp::Keys
	{
		ByteArray skey; //session key
		BIGNUM *exp, *mod, *pkey;
		RSA * rsa;
		ByteArray certificate;

		Keys(): exp(), mod(), pkey(), rsa(RSA_new())
		{ }

		void Update()
		{
			if (RSA_set0_key(rsa, mod, exp, pkey))
			{
				debug("created RSA key");
				mod = exp = pkey = NULL;
			}
			else
				throw std::runtime_error("failed to create RSA key");
		}

		~Keys() {
			if (exp)
				BN_free(exp);
			if (mod)
				BN_free(mod);
			if (pkey)
				BN_free(pkey);
			if (rsa)
				RSA_free(rsa);
		}

		static ByteArray HKDF(const u8 * message, size_t messageSize, size_t keySize)
		{
			static constexpr size_t blockSize = SHA_DIGEST_LENGTH;
			size_t blocks = (keySize + blockSize - 1) / blockSize;
			ByteArray key(blocks * blockSize);
			ByteArray ctr(messageSize + 4);
			const auto ctrPtr = std::copy(message, message + messageSize, ctr.data());

			u8 * dst = key.data();
			for(size_t i = 0; i < blocks; ++i, dst += blockSize)
			{
				ctrPtr[0] = i >> 24;
				ctrPtr[1] = i >> 16;
				ctrPtr[2] = i >> 8;
				ctrPtr[3] = i;
				SHA1(ctr.data(), ctr.size(), dst);
			}

			return key;
		}

		std::tuple<ByteArray, ByteArray> GenerateCertificateMessage()
		{
			static const size_t messageSize = 156 + certificate.size();
			ByteArray challenge(16);
			RAND_bytes(challenge.data(), challenge.size());

			ByteArray message(messageSize);
			auto dst = message.data();
			*dst++ = 0x02;
			*dst++ = 0x01;
			*dst++ = 0x01;
			*dst++ = 0x00;
			*dst++ = 0x00;
			*dst++ = certificate.size() >> 8;
			*dst++ = certificate.size();
			dst = std::copy(certificate.begin(), certificate.end(), dst);

			*dst++ = challenge.size() >> 8;
			*dst++ = challenge.size();
			dst = std::copy(challenge.begin(), challenge.end(), dst);

			ByteArray hash(SHA_DIGEST_LENGTH);
			{
				ByteArray salt(SHA_DIGEST_LENGTH + 8);
				SHA1(message.data() + 2, dst - message.data() - 2, salt.data() + 8);
				SHA1(salt.data(), salt.size(), hash.data());
			}

			ByteArray key = HKDF(hash.data(), hash.size(), 107);
			//HexDump("key", key);

			ByteArray signature(RSA_size(rsa));
			signature[106] = 1;
			for(size_t i = 0; i < hash.size(); ++i)
				signature[i + 107] = hash[i];
			for(size_t i = 0; i < 107; ++i)
				signature[i] ^= key[i];

			signature[0] &= 127;
			signature[127] = 188;
			//HexDump("signature", signature);

			*dst++ = 1;
			*dst++ = 0;
			*dst++ = signature.size();

			if (RSA_private_decrypt(signature.size(), signature.data(), dst, rsa, RSA_NO_PADDING) == -1)
				throw std::runtime_error("RSA_private_encrypt failed");

			return std::make_tuple(challenge, message);
		}

		static u8 FromHex(char ch)
		{
			if (ch >= '0' && ch <= '9')
				return ch - '0';
			if (ch >= 'a' && ch <= 'f')
				return ch - 'a' + 10;
			if (ch >= 'A' && ch <= 'F')
				return ch - 'A' + 10;
			throw std::runtime_error(std::string("invalid hex character ") + ch);
		}

		static ByteArray FromHex(const char *buf, size_t bufsize)
		{
			ByteArray data;
			data.reserve((bufsize + 1) / 2);
			while(buf[0] && buf[1]) {
				u8 h = FromHex(*buf++);
				u8 l = FromHex(*buf++);
				data.push_back((h << 4) | l);
			}
			if (buf[0] != 0 && !isspace(buf[0]))
				throw std::runtime_error("tailing character");

			return data;
		}
	};

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

	void TrustedApp::Authenticate()
	{
		if (!_keys)
			return;

		auto & di = _session->GetDeviceInfo();
		if (di.Supports(DeviceProperty::SessionInitiatorVersionInfo))
			_session->SetDeviceProperty(DeviceProperty::SessionInitiatorVersionInfo, "Android File Transfer for Linux/MTPZClass Driver");

		_session->GenericOperation(OperationCode::EndTrustedAppSession);
		ByteArray challenge, message;
		std::tie(challenge, message) = _keys->GenerateCertificateMessage();
		//HexDump("certificate payload", message);
		_session->GenericOperation(OperationCode::SendWMDRMPDAppRequest, message);
	}

	TrustedApp::KeysPtr TrustedApp::LoadKeys(const std::string & path)
	{
		BIO * bio = BIO_new_file(path.c_str(), "rt");
		if (bio == NULL) {
			error("could not open .mtpz-data");
			return nullptr;
		}

		auto keys = std::make_shared<Keys>();

		try
		{
			char buf[4096];
			//public exp
			BIO_gets(bio, buf, sizeof(buf));
			if (BN_hex2bn(&keys->exp, buf) <= 0)
				throw std::runtime_error("can't read public exponent");

			//session key
			auto r = BIO_gets(bio, buf, sizeof(buf));
			if (r <= 0)
				throw std::runtime_error("BIO_gets: short read");
			keys->skey = Keys::FromHex(buf, r);
			//HexDump("session key", keys->skey);

			//public mod
			BIO_gets(bio, buf, sizeof(buf));
			if (BN_hex2bn(&keys->mod, buf) <= 0)
				throw std::runtime_error("can't read public modulus");

			//private exponent
			BIO_gets(bio, buf, sizeof(buf));
			if (BN_hex2bn(&keys->pkey, buf) <= 0)
				throw std::runtime_error("can't read private key");

			r = BIO_gets(bio, buf, sizeof(buf));
			if (r <= 0)
				throw std::runtime_error("BIO_gets: short read");
			keys->certificate = Keys::FromHex(buf, r);
			//HexDump("certificate", keys->certificate);
		}
		catch(...)
		{ BIO_free(bio); throw; }

		BIO_free(bio);
		keys->Update();

		return keys;
	}

#else

	bool TrustedApp::Probe(const SessionPtr & session)
	{ return false; }

	TrustedApp::KeysPtr TrustedApp::LoadKeys(const std::string & path)
	{ return nullptr; }

	void TrustedApp::Authenticate()
	{ }

#endif
}
