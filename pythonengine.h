#pragma once

#include "Python.h"
#include <string>
#include <map>
#include <vector>
#include <type_traits>

class PythonEngine;

template<class T>
PyObject& operator+(PyObject& a, const T& b) noexcept {
	PyList_Append(&a, PythonEngine::PyType_FromType<T>(b));
	return a;
}

class PythonEngine {
public:
	~PythonEngine() noexcept;
	static PythonEngine& getInstance() noexcept;
	enum PyStatus : int {
		PythonSuccess = 0,
		PythonHomeNotFound,
		PythonInitializeError,
		PythonVersionError,
		PythonLoadExternalModuleError,
		PythonLoadFunctionError,
		PythonAlreadyInitialized
	};
	PyStatus Initialize(std::vector<std::pair<std::string, PyObject* (*)(void)>>& modules = std::vector<std::pair<std::string, PyObject* (*)(void)>>()) noexcept;
	PyStatus LoadModule(std::string&& module_name) noexcept;
	PyStatus LoadFunction(std::string&& function_name, const std::string& module_name) noexcept;
	template<typename ... T>
	PyObject* CallFunction(const std::string& function_name, const std::string& module_name, const T&... args) noexcept {
		PyObject* p_args = PyList_New(0);
		(*p_args + ... + args);
		PyObject* p_func = p_modules[module_name].p_functions[function_name];
		return PyObject_CallObject(p_func, PyList_AsTuple(p_args));
	};

	template<typename T>
	static T PyType_AsType(PyObject* py_data) noexcept {
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
	static std::vector<T> PyType_AsVector(PyObject* py_data) noexcept {
		std::vector<T> vec;
		const size_t size = PyList_Size(py_data);
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
	static PyObject* PyType_FromType(const T& data) noexcept {
		using T_ = std::remove_reference_t<std::remove_cv_t<std::remove_pointer_t<std::remove_all_extents_t<T>>>>;
		if constexpr (std::is_same_v<T_, double>)
			return PyFloat_FromDouble(data);
		else if constexpr (std::is_same_v<T_, bool>)
			return PyBool_FromLong(static_cast<long>(data));
		else if constexpr (std::is_same_v<T_, int>)
			return PyLong_FromLong(static_cast<long>(data));
		else if constexpr (std::is_same_v<T_, char>)
			return (PyObject*)(PyUnicode_FromString(data));
		else if constexpr (std::is_same_v<T_, wchar_t>)
			return (PyObject*)(PyUnicode_FromWideChar(data, wcslen(data)));
		else if constexpr (std::is_same_v<T_, std::string>)
			return (PyObject*)(PyUnicode_FromString(data.c_str()));
		return Py_None;
	}
	template<typename T>
	static PyObject* PyType_FromType(std::vector<T>&& data) noexcept {
		PyObject* p_list = PyList_New(data.size());
		for (int i = 0; i < data.size(); ++i)
			PyList_SetItem(p_list, i, PyType_FromType(data[i]));
		return p_list;
	}

private:
	PythonEngine() noexcept;
	PythonEngine(PythonEngine const&) = delete;
	void operator=(PythonEngine const&) = delete;
	PythonEngine(PythonEngine &&) = delete;
	void operator=(PythonEngine &&) = delete;

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
