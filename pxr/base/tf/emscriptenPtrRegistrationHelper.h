//
// Copyright 2021 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_BASE_TF_EMSCRIPTEN_PTR_REGISTRATION_HELPER_H
#define PXR_BASE_TF_EMSCRIPTEN_PTR_REGISTRATION_HELPER_H

#include "pxr/base/tf/declarePtrs.h"

#ifdef ARCH_OS_WASM_VM
#include <emscripten/bind.h>
#include "pxr/base/tf/emscriptenTypeRegistration.h"

// TYPE should be the class type, without the pxr scope
#define EMSCRIPTEN_REGISTER_SMART_PTR(TYPE) \
    namespace emscripten { \
      template<> \
      struct smart_ptr_trait<pxr::TfRefPtr<pxr::TYPE>> : public default_smart_ptr_trait<pxr::TfRefPtr<pxr::TYPE>> { \
          typedef pxr::TYPE element_type; \
          static pxr::TYPE* get(const pxr::TfRefPtr<pxr::TYPE>& p) { \
              if (p) { \
              return p.operator->(); \
              } else { \
                return NULL; \
              } \
          } \
      }; \
      \
      template<> \
      struct smart_ptr_trait<pxr::TfWeakPtr<pxr::TYPE>> : public default_smart_ptr_trait<pxr::TfWeakPtr<pxr::TYPE>> { \
          typedef pxr::TYPE element_type; \
          static pxr::TYPE* get(const pxr::TfWeakPtr<pxr::TYPE>& p) { \
              if (p) { \
                return p.operator->(); \
              } else { \
                  return NULL; \
              } \
          } \
      }; \
    }

#define EMSCRIPTEN_REGISTER_SDF_HANDLE(TYPE) \
    namespace emscripten { \
      template<> \
      struct smart_ptr_trait<pxr::SdfHandle<pxr::TYPE>> : public default_smart_ptr_trait<pxr::SdfHandle<pxr::TYPE>> { \
          typedef pxr::TYPE element_type; \
          static pxr::TYPE* get(const pxr::SdfHandle<pxr::TYPE>& p) { \
              if (p) { \
              return p.operator->(); \
              } else { \
                return NULL; \
              } \
          } \
      }; \
    }

// TODO: This is not a great solution yet. We shouldn't need to convert weak pointers to ref pointers and back.
// It probably also interferes with the above traits, or emscripten's '.smart_ptr' construct, respectively.
// TYPE should be the class type, without the pxr scope
#define EMSCRIPTEN_ENABLE_WEAK_PTR_CAST(TYPE) \
    EMSCRIPTEN_REGISTER_TYPE_CONVERSION(pxr::TfWeakPtr<pxr::TYPE>) \
        return BindingType<val>::toWireType(val(pxr::TfCreateRefPtrFromProtectedWeakPtr(value)), rvp::default_tag{}); \
    } \
    static pxr::TfWeakPtr<pxr::TYPE> fromWireType(WireType value) { \
        return pxr::TfWeakPtr<pxr::TYPE>(BindingType<val>::fromWireType(value).as<pxr::TfRefPtr<pxr::TYPE>>()); \
    EMSCRIPTEN_REGISTER_TYPE_CONVERSION_END(pxr::TfWeakPtr<pxr::TYPE>)

#endif // ARCH_OS_WASM_VM
#endif // PXR_BASE_TF_EMSCRIPTEN_PTR_REGISTRATION_HELPER_H
