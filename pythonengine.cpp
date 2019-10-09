#include "pythonengine.h"
#include <filesystem>

#define PYTHON_HOME "C:\\Python36\\"
#define WIDER2(x) L ## x
#define WIDER(x) WIDER2(x)
#define BIN_EXT ".exe"
#define PY_VERSION "3.6"

PythonEngine::PythonEngine() noexcept {
	Py_SetProgramName(L"python_engine");
}

PythonEngine::~PythonEngine() noexcept {
	if (Py_IsInitialized())
		Py_Finalize();
}

PythonEngine& PythonEngine::getInstance() noexcept {
	static PythonEngine instance;
	return instance;
}

PythonEngine::PyStatus PythonEngine::Initialize(std::vector<std::pair<std::string, PyObject* (*)(void)>>& modules) noexcept {
	if (Py_IsInitialized()) {
		for (decltype(auto) module : modules) {
			PyImport_AddModule(module.first.c_str());
			PyObject* pyModule = module.second();
			PyObject* sys_modules = PyImport_GetModuleDict();
			PyDict_SetItemString(sys_modules, module.first.c_str(), pyModule);
			Py_DECREF(pyModule);
		}
		return PyStatus::PythonAlreadyInitialized;
	}

	if (std::filesystem::exists(PYTHON_HOME "python" BIN_EXT))
		Py_SetPythonHome(WIDER(PYTHON_HOME));
	else
		return PyStatus::PythonHomeNotFound;

	for (decltype(auto) module : modules)
		PyImport_AppendInittab(module.first.c_str(), module.second);

	Py_Initialize();
	wchar_t* argv[1] = { L"" };
	PySys_SetArgv(1, argv);
	if (Py_IsInitialized() == 0)
		return PyStatus::PythonInitializeError;

	const char* pyVersion = Py_GetVersion();
	if (std::string(pyVersion).rfind(PY_VERSION, 0) != 0)
		return PyStatus::PythonVersionError;

	PyObject* path_list = PySys_GetObject("path");
	PyList_Append(path_list, PythonEngine::PyType_FromType(std::filesystem::current_path().c_str()));

	return PyStatus::PythonSuccess;
}

PythonEngine::PyStatus PythonEngine::LoadModule(const std::string& module_name) noexcept {
	PyObject* p_module = PyImport_ImportModule(module_name.c_str());
	if (p_module == nullptr)
		return PyStatus::PythonLoadExternalModuleError;
	std::map<const std::string, PyObject*> map;
	Module m = { p_module, map };
	p_modules.emplace(module_name, m);
	return PyStatus::PythonSuccess;
}

PythonEngine::PyStatus PythonEngine::LoadFunction(const std::string& function_name, const std::string& module_name) noexcept {
	PyObject* p_func = PyObject_GetAttrString(p_modules[module_name].module_handle, function_name.c_str());
	if (p_func == nullptr)
		return PyStatus::PythonLoadFunctionError;
	p_modules[module_name].p_functions.emplace(function_name, p_func);
	return PyStatus::PythonSuccess;
}
