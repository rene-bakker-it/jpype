
/*****************************************************************************
   Copyright 2004-2008 Steve Menard
   
   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
   
*****************************************************************************/   
#ifndef JP_PRIMITIVE_COMMON_H_
#define JP_PRIMITIVE_COMMON_H_

#include <Python.h>
#include <jpype.h>

typedef unsigned int uint;

#ifdef HAVE_NUMPY
    #define PY_ARRAY_UNIQUE_SYMBOL jpype_ARRAY_API
    #define NO_IMPORT_ARRAY
    #include <numpy/arrayobject.h>
#else
    #define NPY_BOOL 0
    #define NPY_BYTE 0
    #define NPY_SHORT 0
    #define NPY_INT 0
    #define NPY_INT64 0
    #define NPY_FLOAT32 0
    #define NPY_FLOAT64 0
#endif

#define CONVERSION_ERROR_HANDLE(i,o) { if (PyErr_Occurred()!=NULL)  raiseConversionError(i,o); } 

inline void raiseConversionError(int i, PyObject* o)
{
	PyObject* exe = PyErr_Occurred(); 
	// FIXME I don't like this at all.  We are masking the real error with one our own.
	// Thus it is hard to see the problem.
	if(exe != NULL) 
	{
			stringstream ss;
			ss <<  "unable to convert element: " << PyUnicode_FromFormat("%R",o) <<
							" at index: " << i;
			Py_DECREF(o);
			RAISE(JPypeException, ss.str());
	}
}

template <typename array_t,  typename ptr_t>
class JPPrimitiveArrayAccessor
{
	typedef void (JPJavaFrame::*releaseFnc)(array_t, ptr_t, jint);
	typedef ptr_t (JPJavaFrame::*accessFnc)(array_t, jboolean*);

	JPJavaFrame& _frame;
	array_t _array;
	ptr_t _elem;
	releaseFnc _release;
	jboolean _iscopy;
	jint _commit;

public:
	JPPrimitiveArrayAccessor(JPJavaFrame& frame, jarray array, accessFnc access, releaseFnc release)
		: _frame(frame), _array((array_t)array), _release(release)
	{
		_commit = JNI_ABORT;
		_elem = ((&_frame)->*access)(_array, &_iscopy);
	}

	~JPPrimitiveArrayAccessor()
	{
		((&_frame)->*_release)(_array, _elem, _commit);
	}

	ptr_t get() { return _elem; }

	void commit() { _commit = 0; }
};

#if (PY_VERSION_HEX >= 0x02070000)
// for python 2.6 we have also memory view available, but it does not contain the needed functions.
#include <jpype_memory_view.h>

template <typename jarraytype, typename jelementtype, typename setFnc>
inline bool
setViaBuffer(JPJavaFrame& frame, jarray array, int start, uint length, PyObject* sequence, setFnc setter)
{
	JPPyMemoryViewAccessor accessor(sequence);
	if (!accessor.valid())
		return false;

	// ensure length of buffer contains enough elements somehow.
	if ((accessor.size() / sizeof(jelementtype)) != length) 
	{
		std::stringstream ss;
		ss << "Underlying buffer does not contain requested number of elements! Has "
		   << accessor.size() << ", but " << length <<" are requested. Element size is "
		   << sizeof(jelementtype);
		RAISE(JPypeException, ss.str());
	}

	jarraytype a = (jarraytype)array;
	jelementtype* buffer = (jelementtype*) accessor.get();

	(frame.*setter)(a, start, length, buffer);
	return true;
}
#else
template <typename a, typename b, typename c>
bool setViaBuffer(JPJavaFrame& frame, jarray, int, uint, PyObject*, c) {
	return false;
}
#endif

/**
 * gets either a numpy ndarray or a python list with a copy of the underling java array,
 * containing the range [lo, hi].
 *
 * Parameters:
 * -----------
 * lo = low index
 * hi = high index
 * npy_type = e.g NPY_FLOAT64
 * jtype = eg. jdouble
 * convert = function to convert elements to python types. Eg: PyInt_FromLong
 */
template<typename jtype, typename py_wrapper_func>
inline PyObject* getSlice(JPJavaFrame& frame, jarray array, int lo, int hi, int npy_type,
		py_wrapper_func convert)
{
	JPPrimitiveArrayAccessor<jarray, void*> accessor(frame, array, 
		&JPJavaFrame::GetPrimitiveArrayCritical, &JPJavaFrame::ReleasePrimitiveArrayCritical);

	uint len = hi - lo;
#ifdef HAVE_NUMPY
	npy_intp dims[] = {len};
	PyObject* res = PyArray_SimpleNew(1, dims, npy_type);
	if (len > 0)
	{
		jtype* val = (jtype*) accessor.get();
		// use typed numpy arrays for results
		memcpy(((PyArrayObject*) res)->data, &val[lo], len * sizeof(jtype));
	}
	return res;
#else
	PyObject* res = PyList_New(len);
	if (len > 0)
	{
		jtype* val = (jtype*) accessor.get();
		// use python lists for results
		for (Py_ssize_t i = lo; i < hi; i++)
			PyList_SET_ITEM(res, i - lo, convert(val[i]));
	}
	return res;
#endif
}

#endif // JP_PRIMITIVE_COMMON_H_
