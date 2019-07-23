#pragma once

#include "Python.h"
#include <string>
#include <map>
#include <vector>

class PythonEngine {
public:
	~PythonEngine();
	static PythonEngine& getInstance();
	enum Errors : int {
		PythonSuccess = 0,
		PythonHomeNotFound,
		PythonInitializeError,
		PythonVersionError,
		PythonLoadExternalModuleError,
		PythonLoadFunctionError,
		PythonAlreadyInitialized
	};
	PyStatus Initialize(std::vector<std::pair<std::string, PyObject* (*)(void)>>& modules = std::vector<std::pair<std::string, PyObject* (*)(void)>>());
	PyStatus LoadModule(std::string&& module_name);
	PyStatus LoadFunction(std::string&& function_name, const std::string& module_name);
	template<typename ... T>
	PyObject* CallFunction(const std::string& function_name, const std::string& module_name, const T&... args) {
		std::vector<Any> vec = { args... };
		PyObject* p_args = PyTuple_New(vec.size());
		for (int i = 0; i < vec.size(); ++i)
			PyTuple_SetItem(p_args, i, vec[i].p_data);
		PyObject* p_func = p_modules[module_name].p_functions[function_name];
		return PyObject_CallObject(p_func, p_args);
	};

	template<typename T>
	static T PyType_AsType(PyObject* py_data) {
		if constexpr (std::is_same_v<T, double>)
			return PyFloat_AsDouble(py_data);
		else if constexpr (std::is_same_v<T, bool>)
			return static_cast<bool>(PyObject_IsTrue(py_data));
		else if constexpr (std::is_same_v<T, int>)
			return static_cast<int>(PyLong_AsLong(py_data));
		else if constexpr (std::is_same_v<T, char*> || std::is_same_v<T, std::string>)
			return PyUnicode_AsUTF8(py_data);
		return T();
	};
	template<typename T>
	static std::vector<T> PyType_AsVector(PyObject* py_data) {
		std::vector<T> vec;
		size_t size = PyList_Size(py_data);
		vec.reserve(size);
		PyObject* item;
		for (int i = 0; i < size; ++i) {
			item = PyList_GetItem(py_data, i);
			if constexpr (is_std_vector<T>::value)
				vec.push_back(PyType_AsVector<typename T::value_type>(item));
			else
				vec.push_back(PyType_AsType<T>(item));
		}
		return vec;
	};

	template<typename T>
	static PyObject* PyType_FromType(T data) {
		using T_ = std::remove_reference_t<std::remove_cv_t<T>>;
		if constexpr (std::is_same_v<T_, double>)
			return PyFloat_FromDouble(data);
		else if constexpr (std::is_same_v<T_, bool>)
			return PyBool_FromLong(static_cast<long>(data));
		else if constexpr (std::is_same_v<T_, int>)
			return PyLong_FromLong(static_cast<long>(data));
		else if constexpr (std::is_same_v<T_, char*>)
			return (PyObject*)(PyUnicode_FromString(data));
		else if constexpr (std::is_same_v<T_, std::string>)
			return (PyObject*)(PyUnicode_FromString(data.c_str()));
		return Py_None;
	}
	template<typename T>
	static PyObject* PyType_FromType(std::vector<T> data) {
		PyObject* p_list = PyList_New(data.size());
		for (int i = 0; i < data.size(); ++i)
			PyList_SetItem(p_list, i, PyType_FromType(data[i]));
		return p_list;
	}

private:
	PythonEngine();
	PythonEngine(PythonEngine const&) = delete;
	void operator=(PythonEngine const&) = delete;
	PythonEngine(PythonEngine &&) = delete;
	void operator=(PythonEngine &&) = delete;

	struct Any {
		PyObject* p_data;
		template<typename T>
		Any(T&& data) {
			p_data = PyType_FromType(std::forward<T>(data));
		}
	};
	template<typename>
	struct is_std_vector : std::false_type {};
	template<typename T>
	struct is_std_vector<std::vector<T>> : std::true_type {};

	struct Module {
		PyObject* module_handle;
		std::map<const std::string, PyObject*> p_functions;
	};
	std::map<const std::string, Module> p_modules;
};
