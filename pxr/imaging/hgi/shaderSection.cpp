//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "shaderSection.h"

PXR_NAMESPACE_OPEN_SCOPE

HgiShaderSection::HgiShaderSection(
    const std::string &identifier,
    const HgiShaderSectionAttributeVector& attributes,
    const std::string &defaultValue,
    const std::string &arraySize,
    const std::string &blockInstanceIdentifier)
  : _identifierVar(identifier)
  , _attributes(attributes)
  , _defaultValue(defaultValue)
  , _arraySize(arraySize)
  , _blockInstanceIdentifier(blockInstanceIdentifier)
{
}

HgiShaderSection::~HgiShaderSection() = default;

void
HgiShaderSection::WriteType(std::ostream& ss) const
{
}

void
HgiShaderSection::WriteIdentifier(std::ostream& ss) const
{
    ss << _identifierVar;
}

void
HgiShaderSection::WriteBlockInstanceIdentifier(std::ostream& ss) const
{
    ss << _blockInstanceIdentifier;
}

void
HgiShaderSection::WriteDeclaration(std::ostream& ss) const
{
    WriteType(ss);
    ss << " ";
    WriteIdentifier(ss);
    WriteArraySize(ss);
    ss << ";";
}

void
HgiShaderSection::WriteParameter(std::ostream& ss) const
{
    WriteType(ss);
    ss << " ";
    WriteIdentifier(ss);
}

void
HgiShaderSection::WriteArraySize(std::ostream& ss) const
{
    if (!_arraySize.empty()) {
        ss << "[" << _arraySize << "]";
    }
}

const HgiShaderSectionAttributeVector&
HgiShaderSection::GetAttributes() const
{
    return _attributes;
}

const std::string&
HgiShaderSection::_GetDefaultValue() const
{
    return _defaultValue;
}

HgiBaseGLShaderSection::HgiBaseGLShaderSection(
        const std::string &identifier,
        const HgiShaderSectionAttributeVector &attributes,
        const std::string &storageQualifier,
        const std::string &defaultValue,
        const std::string &arraySize,
        const std::string &blockInstanceIdentifier)
        : HgiShaderSection(identifier, attributes, defaultValue,
                           arraySize, blockInstanceIdentifier)
        , _storageQualifier(storageQualifier)
        , _arraySize(arraySize)
{
}

HgiBaseGLShaderSection::~HgiBaseGLShaderSection() = default;

void
HgiBaseGLShaderSection::WriteDeclaration(std::ostream &ss) const
{
    //If it has attributes, write them with corresponding layout
    //identifiers and indicies
    const HgiShaderSectionAttributeVector &attributes = GetAttributes();

    if (!attributes.empty()) {
        ss << "layout(";
        for (size_t i = 0; i < attributes.size(); i++)
        {
            if (i > 0) {
                ss << ", ";
            }
            const HgiShaderSectionAttribute &a = attributes[i];
            ss << a.identifier;
            if(!a.index.empty()) {
                ss << " = " << a.index;
            }
        }
        ss << ") ";
    }
    if (!_storageQualifier.empty()) {
        ss << _storageQualifier << " ";
    }
    WriteType(ss);
    ss << " ";
    WriteIdentifier(ss);
    WriteArraySize(ss);
    ss << ";\n";
}

void
HgiBaseGLShaderSection::WriteParameter(std::ostream &ss) const
{
    WriteType(ss);
    ss << " ";
    WriteIdentifier(ss);
    ss << ";";
}

bool
HgiBaseGLShaderSection::VisitGlobalIncludes(std::ostream &ss)
{
    return false;
}

bool
HgiBaseGLShaderSection::VisitGlobalMacros(std::ostream &ss)
{
    return false;
}

bool
HgiBaseGLShaderSection::VisitGlobalStructs(std::ostream &ss)
{
    return false;
}

bool HgiBaseGLShaderSection::VisitGlobalMemberDeclarations(std::ostream &ss)
{
    return false;
}

bool
HgiBaseGLShaderSection::VisitGlobalFunctionDefinitions(std::ostream &ss)
{
    return false;
}

PXR_NAMESPACE_CLOSE_SCOPE
