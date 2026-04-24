//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#ifndef PXR_USD_SDR_NODE_DISCOVERY_RESULT_H
#define PXR_USD_SDR_NODE_DISCOVERY_RESULT_H

/// \file sdr/shaderNodeDiscoveryResult.h

#include "pxr/usd/sdr/declare.h"

PXR_NAMESPACE_OPEN_SCOPE

/// Represents the raw data of a node, and some other bits of metadata, that
/// were determined via a `SdrDiscoveryPlugin`.
struct SdrShaderNodeDiscoveryResult {
    /// Constructor.
    SdrShaderNodeDiscoveryResult(
        const SdrIdentifier& identifier,
        const SdrVersion& version,
        const std::string& name,
        const TfToken& function,
        const TfToken& discoveryType,
        const TfToken& shadingSystem,
        const std::string& uri,
        const std::string& resolvedUri,
        const std::string &sourceCode=std::string(),
        const SdrTokenMap &metadata=SdrTokenMap(),
        const std::string& blindData=std::string(),
        const TfToken& subIdentifier=TfToken()
    ) : identifier(identifier),
        version(version),
        name(name),
        function(function),
        family(this->function),
        discoveryType(discoveryType),
        shadingSystem(shadingSystem),
        sourceType(this->shadingSystem),
        uri(uri),
        resolvedUri(resolvedUri),
        sourceCode(sourceCode),
        metadata(metadata),
        blindData(blindData),
        subIdentifier(subIdentifier)
    { }

    /// The following methods will removed in favor of implicitly defined
    /// methods once `sourceType` is fully removed from Sdr. The sourceType
    /// reference to shadingSystem in this class causes this group of formerly
    /// implicit methods to be deleted.
    /// {@
    SdrShaderNodeDiscoveryResult(SdrShaderNodeDiscoveryResult&& other)
      : identifier(std::move(other.identifier)),
        version(std::move(other.version)),
        name(std::move(other.name)),
        function(std::move(other.function)),
        family(this->function),
        discoveryType(std::move(other.discoveryType)),
        shadingSystem(std::move(other.shadingSystem)),
        sourceType(this->shadingSystem),
        uri(std::move(other.uri)),
        resolvedUri(std::move(other.resolvedUri)),
        sourceCode(std::move(other.sourceCode)),
        metadata(std::move(other.metadata)),
        blindData(std::move(other.blindData)),
        subIdentifier(std::move(other.subIdentifier))
    { }

    SdrShaderNodeDiscoveryResult(const SdrShaderNodeDiscoveryResult& other)
      : identifier(other.identifier),
        version(other.version),
        name(other.name),
        function(other.function),
        family(this->function),
        discoveryType(other.discoveryType),
        shadingSystem(other.shadingSystem),
        sourceType(this->shadingSystem),
        uri(other.uri),
        resolvedUri(other.resolvedUri),
        sourceCode(other.sourceCode),
        metadata(other.metadata),
        blindData(other.blindData),
        subIdentifier(other.subIdentifier)
    { }

    SdrShaderNodeDiscoveryResult& operator=(
        SdrShaderNodeDiscoveryResult&& other)
    {
        this->identifier = std::move(other.identifier);
        this->version = std::move(other.version);
        this->name = std::move(other.name);
        this->function = std::move(other.function);
        this->family = this->function;
        this->discoveryType = std::move(other.discoveryType);
        this->shadingSystem = std::move(other.shadingSystem);
        this->sourceType = this->shadingSystem;
        this->uri = std::move(other.uri);
        this->resolvedUri = std::move(other.resolvedUri);
        this->sourceCode = std::move(other.sourceCode);
        this->metadata = std::move(other.metadata);
        this->blindData = std::move(other.blindData);
        this->subIdentifier = std::move(other.subIdentifier);
        return *this;
    }

    SdrShaderNodeDiscoveryResult& operator=(
        const SdrShaderNodeDiscoveryResult& other)
    {
        SdrShaderNodeDiscoveryResult copy(other);
        *this = std::move(copy);
        return *this;
    }
    /// @}

    /// The node's identifier.
    ///
    /// How the node is identified. In many cases this will be the
    /// name of the file or resource that this node originated from.
    /// E.g. "mix_float_2_1".  The identifier must be unique for a
    /// given sourceType.
    SdrIdentifier identifier;

    /// The node's version.  This may or may not be embedded in the
    /// identifier, it's up to implementations.  E.g a node with
    /// identifier "mix_float_2_1" might have version 2.1.
    SdrVersion version;

    /// The node's name.
    ///
    /// A version independent identifier for the node type.  This will
    /// often embed type parameterization but should not embed the
    /// version.  E.g a node with identifier "mix_float_2_1" might have
    /// name "mix_float".
    std::string name;

    /// The node's function.
    ///
    /// A node's function specifies a generic grouping of nodes.
    /// E.g a node with identifier "mix_float_2_1" might have function "mix".
    TfToken function;

    /// The node's family.
    ///
    /// \deprecated
    /// Deprecated in favor of `function`.
    TfToken& family;

    /// The node's discovery type.
    ///
    /// The type could be the file extension, or some other type of metadata
    /// that can signify a type prior to parsing. See the documentation for
    /// `SdrParserPlugin` and `SdrParserPlugin::DiscoveryTypes` for more
    /// information on how this value is used.
    TfToken discoveryType;

    /// The node's shading system.
    ///
    /// This type is unique to the parsing plugin and indicates what shading
    /// language and standard should be used to interpret this discovery
    /// result.
    /// \sa SdrShaderNode::GetShadingSystem
    TfToken shadingSystem;

    /// The node's source type.
    ///
    /// \deprecated
    /// Deprecated in favor of `shadingSystem`
    TfToken& sourceType;

    /// The node's origin.
    ///
    /// This may be a filesystem path, a URL pointing to a resource in the
    /// cloud, or some other type of resource identifier.
    std::string uri;

    /// The node's fully-resolved URI.
    ///
    /// For example, this might be an absolute path when the original URI was
    /// a relative path. In most cases, this is the path that `Ar`'s
    /// `Resolve()` returns. In any case, this path should be locally
    /// accessible.
    std::string resolvedUri;

    /// The node's entire source code.
    ///  
    /// The source code is parsed (if non-empty) by parser plugins when the 
    /// resolvedUri value is empty.
    std::string sourceCode;

    /// The node's metadata collected during the discovery process.
    /// 
    /// Additional metadata may be present in the node's source, in the asset
    /// pointed to by resolvedUri or in sourceCode (if resolvedUri is empty).
    /// In general, parsers should override this data with metadata from the 
    /// shader source. 
    SdrTokenMap metadata;

    /// An optional detail for the parser plugin.  The parser plugin
    /// defines the meaning of this data so the discovery plugin must
    /// be written to match.
    std::string blindData;

    /// The subIdentifier is associated with a particular asset and refers to a
    /// specific definition within the asset.  The asset is the one referred to
    /// by `SdrRegistry::GetNodeFromAsset()`.  The subIdentifier is not needed
    /// for all cases where the node definition is not associated with an asset.
    /// Even if the node definition is associated with an asset, the
    /// subIdentifier is only needed if the asset specifies multiple definitions
    /// rather than a single definition.
    TfToken subIdentifier;
};

typedef std::vector<SdrShaderNodeDiscoveryResult> SdrShaderNodeDiscoveryResultVec;

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_SDR_NODE_DISCOVERY_RESULT_H
