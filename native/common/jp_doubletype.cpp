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
#include <jp_primitive_common.h>

HostRef* JPDoubleType::asHostObject(jvalue val)
{
	return JPEnv::getHost()->newFloat(field(val));
}

HostRef* JPDoubleType::asHostObjectFromObject(jvalue val)
{
	return JPEnv::getHost()->newFloat(JPJni::doubleValue(val.l));
}

EMatchType JPDoubleType::canConvertToJava(HostRef* obj)
{
	if (JPEnv::getHost()->isNone(obj))
	{
		return _none;
	}

	if (JPEnv::getHost()->isFloat(obj))
	{
		if (JPEnv::getHost()->isObject(obj))
		{
			return _implicit;
		}
		return _exact;
	}

	if (JPEnv::getHost()->isWrapper(obj))
	{
		JPTypeName name = JPEnv::getHost()->getWrapperTypeName(obj);
		if (name.getType() == JPTypeName::_double)
		{
			return _exact;
		}
	}

	// Java allows conversion to any type with a longer range even if lossy
	if (JPEnv::getHost()->isInt(obj) || JPEnv::getHost()->isLong(obj))
	{
		return _implicit;
	}

	return _none;
}

jvalue JPDoubleType::convertToJava(HostRef* obj)
{
	jvalue res;
	if (JPEnv::getHost()->isWrapper(obj))
	{
		return JPEnv::getHost()->getWrapperValue(obj);
	}
	else if (JPEnv::getHost()->isInt(obj))
	{
		field(res) = JPEnv::getHost()->intAsInt(obj);;
	}
	else if (JPEnv::getHost()->isLong(obj))
	{
		field(res) = (type_t) JPEnv::getHost()->longAsLong(obj);;
	}
	else
	{
		field(res) = (type_t)JPEnv::getHost()->floatAsDouble(obj);
	}
	return res;
}

HostRef* JPDoubleType::convertToDirectBuffer(HostRef* src)
{
	RAISE(JPypeException, "Unable to convert to Direct Buffer");
}

jarray JPDoubleType::newArrayInstance(JPJavaFrame& frame, int sz)
{
	return frame.NewDoubleArray(sz);
}

HostRef* JPDoubleType::getStaticValue(JPJavaFrame& frame, jclass c, jfieldID fid, JPTypeName& tgtType)
{
	jvalue v;
	field(v) = frame.GetStaticDoubleField(c, fid);
	return asHostObject(v);
}

HostRef* JPDoubleType::getInstanceValue(JPJavaFrame& frame, jobject c, jfieldID fid, JPTypeName& tgtType)
{
	jvalue v;
	field(v) = frame.GetDoubleField(c, fid);
	return asHostObject(v);
}

HostRef* JPDoubleType::invokeStatic(JPJavaFrame& frame, jclass claz, jmethodID mth, jvalue* val)
{
	jvalue v;
	field(v) = frame.CallStaticDoubleMethodA(claz, mth, val);
	return asHostObject(v);
}

HostRef* JPDoubleType::invoke(JPJavaFrame& frame, jobject obj, jclass clazz, jmethodID mth, jvalue* val)
{
	jvalue v;
	field(v) = frame.CallNonvirtualDoubleMethodA(obj, clazz, mth, val);
	return asHostObject(v);
}

void JPDoubleType::setStaticValue(JPJavaFrame& frame, jclass c, jfieldID fid, HostRef* obj)
{
	type_t val = field(convertToJava(obj));
	frame.SetStaticDoubleField(c, fid, val);
}

void JPDoubleType::setInstanceValue(JPJavaFrame& frame, jobject c, jfieldID fid, HostRef* obj)
{
	type_t val = field(convertToJava(obj));
	frame.SetDoubleField(c, fid, val);
}

vector<HostRef*> JPDoubleType::getArrayRange(JPJavaFrame& frame, jarray a, int start, int length)
{
	JPPrimitiveArrayAccessor<array_t, type_t*> accessor(frame, a,
			&JPJavaFrame::GetDoubleArrayElements, &JPJavaFrame::ReleaseDoubleArrayElements);

	type_t* val = accessor.get();
	vector<HostRef*> res;
		
	jvalue v;
	for (int i = 0; i < length; i++)
	{
		field(v) = val[i+start];
		res.push_back(asHostObject(v));
	}
	return res;
}

void JPDoubleType::setArrayRange(JPJavaFrame& frame, jarray a, int start, int length, vector<HostRef*>& vals)
{
	JPPrimitiveArrayAccessor<array_t, type_t*> accessor(frame, a,
			&JPJavaFrame::GetDoubleArrayElements, &JPJavaFrame::ReleaseDoubleArrayElements);

	type_t* val = accessor.get();
	for (int i = 0; i < length; i++)
	{
		HostRef* pv = vals[i];
		val[start+i] = field(convertToJava(pv));
	}
	accessor.commit();
}

void JPDoubleType::setArrayRange(JPJavaFrame& frame, jarray a, int start, int length, PyObject* sequence)
{
	if (setViaBuffer<array_t, type_t>(frame, a, start, length, sequence,
		&JPJavaFrame::SetDoubleArrayRegion))
		return;

	JPPrimitiveArrayAccessor<array_t, type_t*> accessor(frame, a,
			&JPJavaFrame::GetDoubleArrayElements, &JPJavaFrame::ReleaseDoubleArrayElements);

	type_t* val = accessor.get();
	for (Py_ssize_t i = 0; i < length; ++i)
	{
		PyObject* o = PySequence_GetItem(sequence, i);
		type_t v = (type_t) PyFloat_AsDouble(o);
		if (v == -1.) { CONVERSION_ERROR_HANDLE(i, o); }
		Py_DECREF(o);
		val[start+i] = v;
	}
	accessor.commit();
}

HostRef* JPDoubleType::getArrayItem(JPJavaFrame& frame, jarray a, int ndx)
{
	array_t array = (array_t)a;
	type_t val;
	frame.GetDoubleArrayRegion(array,ndx, 1, &val);
	jvalue v;
	field(v) = val;
	return asHostObject(v);
}

void JPDoubleType::setArrayItem(JPJavaFrame& frame, jarray a, int ndx , HostRef* obj)
{
	array_t array = (array_t)a;
	type_t val = field(convertToJava(obj));
	frame.SetDoubleArrayRegion(array, ndx, 1, &val);
}

PyObject* JPDoubleType::getArrayRangeToSequence(JPJavaFrame& frame, jarray a, int lo, int hi)
{
	return getSlice<jdouble>(frame, a, lo, hi, NPY_FLOAT64, PyFloat_FromDouble);
}

