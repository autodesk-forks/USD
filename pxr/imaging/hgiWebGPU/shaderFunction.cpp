//
// Copyright 2022 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//
#include "pxr/imaging/hgiWebGPU/shaderFunction.h"
#include "pxr/base/tf/envSetting.h"
#include "pxr/imaging/hgiWebGPU/hgi.h"
#include "pxr/imaging/hgiWebGPU/api.h"
#include "pxr/imaging/hgiWebGPU/conversions.h"
#include "pxr/imaging/hgiWebGPU/shaderCompiler.h"
#include "pxr/imaging/hgiWebGPU/shaderGenerator.h"
#include "pxr/imaging/hgiWebGPU/debugCodes.h"
#include "pxr/base/tf/debug.h"

#include <iostream>
#include <sstream>
#include <fstream>
#include <filesystem>

// tint include depends on this defines to populate the appropriate namespace
#define TINT_BUILD_SPV_READER 1
#define TINT_BUILD_WGSL_WRITER 1
#include <tint/tint.h>
// This include shouldn't be necessary, but it's missing in tint.h
#include <src/tint/lang/core/ir/module.h>

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_ENV_SETTING(HGIWEBGPU_SHADER_CACHE_DIR, "",
    "Define a directory to cache shaders transpiled from GLSL to WGSL."
    "If not set, the shaders will be compiled at runtime."
    "In the case of emscripten, the application is responsible for"
    "mounting the directory to the virtual filesystem."
    "A use case that persists sessions is to use the indexedDB."
    "For example, in the preload.js script:"
    "FS.mkdir(\"/<HGIWEBGPU_SHADER_CACHE_DIR>\");\n"
    "    FS.mount(IDBFS, {autoPersist: true}, \"/<HGIWEBGPU_SHADER_CACHE_DIR>\");\n"
    "    return FS.syncfs(true, function (err) {\n"
    "        if (err) {\n"
    "            console.error(\"Error syncing filesystem:\", err);\n"
    "        } else {\n"
    "            console.log(\"Filesystem synced.\");\n"
    "        }\n"
    "    });"
    "Notice that the directory must match the one defined in the env setting."
    "In case of using this method, you also need to pass the -lidbfs.js linked flag to the application.");

std::string GlslToWgsl(
        const char *shaderCode, HgiShaderStage stage, std::string *errors, const char* debugLbl)
{
    std::string wgslCode;
    // Compile shader and capture errors
    std::vector<unsigned int> spirvData;
    bool result = HgiWebGPUCompileGLSL(
            debugLbl,
            &shaderCode,
            1,
            stage,
            &spirvData,
            errors);

    if (result) {
        tint::spirv::reader::Options readerOptions{};
        readerOptions.allow_non_uniform_derivatives = true;
        std::unordered_set<tint::wgsl::Extension> extensions = {
            tint::wgsl::Extension::kPrimitiveIndex,
            tint::wgsl::Extension::kClipDistances
        };
        //// SPIR-V
        readerOptions.allowed_features.extensions.insert(extensions.begin(), extensions.end());
        tint::Result<tint::core::ir::Module> irResult = tint::spirv::reader::ReadIR(spirvData, readerOptions);
        if (irResult != tint::Success) {
            TF_CODING_ERROR("Tint SPIR-V reader failure:\nParser: " + irResult.Failure().reason + "\n");
            return wgslCode;
        }

        tint::wgsl::writer::Options writerOptions;
        writerOptions.allow_non_uniform_derivatives = true;
        writerOptions.allowed_features.extensions.insert(extensions.begin(), extensions.end());
        writerOptions.allowed_features.features.emplace(tint::wgsl::LanguageFeature::kReadonlyAndReadwriteStorageTextures);
        auto wgslResult = tint::wgsl::writer::ProgramFromIR(irResult.Get(), writerOptions);
        if (wgslResult == tint::Success) {
            tint::Program program = wgslResult.Move();
            if (program.IsValid()) {
                auto tintResult = tint::wgsl::writer::Generate(program);
                if (tintResult == tint::Success) {
                    wgslCode = tintResult->wgsl;
                } else {
                    *errors = tintResult.Failure().reason;
                }
            } else {
                TF_CODING_ERROR("Tint WGSL writer failure:\nParser: " + program.Diagnostics().Str() + "\n");
            }
        } else {
            *errors = wgslResult.Failure().reason;
        }
    }
    return wgslCode;
}

void HgiWebGPUShaderFunction::_CreateBuffersBindingGroupLayoutEntries(
        std::vector<HgiShaderFunctionBufferDesc> const &buffers,
        std::vector<HgiShaderFunctionParamDesc> const &constants,
        wgpu::ShaderStage const &stage)
{
    BindGroupLayoutEntryMap bufferBindGroupEntries;
    BindGroupLayoutEntryMap constantBindGroupEntries;

    if (constants.size() > 0) {
        wgpu::BindGroupLayoutEntry entry;
        // TODO: bindIndex create a static var or derive it from somewhere
        entry.binding = 0;
        entry.visibility = stage;
        entry.buffer.type = wgpu::BufferBindingType::Uniform;
        constantBindGroupEntries.insert(std::make_pair(0,entry));
    }
    _bindGroups.insert(std::make_pair(HgiWebGPUBufferShaderSection::constantsBindingSet, constantBindGroupEntries));

    for (HgiShaderFunctionBufferDesc const &b : buffers)
    {
        wgpu::BindGroupLayoutEntry entry;
        wgpu::BufferBindingLayout bufferLayout;
        bufferLayout.type = HgiWebGPUConversions::GetBufferBindingType(b.binding, b.writable);

        if (stage & wgpu::ShaderStage::Vertex && b.writable && bufferLayout.type == wgpu::BufferBindingType::Storage) {
            // Even though webgpu supports read-write buffers for Fragment shaders, we need to unify the
            // shader code declaration between the two stages
            TF_WARN("No support for writable buffer named %s in vertex stage", b.nameInShader.c_str());
        }
        entry.binding = b.bindIndex;
        entry.buffer = bufferLayout;
        entry.visibility = stage;
        bufferBindGroupEntries.insert(std::make_pair(b.bindIndex, entry));
    }
    _bindGroups.insert(std::make_pair(HgiWebGPUBufferShaderSection::bindingSet, bufferBindGroupEntries));
}

void HgiWebGPUShaderFunction::_CreateTexturesGroupLayoutEntries(
        std::vector<HgiShaderFunctionTextureDesc> const &textures,
        wgpu::ShaderStage const &stage)
{
    BindGroupLayoutEntryMap texturesBindGroupEntries;
    BindGroupLayoutEntryMap samplersBindGroupEntries;
    for (size_t i=0; i<textures.size(); i++)
    {
        HgiShaderFunctionTextureDesc const &t = textures[i];

        wgpu::BindGroupLayoutEntry textureEntry;
        wgpu::BindGroupLayoutEntry samplerEntry;
        textureEntry.visibility = stage;
        if (t.writable) {
            // TODO: This is the only access storage for now
            textureEntry.storageTexture.access = wgpu::StorageTextureAccess::ReadWrite;
            if (t.textureType == HgiShaderTextureTypeCubemapTexture) {
                textureEntry.storageTexture.viewDimension = wgpu::TextureViewDimension::e2DArray;
            } else {
                textureEntry.storageTexture.viewDimension = 
                    HgiWebGPUConversions::GetTextureViewDimension(t.dimensions, t.textureType);
            }
            textureEntry.storageTexture.format = HgiWebGPUConversions::GetPixelFormat(t.format);
        } else {
            textureEntry.texture.viewDimension = 
                HgiWebGPUConversions::GetTextureViewDimension(t.dimensions, t.textureType);
            textureEntry.texture.sampleType = HgiWebGPUConversions::GetTextureSampleType(t.format);
            if (t.textureType == HgiShaderTextureTypeDepth) {
                textureEntry.texture.sampleType = wgpu::TextureSampleType::UnfilterableFloat;
            }
        }
        samplerEntry.visibility = stage;
        textureEntry.binding = i;
        samplerEntry.binding = i;
        // TODO: How to derive this?
        samplerEntry.sampler.type = wgpu::SamplerBindingType::Filtering;
        texturesBindGroupEntries.insert(std::make_pair(i,textureEntry));
        samplersBindGroupEntries.insert(std::make_pair(i,samplerEntry));
    }
    _bindGroups.insert(std::make_pair(HgiWebGPUTextureShaderSection::bindingSet, texturesBindGroupEntries));
    _bindGroups.insert(std::make_pair(HgiWebGPUSamplerShaderSection::bindingSet, samplersBindGroupEntries));
}

HgiWebGPUShaderFunction::HgiWebGPUShaderFunction(
    HgiWebGPU *hgi,
    HgiShaderFunctionDesc const& desc)
    : HgiShaderFunction(desc)
    , _shaderModule(nullptr)
{
    HgiWebGPUShaderGenerator shaderGenerator(hgi, desc);

    shaderGenerator.Execute();
    const char *shaderCode = shaderGenerator.GetGeneratedShaderCode();

    wgpu::ShaderStage stage = HgiWebGPUConversions::GetShaderStages(desc.shaderStage);

    _CreateBuffersBindingGroupLayoutEntries(desc.buffers, desc.constantParams, stage);
    _CreateTexturesGroupLayoutEntries(desc.textures, stage);

    wgpu::ShaderSourceWGSL wgslDesc;
    wgpu::ShaderModuleDescriptor shaderModuleDesc;
    wgslDesc.sType = wgpu::SType::ShaderSourceWGSL;
    shaderModuleDesc.label = _descriptor.debugName.c_str();
    shaderModuleDesc.nextInChain = &wgslDesc;
    std::string wgslCode;

    size_t wgslHash = 0;
    std::filesystem::path wgslCachePath;
    std::filesystem::path cacheDir = TfGetEnvSetting(HGIWEBGPU_SHADER_CACHE_DIR);
    const char* debugLbl = _descriptor.debugName.empty() ?
                           "unknown" : _descriptor.debugName.c_str();

    if (cacheDir.empty()) {
        wgslCode = GlslToWgsl(shaderCode, desc.shaderStage, &_errors, debugLbl);
    } else {
        wgslHash = TfHashCString()(shaderCode);
        std::filesystem::path wgslFileName = std::to_string(wgslHash) + "_" + _descriptor.debugName + ".wgsl";
        wgslCachePath = cacheDir / wgslFileName;
        try {
            if (std::filesystem::exists(wgslCachePath)) {
                std::ifstream wgslFile(wgslCachePath);
                if (wgslFile.is_open()) {
                    std::stringstream buffer;
                    buffer << wgslFile.rdbuf();
                    wgslCode = buffer.str();
                } else {
                    throw std::runtime_error("Failed to open WGSL cache file.");
                }
            } else {
                wgslCode = GlslToWgsl(shaderCode, desc.shaderStage, &_errors, debugLbl);
                std::ofstream wgslFile(wgslCachePath);
                if (wgslFile.is_open()) {
                    wgslFile << wgslCode;
                    wgslFile.close();
                } else {
                    throw std::runtime_error("Failed to create WGSL cache file.");
                }
            }
        } catch (const std::exception &e) {
            wgslCode = GlslToWgsl(shaderCode, desc.shaderStage, &_errors, debugLbl);
            TF_RUNTIME_ERROR("%s", e.what());
        }
    }
    if (TfDebug::IsEnabled(HGIWEBGPU_DUMP_SHADER_SOURCEFILE)) {
        std::string fname;
        static size_t globalDebugID = 0;
        std::stringstream fnameStream;
        static size_t debugShaderID = 0;
        fnameStream << "program" << globalDebugID++ << "_shader" << debugShaderID++ << ".wgsl";
        fname = fnameStream.str();
        std::fstream output(fname.c_str(), std::ios::out);
        output << wgslCode;
        output.close();

        std::cout << "Write " << fname
                  << " (size=" << wgslCode.size() << ")\n";
    }
    
    wgslDesc.code = wgslCode.c_str();

    if (_errors.empty()) {
        _shaderModule = hgi->GetPrimaryDevice().CreateShaderModule(&shaderModuleDesc);
        // Get the compilation details
#if defined ARCH_OS_WASM_VM
        // Getting unimplemented in Chrome
        if( !_shaderModule )
        {
            printf("Failed to create shader module\n");
            _errors = "Failed.";
        }
#else
        _shaderModule.GetCompilationInfo(
                wgpu::CallbackMode::AllowSpontaneous,
                [&](wgpu::CompilationInfoRequestStatus status, const wgpu::CompilationInfo* compilationInfo) {
                    if (status != wgpu::CompilationInfoRequestStatus::Success) {
                        std::stringstream errorss;
                        for (uint32_t i = 0; i < compilationInfo->messageCount; ++i) {
                            auto &compilationMessage = compilationInfo->messages[i];
                            errorss << compilationMessage.lineNum << ": " << compilationMessage.message.data << std::endl;
                        }
                        _errors = errorss.str();
                    }
                });
#endif
    }
    
    // Clear these pointers in our copy of the descriptor since we
    // have to assume they could become invalid after we return.
    _descriptor.shaderCodeDeclarations = nullptr;
    _descriptor.shaderCode = nullptr;
    _descriptor.generatedShaderCodeOut = nullptr;
}

HgiWebGPUShaderFunction::~HgiWebGPUShaderFunction()
{
    if( _shaderModule )
        _shaderModule = nullptr;
}

bool HgiWebGPUShaderFunction::IsValid() const
{
    return _errors.empty();
}

std::string const& HgiWebGPUShaderFunction::GetCompileErrors()
{
    return _errors;
}

size_t HgiWebGPUShaderFunction::GetByteSizeOfResource() const
{
    // TODO: I'm not really sure what this should be, in Vulkan this is the SPIRV code size
    // which doesn't seem like a particularly useful thing and I don't think there is a WGSL
    // equivalent of compiled code size
    return 1u;
}

uint64_t HgiWebGPUShaderFunction::GetRawResource() const
{
    return reinterpret_cast<uint64_t>(_shaderModule.Get());
}

const BindGroupsLayoutMap &HgiWebGPUShaderFunction::GetBindGroups() const {
    return _bindGroups;
}


const char *HgiWebGPUShaderFunction::GetShaderEntryPoint() const
{
    // TODO: I hope your shaders use 'main' as the entrypoint ;)
    // Use reflection to get this
    return "main";
}

wgpu::ShaderModule HgiWebGPUShaderFunction::GetShaderModule() const
{
    return _shaderModule;
}

PXR_NAMESPACE_CLOSE_SCOPE
