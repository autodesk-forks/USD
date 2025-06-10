//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_IMAGING_USD_IMAGING_DATA_SOURCE_CONVERTED_PRIMVARS_H
#define PXR_USD_IMAGING_USD_IMAGING_DATA_SOURCE_CONVERTED_PRIMVARS_H

#include "pxr/usdImaging/usdImaging/dataSourceAttribute.h"
#include "pxr/usdImaging/usdImaging/dataSourcePrimvars.h"

#include "pxr/usd/usdGeom/primvar.h"
#include "pxr/usd/usdGeom/primvarsAPI.h"

#include "pxr/imaging/hd/primvarSchema.h"
PXR_NAMESPACE_OPEN_SCOPE


/// \class UsdImagingDataSourceConvertedAttribute<primvarType, attributeType>
///
/// A data source that represents a USD Attribute which requires conversion.
/// A converter function is provided to convert the attribute type to the primvar type.
///
template <typename primvarType, typename attributeType>
class UsdImagingDataSourceConvertedAttribute : public UsdImagingDataSourceAttribute<attributeType>
{
public:
    HD_DECLARE_DATASOURCE_TEMPLATE(UsdImagingDataSourceConvertedAttribute, primvarType, attributeType)

    using Base = UsdImagingDataSourceAttribute<attributeType>;
    using ConverterFunction = std::function<primvarType(const attributeType&)>;

    /// Returns the VtValue of this attribute at a given \p shutterOffset
    ///
    VtValue GetValue(HdSampledDataSource::Time shutterOffset) override
    {
        // Do conversion before return the value.
        primvarType result = _converter(Base::GetTypedValue(shutterOffset));
        return VtValue(result);
    }

protected:
    UsdImagingDataSourceConvertedAttribute(
        ConverterFunction converter,
        const UsdAttribute& usdAttr,
        const UsdImagingDataSourceStageGlobals& stageGlobals,
        const SdfPath& sceneIndexPath = SdfPath::EmptyPath(),
        const HdDataSourceLocator& timeVaryingFlagLocator =
        HdDataSourceLocator::EmptyLocator())
        : UsdImagingDataSourceAttribute<attributeType>(usdAttr, stageGlobals, sceneIndexPath, timeVaryingFlagLocator),
        _converter(converter)
    {}

    UsdImagingDataSourceConvertedAttribute(
        ConverterFunction converter,
        const UsdAttributeQuery& usdAttrQuery,
        const UsdImagingDataSourceStageGlobals& stageGlobals,
        const SdfPath& sceneIndexPath = SdfPath::EmptyPath(),
        const HdDataSourceLocator& timeVaryingFlagLocator =
        HdDataSourceLocator::EmptyLocator())
        : UsdImagingDataSourceAttribute<attributeType>(usdAttrQuery, stageGlobals, sceneIndexPath, timeVaryingFlagLocator),
        _converter(converter)
    {}

    ConverterFunction _converter;
};

/// \class UsdImagingDataSourceConvertedPrimvar<primvarType, attributeType>
///
/// A data source that represents a USD Primvar which requires conversion.
/// A converter function is provided to convert the attribute type to the primvar type.
///
template <typename primvarType, typename attributeType>
class UsdImagingDataSourceConvertedPrimvar : public UsdImagingDataSourcePrimvar
{
public:
    // Because there are more than one template arguments, we can not use the HD_DECLARE_DATASOURCE macro.
    HD_DECLARE_DATASOURCE_TEMPLATE(UsdImagingDataSourceConvertedPrimvar, primvarType, attributeType)

    using ConverterFunction = std::function<primvarType(const attributeType&)>;

    HdDataSourceBaseHandle Get(const TfToken & name) override
    {
        TRACE_FUNCTION();

        const bool indexed = (_indicesQuery.IsValid() && _indicesQuery.HasValue());

        if (indexed) {
            if (name == HdPrimvarSchemaTokens->indexedPrimvarValue) {
                return UsdImagingDataSourceConvertedAttribute<primvarType, attributeType>::New(
                    _converter, _valueQuery, _stageGlobals);
            }
            else if (name == HdPrimvarSchemaTokens->indices) {
                return UsdImagingDataSourceConvertedAttribute<primvarType, attributeType>::New(
                    _converter, _indicesQuery, _stageGlobals);
            }
        }
        else {
            if (name == HdPrimvarSchemaTokens->primvarValue) {
                return UsdImagingDataSourceConvertedAttribute<primvarType, attributeType>::New(
                    _converter, _valueQuery, _stageGlobals);
            }
        }

        if (name == HdPrimvarSchemaTokens->interpolation) {
            return _interpolation;
        }
        if (name == HdPrimvarSchemaTokens->role) {
            return _role;
        }
        if (name == HdPrimvarSchemaTokens->elementSize) {
            return _elementSize;
        }
        return nullptr;
    }

private:
    UsdImagingDataSourceConvertedPrimvar(
        ConverterFunction converter, 
        const SdfPath &sceneIndexPath,
        const TfToken &name,
        const UsdImagingDataSourceStageGlobals &stageGlobals,
        UsdAttributeQuery valueQuery,
        UsdAttributeQuery indicesQuery,
        HdTokenDataSourceHandle interpolation,
        HdTokenDataSourceHandle role,
        HdIntDataSourceHandle elementSize = nullptr)
        : UsdImagingDataSourcePrimvar(sceneIndexPath, name, stageGlobals, valueQuery, indicesQuery, interpolation, role, elementSize),
        _converter(converter)
    {}

    ConverterFunction _converter;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_IMAGING_USD_IMAGING_DATA_SOURCE_CONVERTED_PRIMVARS_H
