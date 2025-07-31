//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_SDF_USDA_FILE_FORMAT_H
#define PXR_USD_SDF_USDA_FILE_FORMAT_H
 
#include "pxr/pxr.h"
#include "pxr/base/tf/declarePtrs.h"
#include "pxr/base/tf/envSetting.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/usd/sdf/api.h"
#include "pxr/usd/sdf/declareHandles.h" 
#include "pxr/usd/sdf/fileFormat.h"

#include <iosfwd>
#include <string>

PXR_NAMESPACE_OPEN_SCOPE

#define SDF_USDA_FILE_FORMAT_TOKENS \
    ((Id,      "usda"))             \
    ((Version, "1.0"))

TF_DECLARE_PUBLIC_TOKENS(SdfUsdaFileFormatTokens, SDF_API, SDF_USDA_FILE_FORMAT_TOKENS);

TF_DECLARE_WEAK_AND_REF_PTRS(SdfUsdaFileFormat);

SDF_DECLARE_HANDLES(SdfSpec);

class ArAsset;

/// Environment variable indicating the number of MB at which warnings
/// will be emitted when reading a SdfUsdaFileFormat-derived text file.
///
/// \deprecated
/// The env variable functionality will remain, but its API exposure below
/// will be removed in a subsequent release.
SDF_API
extern TfEnvSetting<int> SDF_TEXTFILE_SIZE_WARNING_MB;

/// \class SdfUsdaFileFormat
///
/// File format used by textual USD files.
///
class SdfUsdaFileFormat : public SdfFileFormat
{
public:
    // SdfFileFormat overrides.
    SDF_API
    virtual bool CanRead(const std::string &file) const override;

    SDF_API
    virtual bool Read(
        SdfLayer* layer,
        const std::string& resolvedPath,
        bool metadataOnly) const override;

    SDF_API
    virtual bool WriteToFile(
        const SdfLayer& layer,
        const std::string& filePath,
        const std::string& comment = std::string(),
        const FileFormatArguments& args = FileFormatArguments()) const override;

    SDF_API
    virtual bool ReadFromString(
        SdfLayer* layer,
        const std::string& str) const override;

    SDF_API
    virtual bool WriteToString(
        const SdfLayer& layer,
        std::string* str,
        const std::string& comment = std::string()) const override;

    SDF_API
    virtual bool WriteToStream(
        const SdfSpecHandle &spec,
        std::ostream& out,
        size_t indent) const override;

protected:
    SDF_FILE_FORMAT_FACTORY_ACCESS;

    /// Destructor.
    SDF_API
    virtual ~SdfUsdaFileFormat();

    /// Constructor.
    SDF_API
    SdfUsdaFileFormat();

    /// Constructor. This form of the constructor may be used by formats that
    /// use the .usda text format as their internal representation. 
    /// If a non-empty versionString and target are provided, they will be
    /// used as the file format version and target; otherwise the .usda format
    /// version and target will be implicitly used.
    SDF_API
    explicit SdfUsdaFileFormat(const TfToken& formatId,
                               const TfToken& versionString = TfToken(),
                               const TfToken& target = TfToken());

    /// Return true if layer can be read from \p asset at \p resolvedPath.
    SDF_API
    bool _CanReadFromAsset(
        const std::string& resolvedPath,
        const std::shared_ptr<ArAsset>& asset) const;

    /// Read layer from \p asset at \p resolvedPath into \p layer.
    SDF_API 
    bool _ReadFromAsset(
        SdfLayer* layer, 
        const std::string& resolvedPath,
        const std::shared_ptr<ArAsset>& asset,
        bool metadataOnly) const;

private:
    // Override to return false.  Reloading anonymous text layers clears their
    // content.
    SDF_API virtual bool _ShouldSkipAnonymousReload() const override;

    friend class SdfUsdFileFormat;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_SDF_USDA_FILE_FORMAT_H
