//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.

#include "pxr/pxr.h"
#include "pxr/usd/sdf/pathParser.h"

PXR_NAMESPACE_OPEN_SCOPE

using namespace PXR_PEGTL_NAMESPACE;

namespace Sdf_PathParser {

// Wrap eolf so we can attach an error message without specializing the PEGTL
// built-in directly.
struct PathEnd : PEGTL_NS::eolf {};

////////////////////////////////////////////////////////////////////////
// Actions

template <>
struct Action<ReflexiveRelative> {
    template <class Input>
    static void apply(Input const &in, PPContext &pp) {
        pp.paths.back() = SdfPath::ReflexiveRelativePath();
    }
};

template <>
struct Action<AbsoluteRoot> {
    template <class Input>
    static void apply(Input const &in, PPContext &pp) {
        pp.paths.back() = SdfPath::AbsoluteRootPath();
    }
};

template <>
struct Action<DotDot> {
    template <class Input>
    static void apply(Input const &in, PPContext &pp) {
        if (pp.paths.back().IsEmpty()) {
            pp.paths.back() = SdfPath::ReflexiveRelativePath();
        }
        pp.paths.back() = pp.paths.back().GetParentPath();
    }
};

template <>
struct Action<PrimName> {
    template <class Input>
    static void apply(Input const &in, PPContext &pp) {
        if (pp.paths.back().IsEmpty()) {
            pp.paths.back() = SdfPath::ReflexiveRelativePath();
        }
        pp.paths.back() = pp.paths.back().AppendChild(GetToken(in));
    }
};

template <>
struct Action<VariantSetName> {
    template <class Input>
    static void apply(Input const &in, PPContext &pp) {
        pp.varSetName = in.string();
    }
};

template <>
struct Action<VariantName> {
    template <class Input>
    static void apply(Input const &in, PPContext &pp) {
        pp.varName = in.string();
    }
};

template <>
struct Action<VariantSelection> {
    template <class Input>
    static void apply(Input const &in, PPContext &pp) {
        pp.paths.back() =
            pp.paths.back().AppendVariantSelection(pp.varSetName, pp.varName);
        pp.varSetName.clear();
        pp.varName.clear();
    }
};

template <>
struct Action<PropertyName> {
    template <class Input>
    static void apply(Input const &in, PPContext &pp) {
        if (pp.paths.back().IsEmpty()) {
            pp.paths.back() = SdfPath::ReflexiveRelativePath();
        }
        pp.paths.back() = pp.paths.back().AppendProperty(GetToken(in));
    }
};

template <>
struct Action<RelationalAttributeName> {
    template <class Input>
    static void apply(Input const &in, PPContext &pp) {
        pp.paths.back() =
            pp.paths.back().AppendRelationalAttribute(GetToken(in));
    }
};

template <>
struct Action<TargetPathOpen> {
    template <class Input>
    static void apply(Input const &in, PPContext &pp) {
        pp.paths.emplace_back();
    }
};

template <>
struct Action<TargetPath> {
    template <class Input>
    static void apply(Input const &in, PPContext &pp) {
        pp.targetType = PPContext::IsTargetPath;
    }
};

template <>
struct Action<MapperPath> {
    template <class Input>
    static void apply(Input const &in, PPContext &pp) {
        pp.targetType = PPContext::IsMapperPath;
    }
};

template <>
struct Action<MapperArg> {
    template <class Input>
    static void apply(Input const &in, PPContext &pp) {
        pp.paths.back() = pp.paths.back().AppendMapperArg(GetToken(in));
    }
};

template <>
struct Action<TargetPathClose> {
    template <class Input>
    static void apply(Input const &in, PPContext &pp) {
        SdfPath targetPath = std::move(pp.paths.back());
        pp.paths.pop_back();
        if (pp.targetType == PPContext::IsTargetPath) {
            pp.paths.back() =
                pp.paths.back().AppendTarget(std::move(targetPath));
        }
        else {
            pp.paths.back() =
                pp.paths.back().AppendMapper(std::move(targetPath));
        }
    }
};

template <>
struct Action<ExpressionKW> {
    template <class Input>
    static void apply(Input const &in, PPContext &pp) {
        pp.paths.back() = pp.paths.back().AppendExpression();
    }
};

////////////////////////////////////////////////////////////////////////
// Error messages

template <class Rule>
static char const *ErrorMessage = "";

#define PARSE_ERROR(rule, msg)                                  \
    template <> char const *ErrorMessage<rule> = msg;

PARSE_ERROR(PathEnd,            "expected end of path");
PARSE_ERROR(Path,               "expected a path beginning with '/', '.', "
                                "or a prim name");
PARSE_ERROR(PrimName,           "expected prim name");
PARSE_ERROR(PropertyName,       "expected property name after '.'");
PARSE_ERROR(PropertyNamePart,   "expected identifier after ':'");
PARSE_ERROR(VariantSetName,     "expected variant set name after '{'");
PARSE_ERROR(VariantSelEq,       "expected '=' after variant set name");
PARSE_ERROR(VarSelClose,        "expected variant set name or '}' to "
                                "close variant selection");
PARSE_ERROR(TargetPath,         "expected target or connection path in []");
PARSE_ERROR(MapperPath,         "expected connection path in [] for mapper");
PARSE_ERROR(TargetPathClose,    "expected ']' to close target path");
PARSE_ERROR(Mapper,             "expected target path after 'mapper'");
PARSE_ERROR(MapperOrExpression, "expected 'mapper' or 'expression' after '.'");
PARSE_ERROR(MapperArg,          "expected mapper argument name after '.'");

#undef PARSE_ERROR

template <class Rule>
struct Errors : public normal<Rule>
{
    template <class Input, class... States>
    [[noreturn]] static void raise(Input const &in, States &&...) {
        throw parse_error(ErrorMessage<Rule>, in);
    }
};

} // namespace Sdf_PathParser

bool
Sdf_ParsePath(std::string const &pathStr, SdfPath *path, std::string *errMsg)
{
    Sdf_PathParser::PPContext context;
    try {
        parse<must<Sdf_PathParser::Path, Sdf_PathParser::PathEnd>,
              Sdf_PathParser::Action, Sdf_PathParser::Errors>(
                  string_input<> { pathStr, "" }, context);
        if (path) {
            *path = std::move(context.paths.back());
        }
        return true;
    }
    catch (parse_error const &err) {
        if (errMsg) {
            std::string location;
            auto const &positions = err.positions();
            if (!positions.empty()) {
                // column is 1-based
                const size_t col = positions.front().column;
                const size_t idx = col - 1;
                if (col >= 1 && idx < pathStr.size()) {
                    location = TfStringPrintf(
                        " at character %zu ('%c')", col, pathStr[idx]);
                }
                else {
                    location = TfStringPrintf(" at character %zu", col);
                }
            }
            *errMsg = TfStringPrintf(
                "Ill-formed SdfPath <%s>%s: %s",
                pathStr.c_str(), location.c_str(),
                std::string(err.message()).c_str());
        }
        if (path) {
            *path = SdfPath();
        }
        return false;
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
