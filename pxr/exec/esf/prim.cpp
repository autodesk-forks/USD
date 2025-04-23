//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/esf/prim.h"

#include "pxr/exec/esf/editReason.h"
#include "pxr/exec/esf/journal.h"
#include "pxr/exec/esf/attribute.h"

PXR_NAMESPACE_OPEN_SCOPE

EsfPrimInterface::~EsfPrimInterface() = default;

TfTokenVector EsfPrimInterface::GetAppliedSchemas(EsfJournal *journal) const
{
    if (journal) {
        journal->Add(_GetPath(), EsfEditReason::ResyncedObject);
    }
    return _GetAppliedSchemas();
}

EsfAttribute EsfPrimInterface::GetAttribute(
    const TfToken &attrName,
    EsfJournal *journal) const
{
    if (journal) {
        journal->Add(
            _GetPath().AppendProperty(attrName),
            EsfEditReason::ResyncedObject);
    }
    return _GetAttribute(attrName);
}

EsfPrim EsfPrimInterface::GetParent(EsfJournal *journal) const
{
    if (journal) {
        journal->Add(_GetPath(), EsfEditReason::ResyncedObject);
    }
    return _GetParent();
}

TfType EsfPrimInterface::GetType(EsfJournal *journal) const
{
    if (journal) {
        journal->Add(_GetPath(), EsfEditReason::ResyncedObject);
    }
    return _GetType();
}

PXR_NAMESPACE_CLOSE_SCOPE