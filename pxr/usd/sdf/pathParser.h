//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_SDF_PATH_PARSER_H
#define PXR_USD_SDF_PATH_PARSER_H

#include "pxr/pxr.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/tf/unicodeUtils.h"
#include "pxr/usd/sdf/path.h"

#include "pxr/base/pegtl/pegtl.hpp"

PXR_NAMESPACE_OPEN_SCOPE

bool
Sdf_ParsePath(std::string const &pathStr, SdfPath *path, std::string *errMsg);

namespace Sdf_PathParser {

namespace PEGTL_NS = PXR_PEGTL_NAMESPACE;

////////////////////////////////////////////////////////////////////////
// Helper rules for parsing UTF8 content
struct XidStart
{
    template <typename ParseInput>
    static bool match(ParseInput& in)
    {
        if (!in.empty())
        {
            // peek at the next character in the input
            // if the size is not 0, it was a valid code point
            auto utf8_char = PEGTL_NS::internal::peek_utf8::peek(in);
            if (utf8_char.size != 0)
            {
                // valid utf8_char, data has the code point
                if (TfIsUtf8CodePointXidStart(static_cast<uint32_t>(utf8_char.data)))
                {
                    // it has the property we want, consume the input
                    in.bump(utf8_char.size);
                    return true;
                }
            }
        }

        return false;
    }
};

struct XidContinue
{
    template <typename ParseInput>
    static bool match(ParseInput& in)
    {
        if (!in.empty())
        {
            // peek at the next character in the input
            // if the size is not 0, it was a valid code point
            auto utf8_char = PEGTL_NS::internal::peek_utf8::peek(in);
            if (utf8_char.size != 0)
            {
                // valid utf8_char, data has the code point
                if (TfIsUtf8CodePointXidContinue(static_cast<uint32_t>(utf8_char.data)))
                {
                    // it has the property we want, consume the input
                    in.bump(utf8_char.size);
                    return true;
                }
            }
        }

        return false;
    }
};

////////////////////////////////////////////////////////////////////////
// SdfPath grammar:

struct Slash  : PEGTL_NS::one<'/'> {};
struct Dot    : PEGTL_NS::one<'.'> {};
struct DotDot : PEGTL_NS::two<'.'> {};

struct DotDots : PEGTL_NS::list<DotDot, Slash> {};

// Valid identifiers start with '_' or XidStart, then zero or more XidContinue.
struct Utf8IdentifierStart :
        PEGTL_NS::sor<PEGTL_NS::one<'_'>, XidStart> {};
struct Utf8Identifier :
        PEGTL_NS::seq<Utf8IdentifierStart, PEGTL_NS::star<XidContinue>> {};

struct PrimName : Utf8Identifier {};

// XXX This replicates old behavior where '-' chars are allowed in variant set
// names in SdfPaths, but variant sets in layers cannot have '-' in their names.
// For now we preserve the behavior. Internal bug USD-8321 tracks removing
// support for '-' characters in variant set names in SdfPath.
struct VariantSetName : PEGTL_NS::seq<
    Utf8IdentifierStart,
    PEGTL_NS::star<PEGTL_NS::sor<XidContinue, PEGTL_NS::one<'-'>>>> {};

struct VariantName : PEGTL_NS::seq<
    PEGTL_NS::opt<PEGTL_NS::one<'.'>>,
    PEGTL_NS::star<PEGTL_NS::sor<XidContinue, PEGTL_NS::one<'|', '-'>>>> {};

struct VarSelOpen  : PEGTL_NS::pad<PEGTL_NS::one<'{'>, PEGTL_NS::blank> {};
struct VarSelClose : PEGTL_NS::pad<PEGTL_NS::one<'}'>, PEGTL_NS::blank> {};

// Named so we can attach a targeted error_message.
struct VariantSelEq : PEGTL_NS::pad<PEGTL_NS::one<'='>, PEGTL_NS::blank> {};

struct VariantSelection : PEGTL_NS::if_must<
    VarSelOpen,
    VariantSetName, VariantSelEq, PEGTL_NS::opt<VariantName>,
    VarSelClose> {};

struct VariantSelections : PEGTL_NS::plus<VariantSelection> {};

// '/' and commit to requiring a following prim name.
struct PrimStep : PEGTL_NS::if_must<Slash, PrimName> {};

// Variant selection used as a name separator (foo{vs=sel}bar, no slash).  The
// lookahead ensures we only enter this branch when a prim name immediately
// follows the selection, so trailing variant selections fall through to
// opt<VariantSelections> below.  Note: the lookahead parses
// VariantSelections+PrimName twice; acceptable given the bounded syntax
// involved.
struct VsStep : PEGTL_NS::seq<
    PEGTL_NS::at<PEGTL_NS::seq<VariantSelections, PrimName>>,
    VariantSelections,
    PrimName> {};

struct PrimElts : PEGTL_NS::seq<
    PrimName,
    PEGTL_NS::star<PEGTL_NS::sor<VsStep, PrimStep>>,
    PEGTL_NS::opt<VariantSelections>> {};

// PropertyName broken into a step rule so ':' commits to requiring an
// identifier after it, enabling a targeted error_message on PropertyNamePart.
struct PropertyNamePart : Utf8Identifier {};
struct PropertyNameSep :
        PEGTL_NS::if_must<PEGTL_NS::one<':'>, PropertyNamePart> {};
struct PropertyName :
        PEGTL_NS::seq<Utf8Identifier, PEGTL_NS::star<PropertyNameSep>> {};

struct MapperPath;
struct TargetPath;

struct TargetPathOpen  : PEGTL_NS::one<'['> {};
struct TargetPathClose : PEGTL_NS::one<']'> {};

template <class TargPath>
struct BracketPath : PEGTL_NS::if_must<
    TargetPathOpen, TargPath, TargetPathClose> {};

struct RelationalAttributeName : PropertyName {};

struct MapperKW  : PXR_PEGTL_KEYWORD("mapper") {};
struct MapperArg : PEGTL_NS::identifier {};

struct ExpressionKW : PXR_PEGTL_KEYWORD("expression") {};

struct Mapper : PEGTL_NS::seq<
    BracketPath<MapperPath>, PEGTL_NS::opt<PEGTL_NS::if_must<Dot, MapperArg>>
    > {};

struct MapperOrExpression : PEGTL_NS::sor<
    ExpressionKW,
    PEGTL_NS::if_must<MapperKW, Mapper>
    > {};

// '.' commits to requiring a property name.
struct RelAttrSeq : PEGTL_NS::if_must<
    Dot, RelationalAttributeName,
    PEGTL_NS::opt<
        PEGTL_NS::sor<
            BracketPath<TargetPath>,
            PEGTL_NS::if_must<Dot, MapperOrExpression>
            >
        >
    > {};

// '.' commits to requiring a property name.
struct PropElts : PEGTL_NS::if_must<
    Dot, PropertyName,
    PEGTL_NS::opt<
        PEGTL_NS::sor<
            PEGTL_NS::seq<BracketPath<TargetPath>, PEGTL_NS::opt<RelAttrSeq>>,
            PEGTL_NS::if_must<Dot, MapperOrExpression>
            >
        >
    > {};

struct PathElts :
    PEGTL_NS::if_then_else<PrimElts, PEGTL_NS::opt<PropElts>, PropElts> {};

struct PrimFirstPathElts :
    PEGTL_NS::seq<PrimElts, PEGTL_NS::opt<PropElts>> {};

struct AbsoluteRoot      : Slash {};
struct ReflexiveRelative : PEGTL_NS::seq<Dot, PEGTL_NS::eolf> {};

struct Path : PEGTL_NS::sor<
    PEGTL_NS::seq<AbsoluteRoot, PEGTL_NS::opt<PrimFirstPathElts>>,
    PEGTL_NS::seq<DotDots, PEGTL_NS::opt<PEGTL_NS::if_must<Slash, PathElts>>>,
    ReflexiveRelative,
    PathElts
    > {};

struct TargetPath : Path {};
struct MapperPath : Path {};

////////////////////////////////////////////////////////////////////////
// Actions.

struct PPContext {
    std::vector<SdfPath> paths { 1 };
    enum { IsTargetPath, IsMapperPath } targetType;
    std::string varSetName;
    std::string varName;
};

template <class Input>
TfToken GetToken(Input const &in) {
    constexpr int BufSz = 32;
    char buf[BufSz];
    size_t strSize = std::distance(in.begin(), in.end());
    TfToken tok;
    if (strSize < BufSz) {
        // copy & null-terminate.
        std::copy(in.begin(), in.end(), buf);
        buf[strSize] = '\0';
        tok = TfToken(buf);
    }
    else {
        // fall back to string path.
        tok = TfToken(in.string());
    }
    return tok;
}

template <class Rule>
struct Action : PEGTL_NS::nothing<Rule> {};

} // namespace Sdf_PathParser

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_SDF_PATH_PARSER_H
