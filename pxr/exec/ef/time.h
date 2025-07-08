//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_EF_TIME_H
#define PXR_EXEC_EF_TIME_H

/// \file

#include "pxr/pxr.h"

#include "pxr/exec/ef/api.h"

#include "pxr/usd/usd/timeCode.h"

#include <cstdint>
#include <iosfwd>

PXR_NAMESPACE_OPEN_SCOPE

///////////////////////////////////////////////////////////////////////////////
///
/// A class that represents a point in time for execution.
///
/// Time has a frame value that can be double-valued or set to "default". The
/// "default" frame can be thought to represent a point that is not on the
/// timeline.
///
/// The concept of EfTime having an evaluation location allows us to express a
/// time value that is infinitesimally close - but *just before*, or directly
/// *at* the specified frame value. This is important when using EfTime to
/// evaluate functions with discontinuities (i.e., animation splines with
/// dual-valued knots).
///
/// EfTime also contains spline evaluation flags that cause splines to be
/// evaluated in application-specific, special ways. These flags should
/// be consumed when spline evaluation is dispatched to the application-level
/// evaluation logic.
///
class EfTime
{
public:

    /// Evaluation location.
    ///
    /// The evaluation location indicates how the given frame value should be
    /// used when evaluating functions: 
    /// If the location is \e EvaluateAtTime, we evaluate the function at the
    /// exact, specified time.
    /// If the location is \e EvaluatePre, we evaluate the limit of the function
    /// as it approaches the specified frame. In effect, we evaluate a spline
    /// at a time that is infinitesimally close to the frame value. This is
    /// useful if there is a discontinuity in the function or spline at the
    /// given frame.
    /// At the default frame, evaluation location has no effect.
    ///
    enum EvaluationLocation : uint8_t {
        EvaluatePre,
        EvaluateAtTime
    };

    /// Data type for storing app-specific spline evaluation flags.
    ///
    using SplineEvaluationFlags = uint8_t;

    /// A default constructed EfTime is set to the default frame value.
    ///
    EfTime() :
        _timeCode(UsdTimeCode::Default()),
        _location(EvaluateAtTime),
        _splineFlags(0)
    {}

    /// Constructs an EfTime object for a specific frame with an optional
    /// evaluation location and set of spline flags.
    ///
    explicit EfTime(
        const UsdTimeCode timeCode,
        EvaluationLocation location = EvaluateAtTime,
        SplineEvaluationFlags splineFlags = 0) :
        _timeCode(timeCode),
        _location(location),
        _splineFlags(splineFlags)
    {}


    /// \name Time code
    /// @{

    /// Returns the time code.
    ///
    const UsdTimeCode GetTimeCode() const {
        return _timeCode;
    }

    /// Sets the time code to \p timeCode.
    ///
    void SetTimeCode(const UsdTimeCode timeCode) {
        _timeCode = timeCode;
    }

    /// @}


    /// \name Evaluation location
    /// @{

    /// Returns the evaluation location.
    ///
    EvaluationLocation GetEvaluationLocation() const {
        return _location;
    }

    /// Sets where functions are evaluated relative to the frame value.
    ///
    void SetEvaluationLocation(EvaluationLocation location) {
        _location = location;
    }

    /// @}


    /// \name Spline evaluation flags
    /// @{

    /// Returns the spline evaluation flags that will be used during evaluation.
    ///
    SplineEvaluationFlags GetSplineEvaluationFlags() const {
        return _splineFlags;
    }

    /// Sets the spline evaluation flags that will be used during evaluation.
    ///
    void SetSplineEvaluationFlags(SplineEvaluationFlags flags) {
        _splineFlags = flags;
    }

    /// @}


    /// Returns \c true if *this == rhs.
    ///
    bool operator==(const EfTime &rhs) const {
        if (!_timeCode.IsDefault() && !rhs._timeCode.IsDefault()) {
            return _timeCode == rhs._timeCode
                && _location == rhs._location
                && _splineFlags == rhs._splineFlags;
        }
        return _timeCode.IsDefault() == rhs._timeCode.IsDefault();
    }

    bool operator!=(const EfTime &rhs) const {
        return !(*this == rhs);
    }

    /// Returns \c true if *this < rhs.
    ///
    /// Note that a time with frame set to "default" is lesser than all
    /// non-default times, and that a time with \e EvaluatePre is less than
    /// the corresponding time with \e EvaluateAtTime.
    ///
    /// Also note that evaluation location and spline flags have no effect on
    /// ordering for the default frame. Spline flags are only used for stable
    /// ordering, but there is no logical ordering between two sets of spline
    /// flags.
    ///
    bool operator<(const EfTime &rhs) const {
        if (_timeCode.IsDefault() || rhs._timeCode.IsDefault()) {
            return _timeCode < rhs._timeCode;
        }

        return _timeCode < rhs._timeCode
            || (_timeCode == rhs._timeCode && _location < rhs._location) 
            || (_timeCode == rhs._timeCode && _location == rhs._location &&
                _splineFlags < rhs._splineFlags);
    }

    bool operator<=(const EfTime &rhs) const {
        return !(rhs < *this);
    }

    bool operator>(const EfTime &rhs) const {
        return rhs < *this;
    }

    bool operator>=(const EfTime &rhs) const {
        return !(*this < rhs);
    }

    /// Provides a hash function for EfTime.
    ///
    template <class HashState>
    friend void TfHashAppend(HashState &h, const EfTime &t) {
        h.Append(t._timeCode);
        if (!t._timeCode.IsDefault()) {
            h.Append(t._location);
            h.Append(t._splineFlags);
        }
    }

    /// Returns this object as string. Note that evaluation location will only
    /// be denoted in the output string if it is set to \e EvaluatePre.
    ///
    std::string GetAsString() const;

private:

    // The time code value.
    UsdTimeCode _timeCode;

    // The function evaluation location.
    EvaluationLocation _location;

    // The spline evaluation flags to use during computation.
    SplineEvaluationFlags _splineFlags;
};

/// Output an EfTime.
///
EF_API
std::ostream &operator<<(std::ostream &os, const EfTime &time);

PXR_NAMESPACE_CLOSE_SCOPE

#endif
