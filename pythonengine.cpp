#include "engine.h"
#include <filesystem>
#include <cstdlib>

PythonEngine::PythonEngine() {
	Py_SetProgramName(L"python_trimming");
}

PythonEngine::~PythonEngine() {
	Py_Finalize();
}

PythonEngine & PythonEngine::getInstance() {
	static PythonEngine instance;
	return instance;
}

PythonEngine::PyStatus PythonEngine::Initialize(std::vector<std::pair<std::string, PyObject* (*)(void)>>& modules) {
	if (Py_IsInitialized()) {
		for (auto& module : modules) {
			PyImport_AddModule(module.first.c_str());
			PyObject* pyModule = module.second();
			PyObject* sys_modules = PyImport_GetModuleDict();
			PyDict_SetItemString(sys_modules, module.first.c_str(), pyModule);
			Py_DECREF(pyModule);
		}
		return PythonAlreadyInitialized;
	}

	if (std::filesystem::exists("C:\\Python36\\python.exe"))
		Py_SetPythonHome(L"C:\\Python36");
	else if (std::filesystem::exists("C:\\Python 3.6\\python.exe"))
		Py_SetPythonHome(L"C:\\Python 3.6");
	else if (std::filesystem::exists(std::getenv("PYTHONHOME")))
		Py_SetPythonHome(_wgetenv(L"PYTHONHOME"));
	else if (std::filesystem::exists("C:\\Python3\\python.exe"))
		Py_SetPythonHome(L"C:\\Python3");
	else if (std::filesystem::exists("C:\\Python\\python.exe"))
		Py_SetPythonHome(L"C:\\Python");
	else
		return PythonHomeNotFound;

	for (auto& module : modules)
		PyImport_AppendInittab(module.first.c_str(), module.second);

	Py_Initialize();
	wchar_t* argv[1] = { L"" };
	PySys_SetArgv(1, argv);
	if (Py_IsInitialized() == 0)
		return PythonInitializeError;

	const char* pyVersion = Py_GetVersion();
	if (std::string(pyVersion).rfind("3.6", 0) != 0)
		return PythonVersionError;

	PyRun_SimpleString(
		"import os, sys \n"
		"sys.path.append(os.getcwd()) \n"
	);

	return PythonSuccess;
}

PythonEngine::PyStatus PythonEngine::LoadModule(std::string&& module_name) {
	PyObject* p_module = PyImport_ImportModule(module_name.c_str());
	if (p_module == nullptr)
		return PythonLoadExternalModuleError;
	std::map<const std::string, PyObject*> map;
	Module m = { p_module, map };
	p_modules.emplace(std::move(module_name), m);
	return PythonSuccess;
}

PythonEngine::PyStatus PythonEngine::LoadFunction(std::string&& function_name, const std::string& module_name) {
	PyObject* p_func = PyObject_GetAttrString(p_modules[module_name].module_handle, function_name.c_str());
	if (p_func == nullptr)
		return PythonLoadFunctionError;
	p_modules[module_name].p_functions.emplace(std::move(function_name), p_func);
	return PythonSuccess;
}
