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
		PythonHomeNotFound = 1,
		PythonInitializeError,
		PythonVersionError,
		PythonLoadExternalModuleError,
		PythonLoadFunctionError,
		PythonAppendModuleError
	};
	static constexpr int PythonSuccess = 0;
	int Initialize();
	int LoadModule(std::string&& module_name);
	int LoadFunction(std::string&& function_name, const std::string& module_name);
	template<typename ... T>
	PyObject* CallFunction(const std::string& function_name, const std::string& module_name, const T&... args) {
		std::vector<Any> vec = { args... };
		PyObject* p_args = PyTuple_New(vec.size());
		for (int i = 0; i < vec.size(); ++i)
			PyTuple_SetItem(p_args, i, vec[i].p_data);
		PyObject* p_func = p_modules[module_name].p_functions[function_name];
		return PyObject_CallObject(p_func, p_args);
	};
	int AppendModule(const std::string& module_name, PyObject* (*initfunc)(void));

	template<typename T>
	static T PyType_AsType(PyObject* py_data) {
		return T();
	};
	template<>
	static double PyType_AsType<double>(PyObject* py_data) {
		return PyFloat_AsDouble(py_data);
	};
	template<>
	static bool PyType_AsType<bool>(PyObject* py_data) {
		return static_cast<bool>(PyObject_IsTrue(py_data));
	};
	template<>
	static int PyType_AsType<int>(PyObject* py_data) {
		return static_cast<int>(PyLong_AsLong(py_data));
	};
	template<>
	static char* PyType_AsType<char*>(PyObject* py_data) {
		return PyUnicode_AsUTF8(py_data);
	};
	template<>
	static std::string PyType_AsType<std::string>(PyObject* py_data) {
		return PyUnicode_AsUTF8(py_data);
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

private:
	PythonEngine() {};
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

	static PyObject* PyType_FromType(const double data) {
		return PyFloat_FromDouble(data);
	};
	static PyObject* PyType_FromType(const bool data) {
		return PyBool_FromLong(static_cast<long>(data));
	};
	static PyObject* PyType_FromType(const int data) {
		return PyLong_FromLong(static_cast<long>(data));
	};
	static PyObject* PyType_FromType(const char* data) {
		return (PyObject*)(PyUnicode_FromString(data));
	};
	static PyObject* PyType_FromType(const std::string& data) {
		return (PyObject*)(PyUnicode_FromString(data.c_str()));
	};
	template<typename T>
	static PyObject* PyType_FromType(std::vector<T> data) {
		PyObject* p_list = PyList_New(data.size());
		for (int i = 0; i < data.size(); ++i)
			PyList_SetItem(p_list, i, PyType_FromType(data[i]));
		return p_list;
	}

	struct Module {
		PyObject* module_handle;
		std::map<const std::string, PyObject*> p_functions;
	};
	std::map<const std::string, Module> p_modules;
};