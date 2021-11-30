#ifndef PXR_BASE_TF_PY_BOOST_OBJ_WRAPPER_H
#define PXR_BASE_TF_PY_BOOST_OBJ_WRAPPER_H

#ifdef PXR_PYTHON_SUPPORT_ENABLED
#include "pxr/base/tf/pyUtils.h"
#include <boost/python/object.hpp>
#else
#include <type_traits>
#endif

#include <cstddef>

PXR_NAMESPACE_OPEN_SCOPE
/// \brief TfPyBoostObjWrapperStub
/// 
/// Empty stub for ABI compatibility even if Python support is 
/// enabled so we can ensure the alignment is the same.
class TfPyBoostObjWrapperStub
{
public:
    static constexpr std::size_t Size = 8;
    static constexpr std::size_t Align = 8;

private:
    std::aligned_storage<Size, Align>::type _stub;
};

/// \brief TfPyBoostObjWrapper
///
/// Wrapper of the boost::python::api::object
///
/// The wrapper provides a uniform type 
/// usable when python extensions are accessible
/// and when no python extensions are compiled
#ifdef PXR_PYTHON_SUPPORT_ENABLED
class TfPyBoostObjWrapper
{
  using WrappedType = boost::python::api::object;

 public:
  // Just a constructor that forwards everything to the wrapped object.
  template<typename ...Args>
    TfPyBoostObjWrapper(Args&&... args)
    : _wrappedObject(std::forward<Args>(args)...)
    {}

  inline WrappedType& get() { return _wrappedObject; }

 private:
  WrappedType _wrappedObject;
};
// Verify binary compatibility between the expected size from the stub and the class
static_assert(sizeof(TfPyBoostObjWrapper) == sizeof(TfPyBoostObjWrapperStub),
              "ABI break: Incompatible class sizes.");
static_assert(alignof(TfPyBoostObjWrapper) == alignof(TfPyBoostObjWrapperStub),
              "ABI break: Incompatible class alignments.");
#else
class TfPyBoostObjWrapper : TfPyBoostObjWrapperStub
{
};
#endif

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_BASE_TF_PY_BOOST_OBJ_WRAPPER_H
