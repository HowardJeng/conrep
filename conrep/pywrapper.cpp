// pywrapper.cpp
// implements PyWrapper class
#include "pywrapper.h"
#include "assert.h"

namespace console {
  namespace bpy = boost::python;

  PyWrapper::PyWrapper()
    : main_namespace_(bpy::object(
                        bpy::handle<>(
                          bpy::borrowed(
                            PyImport_AddModule("__main__")
                          )
                        )
                      ).attr("__dict__")) {}

  void PyWrapper::run_string(const char * str) {
    bpy::handle<>(
      PyRun_String(str,
                   Py_file_input,
                   main_namespace_.ptr(),
                   main_namespace_.ptr())
    );
  }

  bpy::object PyWrapper::import(const char * name) {
    ASSERT(std::string(name).find_first_of('.') == std::string::npos);
    PyObject * m = PyImport_ImportModuleEx(const_cast<char *>(name), // Python 2.4.4, 2.5.0, 2.5.1 & 2.5.2
                                           main_namespace_.ptr(),    //   don't seem to modify the
                                           main_namespace_.ptr(),    //   string so the const_cast 
                                           0);                       //   is safe. No guarantees for
                                                                     //   other versions.
    if (!m) bpy::throw_error_already_set();
    bpy::object module((bpy::handle<>(m)));
    main_namespace_[name] = module;
    return module.attr("__dict__");
  }

  PythonInitializer::PythonInitializer(const tstring & exe_directory) {
    Py_Initialize();
    char * argv = "conrep.exe\0";
    PySys_SetArgv(0, &argv); //prevent python from freaking out when sys.argv is accessed

    PyWrapper py;
    bpy::object sys = py.import("sys");
    sys["path"] = bpy::list();
    bpy::object path = sys["path"];
    tstring config_scripts = exe_directory + _T("\\config_scripts.dat");
    path.attr("insert")(0, config_scripts); 
    path.attr("insert")(0, exe_directory);
    // path now contains exe directory first and script package second

    py.import("cStringIO");
    py.run_string("sys.stderr = cStringIO.StringIO()");
  }

  PythonInitializer::~PythonInitializer() {
    Py_Finalize();
  }
}