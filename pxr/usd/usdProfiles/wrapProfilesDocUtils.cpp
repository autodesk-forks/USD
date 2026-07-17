//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/usd/usdProfiles/profilesDocUtils.h"

#include "pxr/external/boost/python/def.hpp"
#include "pxr/external/boost/python/dict.hpp"
#include "pxr/external/boost/python/extract.hpp"
#include "pxr/external/boost/python/list.hpp"
#include "pxr/external/boost/python/object.hpp"
#include "pxr/external/boost/python/stl_iterator.hpp"

#include <map>
#include <string>

PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;

namespace {

std::map<std::string, std::string>
_DictToMap(const dict& d)
{
    std::map<std::string, std::string> out;
    list keys = d.keys();
    const ssize_t n = len(keys);
    for (ssize_t i = 0; i < n; ++i) {
        object k = keys[i];
        object v = d[k];
        std::string ks = extract<std::string>(k);
        std::string vs = extract<std::string>(v);
        out.emplace(std::move(ks), std::move(vs));
    }
    return out;
}

std::string
_MarkdownNoArgs()
{
    return UsdProfilesMarkdown();
}

std::string
_MarkdownWithStyles(const dict& styles)
{
    return UsdProfilesMarkdown(_DictToMap(styles));
}

std::string
_Mermaid(const dict& styles)
{
    return UsdProfilesMermaid(_DictToMap(styles));
}

std::string
_Dot()
{
    return UsdProfilesDot();
}

} // anonymous namespace

void wrapUsdProfilesDocUtils()
{
    def("ProfilesMarkdown", &_MarkdownNoArgs);
    def("ProfilesMarkdown", &_MarkdownWithStyles, (arg("styles")));
    def("ProfilesMermaid",  &_Mermaid,            (arg("styles")));
    def("ProfilesDot",      &_Dot);
}
