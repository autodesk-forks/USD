//
// Copyright 2018 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/base/tf/error.h"
#include "pxr/base/tf/errorMark.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/usd/sdf/textParserUtils.h"
#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdr/shaderMetadataHelpers.h"
#include "pxr/usd/sdr/shaderProperty.h"

#include <algorithm>
#include <iostream>

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,

    // Values for "widget" metadata that indicate the property is an
    // asset identifier
    ((filename, "filename"))            // OSL spec
    ((fileInput, "fileInput"))          // Args spec
    ((assetIdInput, "assetIdInput"))    // Pixar convention

    // Values for "renderType" metadata that indicate the property is a
    // SdrPropertyTypes->Terminal
    ((terminal, "terminal"))
);

namespace ShaderMetadataHelpers
{
    bool
    IsTruthy(const TfToken& key, const SdrTokenMap& metadata)
    {
        const SdrTokenMap::const_iterator search = metadata.find(key);

        // Absence of the option implies false
        if (search == metadata.end()) {
            return false;
        }

        // Presence of the option without a value implies true
        if (search->second.empty()) {
            return true;
        }

        // Copy string for modification below
        std::string boolStr = search->second;

        // Turn into a lower case string
        std::transform(boolStr.begin(), boolStr.end(), boolStr.begin(), ::tolower);

        if ((boolStr == "0") || (boolStr == "false") || (boolStr == "f")) {
            return false;
        }

        return true;
    }


    // -------------------------------------------------------------------------


    std::string
    StringVal(const TfToken& key, const SdrTokenMap& metadata,
              const std::string& defaultValue)
    {
        const SdrTokenMap::const_iterator search = metadata.find(key);

        if (search != metadata.end()) {
            return search->second;
        }

        return defaultValue;
    }


    // -------------------------------------------------------------------------


    TfToken
    TokenVal(const TfToken& key, const SdrTokenMap& metadata,
             const TfToken& defaultValue)
    {
        const SdrTokenMap::const_iterator search = metadata.find(key);

        if (search != metadata.end()) {
            return TfToken(search->second);
        }

        return defaultValue;
    }


    // -------------------------------------------------------------------------


    int
    IntVal(const TfToken& key, const SdrTokenMap& metadata,
           int defaultValue)
    {
        const SdrTokenMap::const_iterator search = metadata.find(key);

        if (search == metadata.end()) {
            return defaultValue;
        }

        try {
            return std::stoi(search->second);
        } catch (...) {
            return defaultValue;
        }
    }


    // -------------------------------------------------------------------------


    SdrStringVec
    StringVecVal(const TfToken& key, const SdrTokenMap& metadata)
    {
        const SdrTokenMap::const_iterator search = metadata.find(key);

        if (search != metadata.end()) {
            return TfStringSplit(search->second, "|");
        }

        return SdrStringVec();
    }


    // -------------------------------------------------------------------------


    SdrTokenVec
    TokenVecVal(const TfToken& key, const SdrTokenMap& metadata)
    {
        const SdrStringVec untokenized = StringVecVal(key, metadata);
        SdrTokenVec tokenized;

        for (const std::string& item : untokenized) {
            tokenized.emplace_back(TfToken(item));
        }

        return tokenized;
    }


    // -------------------------------------------------------------------------


    SdrOptionVec
    OptionVecVal(const std::string& optionStr)
    {
        std::vector<std::string> tokens = TfStringSplit(optionStr, "|");

        // The input string should be formatted as one of the following:
        //
        //     list:   "option1|option2|option3|..."
        //     mapper: "key1:value1|key2:value2|..."
        //
        // If it's a mapper, return the result as a list of key-value tuples to
        // preserve order.

        SdrOptionVec options;

        for (const std::string& token : tokens) {
            size_t colonPos = token.find(':');

            if (colonPos != std::string::npos) {
                options.emplace_back(std::make_pair(
                    TfToken(token.substr(0, colonPos)),
                    TfToken(token.substr(colonPos + 1)))
                );
            } else {
                options.emplace_back(std::make_pair(
                    TfToken(token),
                    TfToken())
                );
            }
        }

        return options;
    }


    // -------------------------------------------------------------------------


    std::string
    CreateStringFromStringVec(const SdrStringVec& stringVec)
    {
        return TfStringJoin(stringVec, "|");
    }


    // -------------------------------------------------------------------------


    bool
    IsPropertyAnAssetIdentifier(const SdrTokenMap& metadata)
    {
        const SdrTokenMap::const_iterator widgetSearch =
            metadata.find(SdrPropertyMetadata->Widget);

        if (widgetSearch != metadata.end()) {
            const TfToken widget = TfToken(widgetSearch->second);

            if ((widget == _tokens->assetIdInput) ||
                (widget == _tokens->filename) ||
                (widget == _tokens->fileInput)) {
                return true;
            }
        }

        return false;
    }

    // -------------------------------------------------------------------------

    bool
    IsPropertyATerminal(const SdrTokenMap& metadata)
    {
        const SdrTokenMap::const_iterator renderTypeSearch =
            metadata.find(SdrPropertyMetadata->RenderType);

        if (renderTypeSearch != metadata.end()) {
            // If the property is a SdrPropertyTypes->Terminal, then the
            // renderType value will be "terminal <terminalName>", where the
            // <terminalName> is the specific kind of terminal.  To identify
            // the property as a terminal, we only need to check that the first
            // string in the renderType value specifies "terminal"
            if (TfStringStartsWith(
                renderTypeSearch->second, _tokens->terminal)) {
                return true;
            }
        }

        return false;
    }

    // -------------------------------------------------------------------------

    TfToken
    GetRoleFromMetadata(const SdrTokenMap& metadata)
    {
        const SdrTokenMap::const_iterator roleSearch =
            metadata.find(SdrPropertyMetadata->Role);

        if (roleSearch != metadata.end()) {
            // If the value found is an allowed value, then we can return it
            const TfToken role = TfToken(roleSearch->second);
            if (std::find(SdrPropertyRole->allTokens.begin(),
                          SdrPropertyRole->allTokens.end(),
                          role) != SdrPropertyRole->allTokens.end()) {
                return role;
            }
        }
        // Return an empty token if no "role" metadata or acceptable value found
        return TfToken();
    }

    // -------------------------------------------------------------------------

    VtValue
    ParseSdfValue(const std::string& valueStr,
                  const SdrShaderPropertyConstPtr& property,
                  std::string* err)
    {
        const SdrSdfTypeIndicator indicator = property->GetTypeAsSdfType();
        const TfToken sdrType = property->GetType();
        const SdfValueTypeName sdfType = indicator.GetSdfType();

        std::string normalizedStr = valueStr;
        if (property->IsDynamicArray() ||
                sdrType == SdrPropertyTypes->Vector) {
            normalizedStr = TfStringTrim(normalizedStr);
            normalizedStr = '[' + normalizedStr + ']';
        } else if (property->IsArray() ||
                   sdrType == SdrPropertyTypes->Color  ||
                   sdrType == SdrPropertyTypes->Color4 ||
                   sdrType == SdrPropertyTypes->Point  ||
                   sdrType == SdrPropertyTypes->Normal) {
            normalizedStr = TfStringTrim(normalizedStr);
            normalizedStr = '(' + normalizedStr + ')';
        } else if (sdfType == SdfValueTypeNames->String ||
                   sdfType == SdfValueTypeNames->Token) {
            normalizedStr = Sdf_QuoteString(normalizedStr);
        } else if (sdfType == SdfValueTypeNames->Asset) {
            if (normalizedStr.empty()) {
                *err = "Attempted parse of invalid empty asset path";
                return VtValue();
            }
            normalizedStr = Sdf_QuoteAssetPath(normalizedStr);
        } else {
            normalizedStr = TfStringTrim(normalizedStr);
        }

        // We transform any TfErrors into messages that are concatenated to the
        // provided err output, so that Python users don't need to deal with
        // a thrown exception.
        TfErrorMark m;
        VtValue outputValue = Sdf_ParseValueFromString(normalizedStr, sdfType);
        if (!m.IsClean()) {
            for (TfError const &tfErr: m) {
                if (err->empty()) {
                    *err = tfErr.GetCommentary();
                } else {
                    *err = TfStringPrintf("%s; %s", err->c_str(),
                                          tfErr.GetCommentary().c_str());
                }
            }
            m.Clear();
        }
        return outputValue;
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
