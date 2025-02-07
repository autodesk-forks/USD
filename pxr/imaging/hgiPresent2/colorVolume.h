//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HGIPRESENT2_COLORVOLUME_H
#define PXR_IMAGING_HGIPRESENT2_COLORVOLUME_H

#include "pxr/pxr.h"

#include "pxr/base/gf/ilmbase_halfLimits.h"
#include "pxr/base/gf/range1d.h"

#include <ostream>

PXR_NAMESPACE_OPEN_SCOPE

/// Properties of the color volume that is defined by a format.
/// These are used to compare different color spaces against a
/// preferred one and pick the closest match.
struct ColorVolume
{
    // Min and max value for a color component.
    // These are assumed to all be the same.
    GfRange1d axisRange;
    // Difference between 1 and the next representable
    // value of a color component. An absolute reference
    // value is used because floating point formats have
    // value relative resolution. For formats with varying
    // resolution per component, the smallest color one is used.
    double epsilon;
    // The number of dimension, either 3 or 4 (+1 for alpha).
    uint32_t dimensions;

    explicit operator bool() const
    {
        return !axisRange.IsEmpty();
    }

    // The result of matching this color volume against another one.
    struct Match
    {
        // Precentage of the range covered by the other color volume.
        double coverage;
        // How many bits of data are loss by the other color volume.
        float dataLoss;
        // How many bits of data are wasted by the other color volume.
        float dataSlack;
        // How many extra dimensions the other color volume has.
        uint32_t extraDimensions;

        // Allow for total ordering of color volume matches by comparing
        // the match results, breaking ties according to importance of the
        // match property: prefer wasting data over losing it.
        bool operator<(const Match &other) const
        {
            if (coverage != other.coverage) {
                return coverage > other.coverage;
            }
            if (dataLoss != other.dataLoss) {
                return dataLoss < other.dataLoss;
            }
            if (dataSlack != other.dataSlack) {
                return dataSlack < other.dataSlack;
            }
            if (extraDimensions != other.extraDimensions) {
                return extraDimensions < other.extraDimensions;
            }
            return false;
        }
    };

    // Match this color volume against another one, so they can be ranked.
    Match
    ComputeMatch(const ColorVolume &other) const
    {
        Match score{};
        score.coverage = GfRange1d::Intersection(axisRange, other.axisRange).
            GetSize() / axisRange.GetSize();
        // Equivalent to log2(other.epsilon) - log2(epsilon)
        const auto dataDiff = static_cast<float>(std::log2(
            other.epsilon / epsilon));
        score.dataLoss = std::max(dataDiff, 0.f);
        score.dataSlack = std::max(-dataDiff, 0.f);
        score.extraDimensions = other.dimensions - dimensions;
        return score;
    }

    static
    ColorVolume
    ForNormInt(bool signed_, uint32_t bits, uint32_t dimensions)
    {
        return {{signed_ ? -1. : 0., 1.},
            (signed_ ? 2. : 1.) / (1 << bits), dimensions};
    }

    static
    ColorVolume
    ForHalf(uint32_t dimensions)
    {
        return {{-PXR_HALF_MAX, PXR_HALF_MAX}, PXR_HALF_EPSILON, dimensions};
    }

    static
    ColorVolume
    ForSingle(uint32_t dimensions)
    {
        return {{-std::numeric_limits<float>::max(), std::numeric_limits<float>::max()}, std::numeric_limits<float>::epsilon(), dimensions};
    }


    static
    ColorVolume
    ForDouble(uint32_t dimensions)
    {
        return {{-std::numeric_limits<double>::max(), std::numeric_limits<double>::max()}, std::numeric_limits<double>::epsilon(), dimensions};
    }

    static
    ColorVolume
    ForFloat(bool signed_, uint32_t exponentBits, uint32_t mantissaBits, uint32_t dimensions)
    {
        // Float value formula: 2^(exponent − exponentBias) * (1 + mantissa / 2^mantissaBits)
        // Exponent bias formula: 2^(exponentBits - 1) - 1

        // Max exponent and mantissa: 2^exponentBits - 2, 2^mantissaBits - 1
        // Max value formula: 2^((2^exponentBits - 2) − exponentBias) * (1 + (2^mantissaBits - 1) / 2^mantissaBits)
        //   Substitute and simplify: 2^exponentBias * (2 - 1 / 2^mantissaBits)
        const auto exponentBias = (1 << (exponentBits - 1)) - 1;
        const auto maxValue = (1 << exponentBias) * (2 + 1. / (1 << mantissaBits));
        // Epsilon value formula: 2^(exponentBias − exponentBias) * (1 + (2^mantissaBits - 1) / 2^mantissaBits) - 2^((1 + exponentBias) − exponentBias) * (1 + 0 / 2^mantissaBits)
        //   Simplify: 1 / 2^mantissaBits
        const auto epsilon = 1. / (1 << mantissaBits);

        return {{signed_ ? -maxValue : 0, maxValue}, epsilon, dimensions};
    }
};

inline std::ostream&
operator<<(std::ostream& out, const ColorVolume::Match& match)
{
    return out << "ColorVolume::Match(" <<
        "coverage: " << match.coverage <<
        ", loss: " << match.dataLoss <<
        ", slack: " << match.dataSlack <<
        ", extraDims: " << match.extraDimensions << ")";
}

PXR_NAMESPACE_CLOSE_SCOPE


#endif
