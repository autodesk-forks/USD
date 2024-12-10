//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_BASE_TF_EMSCRIPTEN_TYPE_REGISTRATION_H
#define PXR_BASE_TF_EMSCRIPTEN_TYPE_REGISTRATION_H

#ifdef ARCH_OS_WASM_VM
#include <emscripten/bind.h>
// Use this define to make a type without bindings known to Emscripten
#define EMSCRIPTEN_REGISTER_TYPE(TYPE) \
  namespace emscripten { \
      namespace internal { \
        template<>\
        struct TypeID<TYPE> {\
            static constexpr TYPEID get() {\
                return LightTypeID<val>::get();\
            }\
        };\
\
        template<>\
        struct TypeID<const TYPE> {\
            static constexpr TYPEID get() {\
                return LightTypeID<val>::get();\
            }\
        };\
\
        template<>\
        struct TypeID<TYPE&> {\
            static constexpr TYPEID get() {\
                return LightTypeID<val>::get();\
            }\
        };\
\
        template<>\
        struct TypeID<const TYPE&> {\
            static constexpr TYPEID get() {\
                return LightTypeID<val>::get();\
            }\
        };\
    }\
  }

// These two defines together allow you to define custom conversions to
// javascript and from javascript to cpp
#define EMSCRIPTEN_REGISTER_TYPE_CONVERSION(TYPE) \
  namespace emscripten { \
      namespace internal { \
          template<> \
          struct BindingType<TYPE> { \
              typedef EM_VAL WireType; \
              static WireType toWireType(const TYPE& value, rvp::default_tag) { \

#define EMSCRIPTEN_REGISTER_TYPE_CONVERSION_END(TYPE) \
            } \
        }; \
        \
    }\
  }\
EMSCRIPTEN_REGISTER_TYPE(TYPE)

// This define is used to map std::vectors of ValueType as arrays in 
// Javascript
#define EMSCRIPTEN_REGISTER_VECTOR_TO_ARRAY_CONVERSION(ValueType) \
namespace emscripten { \
    namespace internal { \
        template<> \
        struct BindingType<std::vector<ValueType>> { \
            using ValBinding = BindingType<val>; \
            using WireType = ValBinding::WireType; \
        \
            static WireType toWireType(const std::vector<ValueType> &vec, rvp::default_tag) { \
                return ValBinding::toWireType(val::array(vec), rvp::default_tag{}); \
            } \
        \
            static std::vector<ValueType> fromWireType(WireType value) { \
                return vecFromJSArray<ValueType>(ValBinding::fromWireType(value)); \
            } \
        }; \
    } \
}

#endif // ARCH_OS_WASM_VM
#endif // PXR_BASE_TF_EMSCRIPTEN_TYPE_REGISTRATION_H
