#ifndef ENCRYPTION_HPP
#define ENCRYPTION_HPP

#include <array>
#include <utility>
#include <cstdlib>

#include <windows.h>


namespace encryption
{
	struct EncryptedString
	{
		CONST BYTE _key;
		CONST BYTE* _raw;

		PSTR _plain;
		UINT64 _length;

		EncryptedString()
			: _key(0x00), _raw(nullptr), _plain(nullptr), _length(0) {}
		EncryptedString(CONST BYTE* raw, UINT64 length, CONST BYTE key)
			: _key(key), _raw(raw), _plain(nullptr), _length(length) {}

		void
		decrypt();
	};

	template<CONST UINT64 N, CONST BYTE KEY, UINT64... I>
	constexpr std::array<BYTE, N-1>
	make_blob(CONST CHAR (&txt)[N], std::index_sequence<I...>)
	{
		return { static_cast<BYTE>(txt[I] ^ (KEY + I))... };
	}

	HMODULE
	get_encrypted_module_name(EncryptedString& self);
	FARPROC
	get_encrypted_function_name(EncryptedString& self, HMODULE h_module);
}

#define ENC_STRING(raw, key, literal)                                \
	inline constexpr auto _blob_##raw =                              \
	encryption::make_blob<sizeof(literal), key>(literal,             \
			std::make_index_sequence<sizeof(literal) - 1>{});        \
	inline encryption::EncryptedString raw{ _blob_##raw.data(), _blob_##raw.size(), key };

#endif