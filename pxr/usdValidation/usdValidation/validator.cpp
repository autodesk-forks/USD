//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/usdValidation/usdValidation/validator.h"

#include "pxr/usdValidation/usdValidation/error.h"
#include "pxr/usdValidation/usdValidation/timeRange.h"

PXR_NAMESPACE_OPEN_SCOPE

UsdValidationValidator::UsdValidationValidator(
    const UsdValidationValidatorMetadata &metadata)
    : _metadata(metadata)
{
}

UsdValidationValidator::UsdValidationValidator(
    const UsdValidationValidatorMetadata &metadata,
    const UsdValidateLayerTaskFn &validateLayerTaskFn,
    std::vector<UsdValidationFixer> fixers /*= {} */)
    : _metadata(metadata), 
    _validateTaskFn(validateLayerTaskFn),
    _fixers(fixers)
{
    _ValidateFixerNames();
}

UsdValidationValidator::UsdValidationValidator(
    const UsdValidationValidatorMetadata &metadata,
    const UsdValidateStageTaskFn &validateStageTaskFn,
    std::vector<UsdValidationFixer> fixers /*= {} */)
    : _metadata(metadata), 
    _validateTaskFn(validateStageTaskFn),
    _fixers(fixers)
{
    _ValidateFixerNames();
}

UsdValidationValidator::UsdValidationValidator(
    const UsdValidationValidatorMetadata &metadata,
    const UsdValidatePrimTaskFn &validatePrimTaskFn,
    std::vector<UsdValidationFixer> fixers /*= {} */)
    : _metadata(metadata),
    _validateTaskFn(validatePrimTaskFn),
    _fixers(fixers)
{
    _ValidateFixerNames();
}

void
UsdValidationValidator::_ValidateFixerNames() const
{
    std::unordered_set<TfToken, TfToken::HashFunctor> fixerNames;
    for (const UsdValidationFixer &fixer : _fixers) {
        if (fixerNames.count(fixer.GetName())) {
            TF_CODING_ERROR(
                "Validator '%s' has multiple fixers with the same name '%s'. "
                "Fixer names must be unique per validator.",
                _metadata.name.GetText(), fixer.GetName().GetText());
        } else {
            fixerNames.insert(fixer.GetName());
        }
    }
}

const UsdValidateLayerTaskFn *
UsdValidationValidator::_GetValidateLayerTask() const
{
    return std::get_if<UsdValidateLayerTaskFn>(&_validateTaskFn);
}

const UsdValidateStageTaskFn *
UsdValidationValidator::_GetValidateStageTask() const
{
    return std::get_if<UsdValidateStageTaskFn>(&_validateTaskFn);
}

const UsdValidatePrimTaskFn *
UsdValidationValidator::_GetValidatePrimTask() const
{
    return std::get_if<UsdValidatePrimTaskFn>(&_validateTaskFn);
}

const std::vector<const UsdValidationFixer *>
UsdValidationValidator::GetFixers() const
{
    std::vector<const UsdValidationFixer *> fixers;
    fixers.reserve(_fixers.size());
    for (const UsdValidationFixer &fixer : _fixers) {
        fixers.push_back(&fixer);
    }
    return fixers;
}

const UsdValidationFixer *
UsdValidationValidator::GetFixerByName(const TfToken &name) const
{
    auto it = std::find_if(_fixers.begin(), _fixers.end(),
        [&name](const UsdValidationFixer& fixer) {
            return name == fixer.GetName();
        });

    if (it == _fixers.end()) {
        return nullptr;
    }

    return &(*it);
}

const std::vector<const UsdValidationFixer *>
UsdValidationValidator::GetFixersByErrorName(
    const TfToken &errorName) const
{
    std::vector<const UsdValidationFixer *> fixersForErrorName;
    for (const UsdValidationFixer &fixer : _fixers) {
        if (fixer.IsAssociatedWithErrorName(errorName)) {
            fixersForErrorName.push_back(&fixer);
        }
    }
    return fixersForErrorName;
}

const UsdValidationFixer *
UsdValidationValidator::GetFixerByNameAndErrorName(
    const TfToken &fixerName, const TfToken &errorName) const
{
    auto it = std::find_if(_fixers.begin(), _fixers.end(),
        [&fixerName, &errorName](const UsdValidationFixer& fixer) {
            return fixerName == fixer.GetName() &&
                   fixer.IsAssociatedWithErrorName(errorName);
        });
    if (it == _fixers.end()) {
        return nullptr;
    }
    return &(*it);
}

const std::vector<const UsdValidationFixer *>
UsdValidationValidator::GetFixersByKeywords(const TfTokenVector &keywords) const
{
    std::vector<const UsdValidationFixer *> fixersForKeywords;
    for (const UsdValidationFixer &fixer : _fixers) {
        for (const TfToken &keyword : keywords) {
            if (fixer.HasKeyword(keyword)) {
                fixersForKeywords.push_back(&fixer);
                // Only add the fixer once, even if it has multiple matching 
                // keywords.
                break;
            }
        }
    }
    return fixersForKeywords;
}

UsdValidationErrorVector
UsdValidationValidator::Validate(const SdfLayerHandle &layer) const
{
    const UsdValidateLayerTaskFn *layerTaskFn = _GetValidateLayerTask();
    if (layerTaskFn) {
        UsdValidationErrorVector errors = (*layerTaskFn)(layer);
        for (UsdValidationError &error : errors) {
            error._SetValidator(this);
        }
        return errors;
    }
    return {};
}

UsdValidationErrorVector
UsdValidationValidator::Validate(
    const UsdStagePtr &usdStage,
    const UsdValidationTimeRange &timeRange) const
{
    const UsdValidateStageTaskFn *stageTaskFn = _GetValidateStageTask();
    if (stageTaskFn) {
        UsdValidationErrorVector errors = 
            (*stageTaskFn)(usdStage, timeRange);
        for (UsdValidationError &error : errors) {
            error._SetValidator(this);
        }
        return errors;
    }
    return {};
}

UsdValidationErrorVector
UsdValidationValidator::Validate(
    const UsdPrim &usdPrim, 
    const UsdValidationTimeRange &timeRange) const
{
    const UsdValidatePrimTaskFn *primTaskFn = _GetValidatePrimTask();
    if (primTaskFn) {
        UsdValidationErrorVector errors = 
            (*primTaskFn)(usdPrim, timeRange);
        for (UsdValidationError &error : errors) {
            error._SetValidator(this);
        }
        return errors;
    }
    return {};
}

UsdValidationValidatorSuite::UsdValidationValidatorSuite(
    const UsdValidationValidatorMetadata &metadata,
    const std::vector<const UsdValidationValidator *> &validators)
    : _metadata(metadata)
    , _containedValidators(validators)
{
}

PXR_NAMESPACE_CLOSE_SCOPE
