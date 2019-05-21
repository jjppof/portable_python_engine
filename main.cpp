#include "engine.h"
#include <windows.h>

int main() {
	static PythonEngine& handle = PythonEngine::getInstance();
	SetCurrentDirectoryA("C:\\Users\\JPFERNA\\Documents\\Visual Studio 2015\\Projects\\PythonEngine\\PythonEngine\\py_files");
	handle.Initialize();
	handle.LoadModule("my_module");
	handle.LoadFunction("my_function", "my_module");
	std::vector<std::string> test = { "a","b","c" };
	std::vector<std::vector<std::string>> test2 = { test, test, test };
	PyObject* p_return = handle.CallFunction("my_function", "my_module", test2, 3.14, true, "testando");
	auto obj = PythonEngine::PyType_AsVector<std::string>(p_return);
	return 0;
}