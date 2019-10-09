#include "pythonengine.h"
#include <utility>

int main() {
	decltype(auto) handle = PythonEngine::getInstance();
	handle.Initialize();
	handle.LoadModule("example");
	handle.LoadFunction("my_function", "example");
	std::vector<int> test = { 1,2,3 };
	std::vector<std::vector<int>> test2 = { test, test, test };
	PyObject* p_return = handle.CallFunction("my_function", "example", test2, 3.14, true, "testing");
	auto obj = std::move(PythonEngine::PyType_AsVector<std::string>(p_return));
	return 0;
}
