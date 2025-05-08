#ifndef UTILS_HPP
#define UTILS_HPP

#include <string>

template <typename F>
struct Defer
{
	F m_f;

	Defer(F f) : m_f(f) {}
	~Defer() { m_f(); }
};

template<typename F>
inline static Defer<F>
make_defer(F f)
{
	return Defer<F>(f);
}

#define DEFER_1(x, y) x##y
#define DEFER_2(x, y) DEFER_1(x, y)
#define DEFER_3(x)    DEFER_2(x, __COUNTER__)

#define defer Defer DEFER_3(_defer_) = [&]()

namespace utilities
{
	std::string
	win32_get_error_string(void);

	int
	win32_load_module_functions(void);

	int
	win32_set_shell_to_current_directory(void);
}

#endif