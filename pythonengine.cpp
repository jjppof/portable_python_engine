#include "engine.h"
#include <windows.h>
#include "Shlwapi.h"
#include <cstdlib>

PythonEngine::~PythonEngine() {
	Py_Finalize();
}

PythonEngine & PythonEngine::getInstance() {
	static PythonEngine instance;
	return instance;
}

int PythonEngine::Initialize() {
	if (PathFileExistsA("C:\\Python36\\python.exe"))
		Py_SetPythonHome(L"C:\\Python36");
	else if (PathFileExistsA("C:\\Python 3.6\\python.exe"))
		Py_SetPythonHome(L"C:\\Python 3.6");
	else if (PathFileExistsA(std::getenv("PYTHONHOME")))
		Py_SetPythonHome(_wgetenv(L"PYTHONHOME"));
	else if (PathFileExistsA("C:\\Python3\\python.exe"))
		Py_SetPythonHome(L"C:\\Python3");
	else if (PathFileExistsA("C:\\Python\\python.exe"))
		Py_SetPythonHome(L"C:\\Python");
	else
		return PythonHomeNotFound;

	Py_SetProgramName(L"python_trimming");
	Py_Initialize();
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

int PythonEngine::LoadModule(std::string&& module_name) {
	PyObject* p_module = PyImport_ImportModule(module_name.c_str());
	if (p_module == nullptr)
		return PythonLoadExternalModuleError;
	std::map<const std::string, PyObject*> map;
	Module m = { p_module, map };
	p_modules.emplace(std::move(module_name), m);
	return PythonSuccess;
}

int PythonEngine::LoadFunction(std::string&& function_name, const std::string& module_name) {
	PyObject* p_func = PyObject_GetAttrString(p_modules[module_name].module_handle, function_name.c_str());
	if (p_func == nullptr)
		return PythonLoadFunctionError;
	p_modules[module_name].p_functions.emplace(std::move(function_name), p_func);
	return PythonSuccess;
}

int PythonEngine::AppendModule(const std::string& module_name, PyObject* (*init_func)(void)) {
	int initResult = PyImport_AppendInittab(module_name.c_str(), init_func);
	if (initResult == -1)
		return PythonAppendModuleError;
	return PythonSuccess;
}
