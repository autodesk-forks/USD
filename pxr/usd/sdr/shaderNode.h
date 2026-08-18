//
// Copyright 2018 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#ifndef PXR_USD_SDR_SHADER_NODE_H
#define PXR_USD_SDR_SHADER_NODE_H

/// \file sdr/shaderNode.h

#include "pxr/pxr.h"
#include "pxr/usd/sdr/api.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/vt/value.h"
#include "pxr/usd/sdr/declare.h"
#include "pxr/usd/sdr/shaderNodeMetadata.h"
#include "pxr/usd/sdr/shaderNodeDiscoveryResult.h"

#include <unordered_map>

PXR_NAMESPACE_OPEN_SCOPE

#define SDR_NODE_FIELD_KEY_TOKENS                  \
    ((Identifier, "_identifier"))                  \
    ((Name, "_name"))                              \
    ((Family, "_family")) /* deprecated */         \
    ((Function, "_function"))                      \
    ((SourceType, "_sourceType")) /* deprecated */ \
    ((ShadingSystem, "_shadingSystem"))

/// \deprecated
/// SdrNodeFieldKey->SourceType is deprecated in favor of
/// SdrNodeFieldKey->ShadingSystem.
TF_DECLARE_PUBLIC_TOKENS(SdrNodeFieldKey, SDR_API, SDR_NODE_FIELD_KEY_TOKENS);

/// \class SdrShaderNode
///
/// Represents a node that holds shading information. Describes information
/// like the name of the node, what its inputs and outputs are, and any
/// associated metadata.
class SdrShaderNode
{
public:

    /// \name Constructors
    ///
    /// Constructors for SdrShaderNode
    ///
    /// \deprecated
    /// The versions of the SdrShaderNode constructors that
    /// take `SdrTokenMap` as metadata are deprecated in favor of
    /// the constructor taking an explicit SdrShaderNodeMetadata. Note that
    /// SdrShaderNodeMetadata has an implicit constructor from
    /// the legacy `SdrTokenMap` metadata.
    /// \sa SdrShaderNodeMetadata::SdrShaderNodeMetadata(const SdrTokenMap&)
    /// {@

    /// Constructor
    SDR_API
    SdrShaderNode(
        const SdrIdentifier& identifier,
        const SdrVersion& version,
        const std::string& name,
        const TfToken& function,
        const TfToken& shadingSystem,
        const std::string& definitionURI,
        const std::string& implementationURI,
        SdrShaderPropertyUniquePtrVec&& properties,
        const SdrShaderNodeMetadata& metadata = SdrShaderNodeMetadata(),
        const std::string &sourceCode = std::string()
    ): SdrShaderNode(
        identifier,
        version,
        name,
        function,
        metadata.GetContext(),
        shadingSystem,
        definitionURI,
        implementationURI,
        std::move(properties),
        metadata,
        sourceCode
    ) {}

    /// \deprecated
    /// Deprecated in favor of the above constructor that expects the "context"
    /// concept to be specified in metadata instead of as a direct constructor
    /// argument.
    SDR_API
    SdrShaderNode(
        const SdrIdentifier& identifier,
        const SdrVersion& version,
        const std::string& name,
        const TfToken& function,
        const TfToken& context, // deprecated
        const TfToken& shadingSystem,
        const std::string& definitionURI,
        const std::string& implementationURI,
        SdrShaderPropertyUniquePtrVec&& properties,
        const SdrShaderNodeMetadata& metadata = SdrShaderNodeMetadata(),
        const std::string &sourceCode = std::string());
    /// @}

    /// Destructor.
    SDR_API
    virtual ~SdrShaderNode();

    /// \name The Basics
    /// @{

    /// Return the identifier of the node.
    const SdrIdentifier& GetIdentifier() const { return _identifier; }

    /// Return the version of the node
    SdrVersion GetShaderVersion() const { return _version; }

    /// Gets the name of the node.
    const std::string& GetName() const { return _name; }

    /// Gets the name of the family that the node belongs to. An empty token
    /// will be returned if the node does not belong to a family.
    ///
    /// \deprecated in favor of SdrShaderNode::GetFunction
    const TfToken& GetFamily() const { return _function; }

    /// Gets the function that the node belongs to.
    ///
    /// One example of a function would be "add" -- multiple
    /// specializations on argument types and version may be registered
    /// under the same function but on different nodes.
    const TfToken& GetFunction() const { return _function; }

    /// Gets the shading system that this shader node originated from.
    ///
    /// "Shading system" describes a system that usually has its own
    /// standard and shading language
    ///
    /// As a concrete example from the `Sdr` library, several shader parsers
    /// exist and operate on shaders from different shading systems. In this
    /// scenario, each distinct type of shader (OSL, Args, etc) is considered
    /// a different _shading system_, even though they are all shaders.
    const TfToken& GetShadingSystem() const { return _shadingSystem; }

    /// Gets the type of source that this shader node originated from.
    ///
    /// \deprecated
    /// Deprecated in favor of `GetShadingSystem`
    const TfToken& GetSourceType() const { return _shadingSystem; }

    /// Gets the URI to the resource that provided this node's
    /// definition. Could be a path to a file, or some other resource
    /// identifier. This URI should be fully resolved.
    ///
    /// \sa SdrShaderNode::GetResolvedImplementationURI()
    const std::string& GetResolvedDefinitionURI() const { return _definitionURI; }

    /// Gets the URI to the resource that provides this node's
    /// implementation. Could be a path to a file, or some other resource
    /// identifier. This URI should be fully resolved.
    ///
    /// \sa SdrShaderNode::GetResolvedDefinitionURI()
    const std::string& GetResolvedImplementationURI() const {
        return _implementationURI;
    }

    /// Returns  the source code for this node. This will be empty for most 
    /// nodes. It will be non-empty only for the nodes that are constructed 
    /// using \ref SdrRegistry::GetShaderNodeFromSourceCode(), in which case, the 
    /// source code has not been parsed (or even compiled) yet. 
    /// 
    /// An unparsed node with non-empty source-code but no properties is 
    /// considered to be invalid. Once the node is parsed and the relevant 
    /// properties and metadata are extracted from the source code, the node 
    /// becomes valid.
    /// 
    /// \sa SdrShaderNode::IsValid
    const std::string &GetSourceCode() const { return _sourceCode; }

    /// Whether or not this node is valid. A node that is valid indicates that
    /// the parser plugin was able to successfully parse the contents of this
    /// node.
    ///
    /// Note that if a node is not valid, some data like its name, URI, source 
    /// code etc. could still be available (data that was obtained during the 
    /// discovery process). However, other data that must be gathered from the 
    /// parsing process will NOT be available (eg, inputs and outputs).
    SDR_API
    bool IsValid() const { return _isValid; }

    /// Gets a string with basic information about this node. Helpful for
    /// things like adding this node to a log.
    SDR_API
    std::string GetInfoString() const;

    /// @}

    /// \name Inputs and Outputs
    /// An input or output is also generically referred to as a "property".
    /// @{

    /// Get an ordered list of all the input names on this shader node.
    SDR_API
    const SdrTokenVec& GetShaderInputNames() const;

    /// Get an ordered list of all the output names on this shader node.
    SDR_API
    const SdrTokenVec& GetShaderOutputNames() const;

    /// Get a shader input property by name. `nullptr` is returned if an input
    /// with the given name does not exist.
    SDR_API
    SdrShaderPropertyConstPtr GetShaderInput(const TfToken& inputName) const;

    /// Get a shader output property by name. `nullptr` is returned if an output
    /// with the given name does not exist.
    SDR_API
    SdrShaderPropertyConstPtr GetShaderOutput(const TfToken& outputName) const;

    /// Returns the list of all inputs that are tagged as asset identifier 
    /// inputs.
    SDR_API
    SdrTokenVec GetAssetIdentifierInputNames() const;

    /// Returns the first shader input that is tagged as the default input.
    /// A default input and its value can be used to acquire a fallback value
    /// for a node when the node is considered 'disabled' or otherwise
    /// incapable of producing an output value.
    SDR_API
    SdrShaderPropertyConstPtr GetDefaultInput() const;

    /// @}


    /// \name Metadata
    /// The metadata returned here is a direct result of what the parser plugin
    /// is able to determine about the node. See the documentation for a
    /// specific parser plugin to get help on what the parser is looking for to
    /// populate these values.
    /// @{

    /// All metadata that came from the parse process.
    ///
    /// \deprecated in favor of GetMetadataObject
    ///
    /// If this node was initialized with the upgraded SdrShaderNodeMetadata
    /// class, the legacy metadata returned by this function will contain only
    /// string representations of named metadata, and any other metadata items
    /// whose value holds a string type in SdrShaderNodeMetadata. Other metadata
    /// items may be omitted.
    SDR_API
    const SdrTokenMap& GetMetadata() const;

    /// All metadata that came from the parse process.
    SDR_API
    const SdrShaderNodeMetadata& GetMetadataObject() const;

    /// Gets the domain of the node.
    ///
    /// \sa SdrShaderNodeMetadata::GetDomain
    const TfToken& GetDomain() const { return _domain; }

    /// Gets the subdomain of the node if the subdomain metadata item exists.
    ///
    /// If the subdomain metadata doesn't exist, returns an empty TfToken.
    ///
    /// \sa SdrShaderNodeMetadata::GetSubdomain
    const TfToken& GetSubdomain() const { return _subdomain; }

    /// Gets the context of the node.
    ///
    /// Context describes a node's usage group within its subdomain.
    /// \sa SdrShaderNodeMetadata::GetContext.
    ///
    /// If the context metadatda doesn't exist, returns an empty TfToken.
    const TfToken& GetContext() const { return _context; }

    /// Returns the role of this node.
    ///
    /// Role provides finer granularity for contexts that contain many nodes.
    /// \sa SdrShaderNodeMetadata::GetRole
    ///
    /// If no role exists, returns the name of this node.
    SDR_API
    TfToken GetRole() const;

    /// The label assigned to this node, if any. Distinct from the name
    /// returned from `GetName()`. In the context of a UI, the label value
    /// might be used as the display name for the node instead of the name.
    ///
    /// Returns an empty TfToken if no label is present.
    SDR_API
    const TfToken& GetLabel() const { return _label; }

    /// The category assigned to this node, if any. Distinct from the family
    /// returned from `GetFamily()`.
    ///
    /// \deprecated
    ///
    /// Returns an empty TfToken if no category is present.
    SDR_API
    const TfToken& GetCategory() const { return _category; }

    /// The help message assigned to this node, if any.
    ///
    /// Returns an empty string if no help message is present.
    SDR_API
    std::string GetHelp() const;

    /// The departments this node is associated with, if any.
    ///
    /// \deprecated
    SDR_API
    const SdrTokenVec& GetDepartments() const { return _departments; }

    /// Gets the pages which should be opened or expanded by default.
    SDR_API
    const SdrTokenVec& GetOpenPages() const { return _openPages; }

    /// Gets the `shownIf` expressions associated with each page.
    SDR_API
    const SdrTokenMap& GetPagesShownIf() const { return _pagesShownIf; }

    /// The list of primvars this node knows it requires / uses.
    /// For example, a shader node may require the 'normals' primvar to function
    /// correctly. Additional, user specified primvars may have been authored on
    /// the node. These can be queried via `GetAdditionalPrimvarProperties()`.
    /// Together, `GetPrimvars()` and `GetAdditionalPrimvarProperties()`,
    /// provide the complete list of primvar requirements for the node.
    ///
    /// Note that SdrShaderNode::GetPrimvars isn't equivalent to
    /// SdrShaderNodeMetadata::GetPrimvars. The former returns primvars
    /// processed with access to SdrShaderProperty information.
    SDR_API
    const SdrTokenVec& GetPrimvars() const { return _primvars; }

    /// The list of string input properties whose values provide the names of
    /// additional primvars consumed by this node. For example, this may return
    /// a token named `varname`. This indicates that the client should query the
    /// value of a (presumed to be string-valued) input attribute named varname
    /// from its scene description to determine the name of a primvar the 
    /// node will consume. See `GetPrimvars()` for additional information.
    SDR_API
    const SdrTokenVec& GetAdditionalPrimvarProperties() const {
        return _primvarNamingProperties;
    }

    /// Returns the implementation name of this node.  The name of the node
    /// is how to refer to the node in shader networks.  The label is how to
    /// present this node to users.  The implementation name is the name of
    /// the function (or something) this node represents in the
    /// implementation.  Any client using the implementation \b must call
    /// this method to get the correct name;  using \c getName() is not
    /// correct.
    SDR_API
    std::string GetImplementationName() const;

    /// @}


    /// \name Aggregate Information
    /// @{

    /// Gets the pages on which the node's properties reside (an aggregate of
    /// the unique `SdrShaderProperty::GetPage()` values for all of the node's
    /// properties). Nodes themselves do not reside on pages. In an example
    /// scenario, properties might be divided into two pages, 'Simple' and
    /// 'Advanced'.
    SDR_API
    const SdrTokenVec& GetPages() const { return _pages; }

    /// Gets the names of the properties on a certain page (one that was
    /// returned by `GetPages()`). To get properties that are not assigned to a
    /// page, an empty string can be used for \p pageName.
    SDR_API
    SdrTokenVec GetPropertyNamesForPage(const std::string& pageName) const;

    /// Gets all vstructs that are present in the shader.
    SDR_API
    SdrTokenVec GetAllVstructNames() const;

    /// @}

    // Stores the result of the compliance check of property names to
    // sdrShaderNodeIdentifiers
    using ComplianceResults = std::unordered_map<TfToken,
                              std::vector<SdrIdentifier>, 
                              TfToken::HashFunctor>;

    /// This method checks if same named properties of \p shaderNodes are
    /// compatible with each other.
    ///
    /// Checks if the same name properties have matching types and default
    /// values. In order to determine if same name properties from different
    /// shader nodes are compliant, we assume that the first shaderNode in the 
    /// list providing this property is authoritative, and other nodes differing
    /// wrt this property are non-compliant. A map of property names and their 
    /// respective shaderNodes are stored in the map. An empty map returned 
    /// represents no compliance issues.
    SDR_API
    static
    ComplianceResults CheckPropertyCompliance(
        const std::vector<SdrShaderNodeConstPtr> &shaderNodes);

    /// \cond
    /// Hide from the API.

    // Performs a post-process on properties to determine information that can
    // only be determined after parsing or in aggregate. Clients SHOULD NOT
    // need to call this.
    void _PostProcessProperties();

    /// \endcond

    /// Gets an item of data from this shader node according to the
    /// requested key.
    
    /// Special keys indicate class fields and are identified by the
    /// SdrNodeFieldKey enum. The return type of GetDataForKey for these
    /// special keys is the return type of their getters.
    /// GetDataForKey(SdrNodeFieldKey->Identifier) results in VtValue
    /// holding a TfToken because GetIdentifier returns TfToken. The
    /// return types for these special keys is as follows:
    /// - SdrNodeFieldKey->Identifier -> VtValue holding TfToken
    /// - SdrNodeFieldKey->Name -> VtValue holding std::string
    /// - SdrNodeFieldKey->Function -> VtValue holding TfToken
    /// - SdrNodeFieldKey->ShadingSystem -> VtValue holding TfToken
    /// - (deprecated) SdrNodeFieldKey->SourceType -> VtValue holding TfToken
    ///
    /// Any requested data item that isn't a SdrNodeFieldKey will be looked
    /// for in this shader node's metadata.
    ///
    /// Returns an empty VtValue if no data is found to be associated with the
    /// requested key.
    SDR_API
    VtValue GetDataForKey(const TfToken& key) const;

protected:
    SdrShaderNode& operator=(const SdrShaderNode&) = delete;

    bool _isValid;
    SdrIdentifier _identifier;
    SdrVersion _version;
    std::string _name;
    TfToken _function;
    TfToken _context;
    TfToken _shadingSystem;
    std::string _definitionURI;
    std::string _implementationURI;
    SdrShaderPropertyUniquePtrVec _properties;
    SdrTokenMap _legacyMetadata;
    SdrShaderNodeMetadata _metadata;
    std::string _sourceCode;

    SdrShaderPropertyMap _inputs;
    SdrTokenVec _inputNames;
    SdrShaderPropertyMap _outputs;
    SdrTokenVec _outputNames;

    // Stored metadata to support getter API on SdrShaderNode that
    // returns const ref.
    TfToken _domain;
    TfToken _subdomain;
    TfToken _label;
    TfToken _category;
    SdrTokenVec _departments;
    SdrTokenVec _openPages;
    SdrTokenMap _pagesShownIf;

    // Processed primvar metadata. `_primvars` contains the names of primvars
    // consumed by this node, whereas `_primvarNamingProperties` contains the
    // names of string input properties whose values provide the names of
    // additional primvars consumed by this node.
    SdrTokenVec _primvars;
    SdrTokenVec _primvarNamingProperties;

    // Aggregated pages data, from examining SdrShaderProperty's page metadata
    SdrTokenVec _pages;

private:
    // Initializes `_primvars` and `_primvarNamingProperties`
    void _InitializePrimvars();

    // Determines which pages are present on the node's properties
    SdrTokenVec _ComputePages() const;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_SDR_SHADER_NODE_H
