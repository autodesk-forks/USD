//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/tf/diagnosticTrap.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/diagnosticMgr.h"

#include <utility>

PXR_NAMESPACE_OPEN_SCOPE

TfDiagnosticTrap::TfDiagnosticTrap()
{
    TfDiagnosticMgr::GetInstance()._PushTrap(this);
    _active = true;
}

TfDiagnosticTrap::~TfDiagnosticTrap()
{
    Dismiss();
}

void
TfDiagnosticTrap::Dismiss()
{
    if (!std::exchange(_active, false)) {
        return;
    }
    TfDiagnosticMgr::GetInstance()._PopTrap(this);
    _container.Post();
}

void
TfDiagnosticTrap::Clear()
{
    EraseMatching([](TfDiagnosticBase const &) { return true; });
}

void
TfDiagnosticTrap::ClearErrors()
{
    EraseMatching([](TfError const &) { return true; });
}

void
TfDiagnosticTrap::ClearWarnings()
{
    EraseMatching([](TfWarning const &) { return true; });
}

void
TfDiagnosticTrap::ClearStatuses()
{
    EraseMatching([](TfStatus const &) { return true; });
}

bool
TfDiagnosticTrap::IsClean() const {
    return _container.IsEmpty();
}

bool
TfDiagnosticTrap::HasErrors() const
{
    return HasAnyMatching([](TfError const &) { return true; });
}

bool
TfDiagnosticTrap::HasWarnings() const
{
    return HasAnyMatching([](TfWarning const &) { return true; });
}

bool
TfDiagnosticTrap::HasStatuses() const
{
    return HasAnyMatching([](TfStatus const &) { return true; });
}

std::vector<TfError> const &
TfDiagnosticTrap::GetErrors() const
{
    return _container.GetErrors();
}

std::vector<TfWarning> const &
TfDiagnosticTrap::GetWarnings() const
{
    return _container.GetWarnings();
}

std::vector<TfStatus> const &
TfDiagnosticTrap::GetStatuses() const
{
    return _container.GetStatuses();
}

TfDiagnosticTransport
TfDiagnosticTrap::Transport()
{
    TfDiagnosticTransport transport { std::move(_container) };
    _container.Clear();
    _OnContentsChanged();
    return transport;
}

void
TfDiagnosticTrap::_OnContentsChanged() const
{
    TfDiagnosticMgr::GetInstance()._RebuildTrappedDiagnosticsLogText();
}

PXR_NAMESPACE_CLOSE_SCOPE
