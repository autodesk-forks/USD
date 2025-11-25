//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
///
/// \file js/value.cpp

#include "pxr/pxr.h"
#include "pxr/base/js/value.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/staticData.h"
#include "pxr/base/tf/stringUtils.h"

PXR_NAMESPACE_OPEN_SCOPE

// Variant type index that specifies a uint64_t value is held.
//
// This relies on the position of _JsNull and uint64_t in the variant
// type declaration, as well as the value of NullType in the Type enum.
static const size_t _UInt64Type = JsValue::NullType + 1;

// Compile-time mapping between a type held by JsValue::_Holder and the
// corresponding JsValue::Type enum value.
template <typename T>
static constexpr JsValue::Type _EnumValueForType = {};
template <>
constexpr JsValue::Type _EnumValueForType<JsObject> = JsValue::ObjectType;
template <>
constexpr JsValue::Type _EnumValueForType<JsArray> = JsValue::ArrayType;
template <>
constexpr JsValue::Type _EnumValueForType<std::string> = JsValue::StringType;

namespace {
template <enum JsValue::Type EnumValue>
struct _TypeForEnumValue;
template <>
struct _TypeForEnumValue<JsValue::ObjectType> { using type = JsObject; };
template <>
struct _TypeForEnumValue<JsValue::ArrayType> { using type = JsArray; };
template <>
struct _TypeForEnumValue<JsValue::StringType> { using type = std::string; };
}

/// Helper to reference count larger value types.
template <typename T>
struct JsValue::_Holder : _HolderBase<_EnumValueForType<T>>
{
    using Base = _HolderBase<_EnumValueForType<T>>;
    using BasePtr = TfDelegatedCountPtr<Base>;

    template <typename... Args>
    static BasePtr New(Args&& ...args) {
        return BasePtr(
            TfDelegatedCountDoNotIncrementTag,
            new _Holder<T>(std::forward<Args>(args)...));
    }

    template <typename... Ts>
    static const T& UncheckedGet(const std::variant<Ts...>& value) {
        return UncheckedGet(*std::get_if<BasePtr>(&value));
    }

    static const T& UncheckedGet(const BasePtr& ptr) {
        const _Holder& holder = static_cast<const _Holder&>(*ptr);
        return holder._value;
    }

private:
    explicit _Holder(T&& value) : _value(std::move(value)) {}
    explicit _Holder(const T& value) : _value(value) {}

private:
    T _value;
};

template <enum JsValue::Type EnumValue>
void
JsValue::_HolderBase<EnumValue>::_Delete(const _HolderBase *h) noexcept
{
    using T = typename _TypeForEnumValue<EnumValue>::type;
    delete static_cast<const _Holder<T>*>(h);
}
template struct JsValue::_HolderBase<JsValue::ObjectType>;
template struct JsValue::_HolderBase<JsValue::ArrayType>;
template struct JsValue::_HolderBase<JsValue::StringType>;

static std::string
_GetTypeName(const size_t t)
{
    switch (t) {
    case JsValue::ObjectType: return "object";
    case JsValue::ArrayType:  return "array";
    case JsValue::StringType: return "string";
    case JsValue::BoolType:   return "bool";
    case JsValue::IntType:    return "int";
    case JsValue::RealType:   return "real";
    case JsValue::NullType:   return "null";
    case _UInt64Type:         return "int";
    default:                  return "unknown";
    };
}

JsValue::JsValue()
    : _value(_JsNull())
{
    // Do Nothing.
}

JsValue::JsValue(const JsObject& value)
    : _value(_Holder<JsObject>::New(value))
{
    // Do Nothing.
}

JsValue::JsValue(JsObject&& value)
    : _value(_Holder<JsObject>::New(std::move(value)))
{
    // Do Nothing.
}

JsValue::JsValue(const JsArray& value)
    : _value(_Holder<JsArray>::New(value))
{
    // Do Nothing.
}

JsValue::JsValue(JsArray&& value)
    : _value(_Holder<JsArray>::New(std::move(value)))
{
    // Do Nothing.
}

JsValue::JsValue(const char* value)
    : _value(_Holder<std::string>::New(value))
{
    // Do Nothing.
}

JsValue::JsValue(const std::string& value)
    : _value(_Holder<std::string>::New(value))
{
    // Do Nothing.
}

JsValue::JsValue(std::string&& value)
    : _value(_Holder<std::string>::New(std::move(value)))
{
    // Do Nothing.
}

JsValue::JsValue(bool value)
    : _value(value)
{
    // Do Nothing.
}

JsValue::JsValue(int value)
    : _value(static_cast<int64_t>(value))
{
    // Do Nothing.
}

JsValue::JsValue(int64_t value)
    : _value(value)
{
    // Do Nothing.
}

JsValue::JsValue(uint64_t value)
    : _value(value)
{
    // Do Nothing.
}

JsValue::JsValue(double value)
    : _value(value)
{
    // Do Nothing.
}

static JsValue::Type
_GetTypeFromIndex(
    size_t typeIndex)
{
    return (typeIndex == _UInt64Type)
        ? JsValue::IntType
        : static_cast<JsValue::Type>(typeIndex);
}

static bool
_CheckType(
    const size_t heldTypeIndex,
    const JsValue::Type requestedType,
    std::string* whyNot)
{
    const JsValue::Type heldType = _GetTypeFromIndex(heldTypeIndex);
    if (heldType != requestedType) {
        if (whyNot) {
            *whyNot = TfStringPrintf(
                "Attempt to get %s from value holding %s",
                _GetTypeName(requestedType).c_str(),
                _GetTypeName(heldType).c_str());
        }
        return false;
    }
    return true;
}

const JsObject&
JsValue::GetJsObject() const
{
    std::string whyNot;
    if (!_CheckType(_value.index(), ObjectType, &whyNot)) {
        TF_CODING_ERROR(whyNot);
        static TfStaticData<JsObject> _emptyObject;
        return *_emptyObject;
    }

    return _Holder<JsObject>::UncheckedGet(_value);
}

const JsArray&
JsValue::GetJsArray() const
{
    std::string whyNot;
    if (!_CheckType(_value.index(), ArrayType, &whyNot)) {
        TF_CODING_ERROR(whyNot);
        static TfStaticData<JsArray> _emptyArray;
        return *_emptyArray;
    }

    return _Holder<JsArray>::UncheckedGet(_value);
}

const std::string&
JsValue::GetString() const
{
    std::string whyNot;
    if (!_CheckType(_value.index(), StringType, &whyNot)) {
        TF_CODING_ERROR(whyNot);
        static TfStaticData<std::string> _emptyString;
        return *_emptyString;
    }

    return _Holder<std::string>::UncheckedGet(_value);
}

bool
JsValue::GetBool() const
{
    std::string whyNot;
    if (!_CheckType(_value.index(), BoolType, &whyNot)) {
        TF_CODING_ERROR(whyNot);
        return false;
    }

    return std::get<bool>(_value);
}

int
JsValue::GetInt() const
{
    std::string whyNot;
    if (!_CheckType(_value.index(), IntType, &whyNot)) {
        TF_CODING_ERROR(whyNot);
        return 0;
    }

    return static_cast<int>(GetInt64());
}

int64_t
JsValue::GetInt64() const
{
    std::string whyNot;
    if (!_CheckType(_value.index(), IntType, &whyNot)) {
        TF_CODING_ERROR(whyNot);
        return 0;
    }

    if (IsUInt64())
        return static_cast<int64_t>(GetUInt64());

    return std::get<int64_t>(_value);
}

uint64_t
JsValue::GetUInt64() const
{
    std::string whyNot;
    if (!_CheckType(_value.index(), IntType, &whyNot)) {
        TF_CODING_ERROR(whyNot);
        return 0;
    }

    if (!IsUInt64())
        return static_cast<uint64_t>(GetInt64());

    return std::get<uint64_t>(_value);
}

double
JsValue::GetReal() const
{
    if (_GetTypeFromIndex(_value.index()) == IntType) {
        return IsUInt64() ?
            static_cast<double>(GetUInt64()) :
            static_cast<double>(GetInt64());
    }

    std::string whyNot;
    if (!_CheckType(_value.index(), RealType, &whyNot)) {
        TF_CODING_ERROR(whyNot);
        return 0;
    }

    return std::get<double>(_value);
}

JsValue::Type
JsValue::GetType() const
{
    return _GetTypeFromIndex(_value.index());
}

std::string
JsValue::GetTypeName() const
{
    return _GetTypeName(_value.index());
}

bool
JsValue::IsObject() const
{
    return _value.index() == ObjectType;
}

bool
JsValue::IsArray() const
{
    return _value.index() == ArrayType;
}

bool
JsValue::IsString() const
{
    return _value.index() == StringType;
}

bool
JsValue::IsBool() const
{
    return _value.index() == BoolType;
}

bool
JsValue::IsInt() const
{
    const size_t index = _value.index();
    return index == IntType || index == _UInt64Type;
}

bool
JsValue::IsReal() const
{
    return _value.index() == RealType;
}

bool
JsValue::IsUInt64() const
{
    return _value.index() == _UInt64Type;
}

bool
JsValue::IsNull() const
{
    return _value.index() == NullType;
}

JsValue::operator bool() const
{
    return !IsNull();
}

// Visitor that compares a variant's value with the value held by another
// JsValue.
struct JsValue::_IsValueEqualVisitor
{
    template <enum Type EnumValue>
    using BasePtr = TfDelegatedCountPtr<_HolderBase<EnumValue>>;

    // Overload for types held indirectly via _Holder.
    template <enum Type EnumValue>
    bool operator()(
        const BasePtr<EnumValue>& lhs,
        const BasePtr<EnumValue>& rhs) const {
        using T = typename _TypeForEnumValue<EnumValue>::type;
        return _Holder<T>::UncheckedGet(lhs) == _Holder<T>::UncheckedGet(rhs);
    }

    // Overload for comparing the same type.
    template <typename T>
    bool operator()(const T& lhs, const T& rhs) const {
        return lhs == rhs;
    }

    // Different types never compare equal.
    //
    // Implicitly, this means that values holding mathematically equal numbers
    // may compare not equal depending on the C++ type used to hold the value,
    // i.e. 1.0 vs 1L vs 1UL.
    template <typename T, typename U>
    bool operator()(const T&, const U&) const {
        return false;
    }
};

bool
JsValue::operator==(const JsValue& other) const
{
    return std::visit(_IsValueEqualVisitor{}, _value, other._value);
}

bool
JsValue::operator!=(const JsValue& other) const
{
    return !(*this == other);
}

PXR_NAMESPACE_CLOSE_SCOPE
