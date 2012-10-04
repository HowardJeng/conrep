// Python embedding goodies
#ifndef CONREP_PYWRAPPER_H
#define CONREP_PYWRAPPER_H

#pragma warning(push)
  #pragma warning(disable:4100)
  #pragma warning(disable:4121)
  #pragma warning(disable:4244)
  #pragma warning(disable:4267)
  #pragma warning(disable:4511)
  #pragma warning(disable:4512)
  #pragma warning(disable:4671)
  #pragma warning(disable:4673)
  #include <boost/python.hpp>
#pragma warning(pop)

#include "tchar.h"

namespace console {
  class PyWrapper {
    public:
      PyWrapper();
      
      void run_string(const char * str); // run a string in the Python interpreter
      
      // import a module. this function returns the module's dictionary, not
      //   the module itself.
      boost::python::object import(const char * name);

      typedef boost::python::api::const_object_item const_object_item;
      typedef boost::python::api::      object_item       object_item;
      
      template <typename T> 
      const_object_item operator[](const T & key) const { return main_namespace_[key]; } // provide indexers into
      template <typename T>       
            object_item operator[](const T & key)       { return main_namespace_[key]; } //   python namespace
   
    private:
      boost::python::object main_namespace_; // dictionary for the main namespace (an
                                            //   object, not a dict, since declaring it 
                                            //   as a dict copies the dictionary rather
                                            //   than creating a reference to it)
  };

  class PythonInitializer {
    public:
      PythonInitializer(const tstring & exe_directory);
      ~PythonInitializer();
    private:
      PythonInitializer(const PythonInitializer &);
      PythonInitializer & operator=(const PythonInitializer &);
  };
}

#endif
