//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "spirvTransforms.h"

#include "pxr/base/tf/span.h"
#include "pxr/base/trace/trace.h"

#include <spirv-tools/libspirv.hpp>
#include <spirv/unified1/spirv.h>

#include <unordered_set>

PXR_NAMESPACE_OPEN_SCOPE

TF_REGISTRY_FUNCTION(TfDebug)
{
    TF_DEBUG_ENVIRONMENT_SYMBOL(HGIWEBGPU_DEBUG_SPIRV_TRANSFORM,
        "Dump SPIR-V after transformation");
}

using Word = uint32_t;
using Operand = Word;
using Id = Word;
using Literal = TfSpan<const Word>;

class _SpvByteCode;

/// Extend this class and override the functions to receive parsed
/// instructions from a SPIR-V module. The functions are called in
/// order following the logical layout rules. Keep the functions
/// sorted this way too. See:
/// https://registry.khronos.org/SPIR-V/specs/unified1/SPIRV.html#_logical_layout_of_a_module
/// This only covers the opcode we use now, add more when necessary.
class _SpvVisitor
{
public:
    explicit _SpvVisitor(_SpvByteCode& byteCode)
        : _byteCode{byteCode}
    {
    }

    virtual ~_SpvVisitor() = default;

    virtual void
    OpEntryPoint(SpvExecutionModel executionModel, Id entryPoint,
        std::string_view name, TfSpan<const Operand> variables)
    {
    }

    virtual void
    OpDecorate(Id target, SpvDecoration decoration,
        const std::vector<Literal>& extra)
    {
    }

    virtual void
    OpMemberDecorate(Id structType, Word member, SpvDecoration decoration,
        const std::vector<Literal>& extra)
    {
    }

    virtual void
    OpTypeInt(Id result, Word width, Word isSigned)
    {
    }

    virtual void
    OpTypeFloat(Id result, Word width, std::optional<Operand> encoding)
    {
    }

    virtual void
    OpTypePointer(Id result, SpvStorageClass storageClass, Id pointeeType)
    {
    }

    virtual void
    OpConstant(Id resultType, Id result, Literal value)
    {
    }

    virtual void
    OpVariable(Id resultType, Id result, SpvStorageClass storageClass,
        std::optional<Operand> initializer)
    {
    }

    /// This is called once before the first OpFunction()
    virtual void
    BeginFunctions()
    {
    }

    virtual void
    OpFunction(Id resultType, Id result, SpvFunctionControlMask functionControl,
        Id functionType)
    {
    }

    virtual void
    OpFunctionEnd()
    {
    }

    virtual void
    OpLabel(Id result)
    {
    }

protected:
    /// An instruction is copied to the result
    /// bytecode before the callback is called.
    /// Use the mutation function to edit the bytecode.
    _SpvByteCode& _byteCode;
};

/// Represents a SPIR-V bytecode to transform.
/// The transformations are done on a copy.
class _SpvByteCode
{
public:
    explicit _SpvByteCode(const std::vector<Word>& spirv)
        : _oldSpirv{spirv}
    {
        _newSpirv.reserve(_oldSpirv.size());
        _spvTools.SetMessageConsumer(&_PrintMessage);
    }

    /// Convenience overload.
    template<typename VisitorType>
    bool
    Apply()
    {
        VisitorType visitor{*this};
        return Apply(visitor);
    }

    /// Parse the bytecode and apply a transformation.
    /// Copies the instructions one at a time, and calls
    /// the corresponding _SpvVisitor virtual member function
    /// after each supported instruction is copied.
    bool
    Apply(_SpvVisitor& visitor)
    {
        _firstFunctionSeen = false;
        _spvTools.Parse(_oldSpirv,
            [this](const spv_endianness_t endianness,
                    const spv_parsed_header_t& header) {
                return _ParseHeader(endianness, header);
            },
            [this, &visitor](const spv_parsed_instruction_t& inst) {
                return _ParseInstruction(visitor, inst);
            });

        _newSpirv[offsetof(spv_parsed_header_t, bound) / sizeof(Word)] = _nextId;

        if (ARCH_UNLIKELY(TfDebug::IsEnabled(
                HGIWEBGPU_DEBUG_SPIRV_TRANSFORM))) {
            std::string disassembly;
            _spvTools.Disassemble(_newSpirv, &disassembly);
            TF_DEBUG(HGIWEBGPU_DEBUG_SPIRV_TRANSFORM).Msg(
                "--- BEGIN SPIR-V DISASSEMBLY ---\n%s"
                "\n---- END SPIR-V DISASSEMBLY ----\n",
                disassembly.c_str());
        }

        if (ARCH_UNLIKELY(!_spvTools.Validate(_newSpirv))) {
            TF_CODING_ERROR("SPIR-V transformation failed");
            return false;
        }

        return true;
    }

    /// Inject an instruction immediately after the current instruction,
    /// and any previously injected instruction.
    void
    Inject(SpvOp op, std::initializer_list<Word> words)
    {
        const auto opCode = (static_cast<Word>(words.size() + 1) << 16) |
                (static_cast<Word>(op) & 0xffff);
        _newSpirv.insert(_newSpirv.end(), opCode);
        _newSpirv.insert(_newSpirv.end(), words);
    }

    /// Modify a word in the bytecode.
    void
    Modify(size_t offset, Word word)
    {
        _newSpirv.at(offset) = word;
    }

    /// Word offset of the current instruction in the bytecode.
    /// This is the position of the first word in the vector.
    size_t
    CurrentOffset() const
    {
        return _offset;
    }

    /// Allocate a new <id>.
    Id
    NextId()
    {
        return _nextId++;
    }

    /// Move out the bytecode transformation output.
    std::vector<Word>
    TakeResult()
    {
        return std::move(_newSpirv);
    }

    /// Get a word sized int from a literal.
    static Word
    LiteralAsInt(Literal literal)
    {
        if (ARCH_UNLIKELY(literal.size() != 1)) {
            TF_CODING_ERROR("Literal must be one word");
        }

        return literal.front();
    }

    /// Get an enum from a literal.
    template<typename T>
    static T
    LiteralAsEnum(Literal literal)
    {
        static_assert(std::is_enum_v<T>, "T must be an enum");
        return static_cast<T>(LiteralAsInt(literal));
    }

    /// Get a UTF-8 string from a literal.
    static std::string_view
    LiteralAsString(Literal literal)
    {
        const auto chars = reinterpret_cast<const char*>(literal.data());
        const auto length = strnlen(chars, literal.size() * sizeof(Word));
        return {chars, length};
    }

private:
    static void
    _PrintMessage(spv_message_level_t level, const char *source,
        const spv_position_t &position, const char *message)
    {
        switch (level) {
            case SPV_MSG_FATAL:
                TF_FATAL_ERROR("%s:%lu -- %s",
                    source, position.index, message);
            break;
            case SPV_MSG_INTERNAL_ERROR:
            case SPV_MSG_ERROR:
                TF_RUNTIME_ERROR("%s:%lu -- %s",
                    source, position.index, message);
            break;
            case SPV_MSG_WARNING:
                TF_WARN("%s:%lu -- %s",
                    source, position.index, message);
            break;
            case SPV_MSG_INFO:
            case SPV_MSG_DEBUG:
                break;
        }
    }

    spv_result_t
    _ParseHeader(const spv_endianness_t, const spv_parsed_header_t& header)
    {
        _nextId = header.bound;
        _newSpirv.resize(sizeof(header) / sizeof(Word));
        std::memcpy(_newSpirv.data(), &header, sizeof(header));
        return SPV_SUCCESS;
    }

    spv_result_t
    _ParseInstruction(_SpvVisitor& visitor,
        const spv_parsed_instruction_t& inst)
    {
        if (!_firstFunctionSeen && inst.opcode == SpvOpFunction) {
            _firstFunctionSeen = true;
            visitor.BeginFunctions();
        }

        _offset = _newSpirv.size();
        _newSpirv.insert(_newSpirv.end(), inst.words,
            inst.words + inst.num_words);

        size_t opIndex = 0;
        const auto nextOperand = [&inst, &opIndex] {
            return inst.words[inst.operands[opIndex++].offset];
        };
        const auto optionalOperand = [&inst, &opIndex, &nextOperand] {
            return opIndex < inst.num_operands ?
                std::optional{nextOperand()} : std::nullopt;
        };
        const auto remainingOperands = [&inst, &opIndex] {
            // One word per operand
            const auto firstWordOffset =
                static_cast<size_t>(inst.operands[opIndex].offset);
            opIndex = inst.num_operands;
            return TfSpan{inst.words + firstWordOffset,
                inst.num_words - firstWordOffset};
        };
        const auto nextLiteral = [&inst, &opIndex] {
            const auto& operand = inst.operands[opIndex++];
            return Literal{inst.words + operand.offset, operand.num_words};
        };
        const auto remainingLiterals = [&inst, &opIndex, &nextLiteral] {
            std::vector<Literal> literals;
            for (size_t i = opIndex; i < inst.num_operands; i++) {
                literals.push_back(nextLiteral());
            }
            return literals;
        };
        const auto skipInstruction = [&inst, &opIndex] {
            opIndex = inst.num_operands;
        };

        switch (inst.opcode) {
            case SpvOpEntryPoint:
                visitor.OpEntryPoint(
                    static_cast<SpvExecutionModel>(nextOperand()),
                    nextOperand(),
                    LiteralAsString(nextLiteral()),
                    remainingOperands());
                break;
            case SpvOpDecorate:
                visitor.OpDecorate(
                    nextOperand(),
                    static_cast<SpvDecoration>(nextOperand()),
                    remainingLiterals());
                break;
            case SpvOpMemberDecorate:
                visitor.OpMemberDecorate(
                    nextOperand(),
                    LiteralAsInt(nextLiteral()),
                    static_cast<SpvDecoration>(nextOperand()),
                    remainingLiterals());
                break;
            case SpvOpTypeInt:
                visitor.OpTypeInt(
                    nextOperand(),
                    LiteralAsInt(nextLiteral()),
                    LiteralAsInt(nextLiteral()));
                break;
            case SpvOpTypeFloat:
                visitor.OpTypeFloat(
                    nextOperand(),
                    LiteralAsInt(nextLiteral()),
                    optionalOperand());
                break;
            case SpvOpTypePointer:
                visitor.OpTypePointer(
                    nextOperand(),
                    static_cast<SpvStorageClass>(nextOperand()),
                    nextOperand());
                break;
            case SpvOpConstant:
                visitor.OpConstant(
                    nextOperand(),
                    nextOperand(),
                    nextLiteral());
                break;
            case SpvOpVariable:
                visitor.OpVariable(
                    nextOperand(),
                    nextOperand(),
                    static_cast<SpvStorageClass>(nextOperand()),
                    optionalOperand());
                break;
            case SpvOpFunction:
                visitor.OpFunction(
                    nextOperand(),
                    nextOperand(),
                    static_cast<SpvFunctionControlMask>(nextOperand()),
                    nextOperand());
                break;
            case SpvOpFunctionEnd:
                visitor.OpFunctionEnd();
                break;
            case SpvOpLabel:
                visitor.OpLabel(nextOperand());
                break;
            default:
                skipInstruction();
                break;
        }

        if (ARCH_UNLIKELY(opIndex != inst.num_operands)) {
            TF_CODING_ERROR("Operands were not evaluated correctly");
        }

        return SPV_SUCCESS;
    }

    const std::vector<Word>& _oldSpirv;
    std::vector<Word> _newSpirv;

    spvtools::SpirvTools _spvTools{SPV_ENV_VULKAN_1_0};

    size_t _offset = 0;
    Id _nextId = 0;
    bool _firstFunctionSeen = false;
};

/// Negate the output vertex position Y coordinate, at the very end of the
/// shader. This is achieved by redirecting the current vertex entry point to a
/// new one, which is injected right after the original one. This new entry
/// point calls the original one, negates the output vertex position Y
/// coordinate, then returns. In GLSL, it looks like this:
///     out gl_PerVertex
///     {
///         vec4 gl_Position;
///     };
///
///     void old_main() { ... } // renamed
///
///     void main()
///     {
///         old_main();
///         gl_PerVertex.gl_Position.y = -gl_PerVertex.gl_Position.y;
///     }
///
/// In SPIR-V, gl_PerVertex is compiled to a "Block" struct. Its value is
/// exposed as an "Output" pointer variable to the struct. In pseudo C++, it
/// looks like this:
///     [[Block]] struct gl_PerVertex
///     {
///         [[BuiltIn(Position)]] vec4 gl_Position;
///     };
///     [[Output]] gl_PerVertex* vertexOut;
///
///     void old_main() { ... } // renamed
///
///     [[Interface(vertexOut)]]
///     void main()
///     {
///         old_main();
///         vertexOut->gl_Position.y = -vertexOut->gl_Position.y;
///     }
///
/// To do the transformation, we locate all structs annotated (decorated)
/// with Block and with a BuiltIn Position member. We look for an output
/// variable having such a struct pointer type, and also belonging to the entry
/// point interface variable list. Then after the original entry point has been
/// parsed, we write our new entry point, using the information we collected
/// about the variables and structs to negate gl_Position.y in the output.
///
/// Finally, in SPIR-V it looks like this (some code omitted):
///                          OpEntryPoint Vertex %main "main" %gl_PerVertex_var
///                          ; bytecode omitted...
///                          OpMemberDecorate %gl_PerVertex 0 BuiltIn Position
///                          OpDecorate %gl_PerVertex Block
///                          ; bytecode omitted...
///          %gl_PerVertex = OpTypeStruct %vec4
///  %gl_PerVertex_out_ptr = OpTypePointer Output %gl_PerVertex
///      %gl_PerVertex_var = OpVariable %gl_PerVertex_out_ptr Output
///                          ; bytecode omitted...
///                          ; \/ our injected entry point \/
///                  %main = OpFunction %void None %main_func
///                %unused = OpLabel
///           %void_return = OpFunctionCall %void %old_main
///        %position_y_ptr = OpAccessChain %out_float_ptr %gl_PerVertex_var %int_0 %int_1
///            %position_y = OpLoad %float %position_y_ptr
///      %minus_position_y = OpFNegate %float %position_y
///                          OpStore %position_y_ptr %minus_position_y
///                          OpReturn
///                          OpFunctionEnd
///
/// Note that shader interface with Block structs is defined in the Vulkan spec,
/// so we can assume GLSL is always compiled this way. See:
/// https://registry.khronos.org/vulkan/specs/1.3/html/vkspec.html#interfaces-iointerfaces
class _FlipYVisitor final : public _SpvVisitor
{
public:
    using _SpvVisitor::_SpvVisitor;

    ~_FlipYVisitor() override = default;

    void
    OpEntryPoint(SpvExecutionModel executionModel, Id entryPoint,
        std::string_view name, TfSpan<const Operand> variables) override
    {
        if (executionModel == SpvExecutionModelVertex) {
            _entryPointsById.try_emplace(entryPoint,
                _EntryPoint{_byteCode.CurrentOffset(),
                    {variables.begin(), variables.end()}});
        }
    }

    void
    OpDecorate(Id target, SpvDecoration decoration,
        const std::vector<Literal>& extra) override
    {
        // OpDecorate and OpMemberDecorate are un-sequenced,
        // so record all structs that are Block or with a
        // BuiltIn Position member. We'll filter those with only
        // both in OpVariable, when all decoration are complete.
        if (decoration == SpvDecorationBlock) {
            if (const auto it = _structByTypeId.find(target);
                    it != _structByTypeId.end()) {
                it->second._isBlock = true;
            } else {
                _structByTypeId.try_emplace(target,
                    _Struct{std::nullopt, true});
            }
        }
    }

    void
    OpMemberDecorate(Id structType, Word member, SpvDecoration decoration,
        const std::vector<Literal>& extra) override
    {
        if (decoration == SpvDecorationBuiltIn &&
                _SpvByteCode::LiteralAsEnum<SpvBuiltIn>(extra.front()) ==
                SpvBuiltInPosition) {
            if (const auto it = _structByTypeId.find(structType);
                    it != _structByTypeId.end()) {
                it->second._positionBuiltInMember = member;
            } else {
                _structByTypeId.try_emplace(structType, _Struct{member, false});
            }
        }
    }

    void
    OpTypeInt(Id result, Word width, Word isSigned) override
    {
        // Looks for the types we'll need to write
        // our new entry point. We'll declare any
        // missing ones in BeginFunctions().

        if (width == 32 && isSigned == 1) {
            _intTypeId = result;
        }
    }

    void
    OpTypeFloat(Id result, Word width, std::optional<Operand> encoding) override
    {
        if (width == 32) {
            _floatTypeId = result;
        }
    }

    void
    OpTypePointer(Id result, SpvStorageClass storageClass,
        Id pointeeType) override
    {
        if (storageClass == SpvStorageClassOutput) {
            if (pointeeType == _floatTypeId) {
                _floatPointerTypeId = result;
            } else if (const auto it = _structByTypeId.find(pointeeType);
                    it != _structByTypeId.end()) {
                // Variables are always pointer types, so record those.
                _structByPointerTypeId.try_emplace(result, &it->second);
            }
        }
    }

    void
    OpConstant(Id resultType, Id result, Literal value) override
    {
        // We need int constants to access struct
        // members using indices.

        if (resultType == _intTypeId) {
            _intConstantIds.try_emplace(_SpvByteCode::LiteralAsInt(value),
                result);
        }
    }

    void
    OpVariable(Id resultType, Id result, SpvStorageClass storageClass,
        std::optional<Operand> initializer) override
    {
        if (storageClass != SpvStorageClassOutput) {
            return;
        }

        // If a variable has a vertex output
        // block struct type, then record it.
        if (const auto it = _structByPointerTypeId.find(resultType);
                it != _structByPointerTypeId.end()) {
            if (const auto* struct_ = it->second;
                struct_->_isBlock && struct_->_positionBuiltInMember) {
                _vertexOutByVariableId.try_emplace(result,
                    _VertexOutStruct{*struct_->_positionBuiltInMember});
            }
        }
    }

    void
    BeginFunctions() override
    {
        // Write out any missing types and constants
        // just before the first OpFunction, as required
        // by the SPIR-V module logical layout.

        if (!_intTypeId) {
            _intTypeId = _byteCode.NextId();
            _byteCode.Inject(SpvOpTypeInt, {_intTypeId, 32, 1});
        }
        if (!_floatTypeId) {
            _floatTypeId = _byteCode.NextId();
            _byteCode.Inject(SpvOpTypeFloat, {_floatTypeId, 32});
        }
        if (!_floatPointerTypeId) {
            _floatPointerTypeId = _byteCode.NextId();
            _byteCode.Inject(SpvOpTypePointer,
                {_floatPointerTypeId, SpvStorageClassOutput, _floatTypeId});
        }

        static constexpr Word yMember = 1;
        if (!_intConstantIds.count(yMember)) {
            _InjectConstInt(yMember);
        }

        for (const auto& [_, vertexOut] : _vertexOutByVariableId) {
            if (!_intConstantIds.count(vertexOut._positionBuiltInMember)) {
                _InjectConstInt(vertexOut._positionBuiltInMember);
            }
        }
    }

    void
    OpFunction(Id resultType, Id result, SpvFunctionControlMask functionControl,
        Id functionType) override
    {
        // Look for the vertex entry points to override,
        // by using the interface variable list.
        if (const auto entryIt = _entryPointsById.find(result);
                entryIt != _entryPointsById.end()) {
            for (Id variable : entryIt->second._interfaceVariables) {
                if (const auto structIt = _vertexOutByVariableId.find(variable);
                        structIt != _vertexOutByVariableId.end()) {
                    // Record all the data we need to define
                    // the new entry point and call the old one.
                    entryIt->second._functionId = result;
                    entryIt->second._functionTypeId = functionType;
                    entryIt->second._resultTypeId = resultType;
                    entryIt->second._outputVariableId = variable;
                    entryIt->second._vertexOut = &structIt->second;
                    // It's a declaration unless we
                    // see a label before OpFunctionEnd
                    _currentEntryPointDeclaration = &entryIt->second;
                    break;
                }
            }
        }
    }

    void
    OpFunctionEnd() override
    {
        if (!_currentEntryPointDefinition) {
            return;
        }

        // See the class documentation for an explanation of this code.
        const Id newEntryPointId = _byteCode.NextId();
        _byteCode.Modify(
            _currentEntryPointDefinition->_instructionOffset + 2,
            newEntryPointId);

        _byteCode.Inject(SpvOpFunction, {
            _currentEntryPointDefinition->_resultTypeId,
            newEntryPointId,
            SpvFunctionControlMaskNone,
            _currentEntryPointDefinition->_functionTypeId
        });
        _byteCode.Inject(SpvOpLabel, {_byteCode.NextId()});

        _byteCode.Inject(SpvOpFunctionCall,
            {_currentEntryPointDefinition->_resultTypeId,
            _byteCode.NextId(), _currentEntryPointDefinition->_functionId});

        const Id positionYPtr = _byteCode.NextId();
        _byteCode.Inject(SpvOpAccessChain, {
            _floatPointerTypeId, positionYPtr,
            _currentEntryPointDefinition->_outputVariableId,
            _intConstantIds.at(_currentEntryPointDefinition->_vertexOut->
                _positionBuiltInMember),
            _intConstantIds.at(1)
        });
        const Id positionY = _byteCode.NextId();
        _byteCode.Inject(SpvOpLoad, {_floatTypeId, positionY, positionYPtr});
        const Id minusPositionY = _byteCode.NextId();
        _byteCode.Inject(SpvOpFNegate,
            {_floatTypeId, minusPositionY, positionY});
        _byteCode.Inject(SpvOpStore, {positionYPtr, minusPositionY});

        _byteCode.Inject(SpvOpReturn, {});
        _byteCode.Inject(SpvOpFunctionEnd, {});

        _currentEntryPointDefinition = nullptr;
    }

    void
    OpLabel(Id result) override
    {
        if (_currentEntryPointDeclaration) {
            // Label marks this as a definition
            _currentEntryPointDefinition = _currentEntryPointDeclaration;
            _currentEntryPointDeclaration = nullptr;
        }
    }

private:
    void
    _InjectConstInt(Word value)
    {
        const Id constIntId = _byteCode.NextId();
        _byteCode.Inject(SpvOpConstant, {_intTypeId, constIntId, value});
        _intConstantIds.try_emplace(value, constIntId);
    }

    /// Vertex output struct candidates
    struct _Struct {
        std::optional<Word> _positionBuiltInMember;
        bool _isBlock = false;
    };
    std::unordered_map<Id, _Struct> _structByTypeId;
    std::unordered_map<Id, _Struct*> _structByPointerTypeId;

    /// A vertex output struct
    struct _VertexOutStruct {
        // Initialize with an invalid value
        Word _positionBuiltInMember = std::numeric_limits<Word>::max();
    };
    std::unordered_map<Id, _VertexOutStruct> _vertexOutByVariableId;

    /// A vertex entry point to redirect
    struct _EntryPoint {
        size_t _instructionOffset = 0;
        std::unordered_set<Id> _interfaceVariables;
        Id _functionId = 0;
        Id _functionTypeId = 0;
        Id _resultTypeId = 0;
        Id _outputVariableId = 0;
        _VertexOutStruct* _vertexOut = nullptr;
    };
    std::unordered_map<Id, _EntryPoint> _entryPointsById;

    /// Types and constants we need
    Id _intTypeId = 0;
    Id _floatTypeId = 0;
    Id _floatPointerTypeId = 0;
    std::unordered_map<Word, Id> _intConstantIds;

    /// Track the entry point we're currently visiting
    _EntryPoint* _currentEntryPointDeclaration = nullptr;
    _EntryPoint* _currentEntryPointDefinition = nullptr;
};

bool
ApplySpirvViewportFlip(std::vector<uint32_t>& spirv)
{
    TRACE_FUNCTION();

    _SpvByteCode byteCode{spirv};
    if (ARCH_UNLIKELY(!byteCode.Apply<_FlipYVisitor>())) {
        return false;
    }

    spirv = byteCode.TakeResult();
    return true;
}

PXR_NAMESPACE_CLOSE_SCOPE
