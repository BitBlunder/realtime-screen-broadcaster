#include "../../include/utils/encryption.hpp"

#include "../../include/utils/log.hpp"
#include "../../include/utils/utils.hpp"
#include "../../include/utils/win32_symbols.hpp"

namespace encryption
{
	void
	EncryptedString::decrypt()
	{
		if (this->_plain)
			return;

		this->_plain = new CHAR[this->_length + 1];
		this->_plain[this->_length] = '\0';

		for (size_t i = 0; i < this->_length; i++)
			this->_plain[i] = this->_raw[i] ^ (this->_key + i);
	}

	HMODULE
	get_encrypted_module_name(EncryptedString& self)
	{
		self.decrypt();

		HMODULE h_module = fptr_LoadLibraryA(self._plain);
		if (h_module == NULL) {
			LOG_ERROR("Failed to load %s with %s", self._plain, utilities::win32_get_error_string().c_str());

			return nullptr;
		}

		return h_module;
	}
	FARPROC
	get_encrypted_function_name(EncryptedString& self, HMODULE h_module)
	{
		self.decrypt();

		FARPROC function_ptr = GetProcAddress(h_module, self._plain);
		if (function_ptr == NULL) {
			LOG_ERROR("Failed to get %s with %s", self._plain, utilities::win32_get_error_string().c_str());

			return nullptr;
		}

		return function_ptr;
	}
}
