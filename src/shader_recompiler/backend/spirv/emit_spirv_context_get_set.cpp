// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/assert.h"
#include "shader_recompiler/backend/spirv/emit_spirv_instructions.h"
#include "shader_recompiler/backend/spirv/spirv_emit_context.h"
#include "shader_recompiler/ir/attribute.h"
#include "shader_recompiler/ir/patch.h"
#include "shader_recompiler/runtime_info.h"

#include <magic_enum/magic_enum.hpp>

namespace Shader::Backend::SPIRV {
namespace {

Id VsOutputAttrPointer(EmitContext& ctx, VsOutput output) {
    switch (output) {
    case VsOutput::ClipDist0:
    case VsOutput::ClipDist1:
    case VsOutput::ClipDist2:
    case VsOutput::ClipDist3:
    case VsOutput::ClipDist4:
    case VsOutput::ClipDist5:
    case VsOutput::ClipDist6:
    case VsOutput::ClipDist7: {
        const u32 index = u32(output) - u32(VsOutput::ClipDist0);
        const Id clip_num{ctx.ConstU32(index)};
        ASSERT_MSG(Sirit::ValidId(ctx.clip_distances), "Clip distance used but not defined");
        return ctx.OpAccessChain(ctx.output_f32, ctx.clip_distances, clip_num);
    }
    case VsOutput::CullDist0:
    case VsOutput::CullDist1:
    case VsOutput::CullDist2:
    case VsOutput::CullDist3:
    case VsOutput::CullDist4:
    case VsOutput::CullDist5:
    case VsOutput::CullDist6:
    case VsOutput::CullDist7: {
        const u32 index = u32(output) - u32(VsOutput::CullDist0);
        const Id cull_num{ctx.ConstU32(index)};
        ASSERT_MSG(Sirit::ValidId(ctx.cull_distances), "Cull distance used but not defined");
        return ctx.OpAccessChain(ctx.output_f32, ctx.cull_distances, cull_num);
    }
    default:
        UNREACHABLE();
    }
}

Id OutputAttrPointer(EmitContext& ctx, IR::Attribute attr, u32 element) {
    if (IR::IsParam(attr)) {
        const u32 attr_index{u32(attr) - u32(IR::Attribute::Param0)};
        if (ctx.stage == Stage::Local && ctx.runtime_info.ls_info.links_with_tcs) {
            const auto component_ptr = ctx.TypePointer(spv::StorageClass::Output, ctx.F32[1]);
            return ctx.OpAccessChain(component_ptr, ctx.output_attr_array, ctx.ConstU32(attr_index),
                                     ctx.ConstU32(element));
        } else {
            const auto& info{ctx.output_params.at(attr_index)};
            ASSERT(info.num_components > 0);
            if (info.num_components == 1) {
                return info.id;
            } else {
                return ctx.OpAccessChain(info.pointer_type, info.id, ctx.ConstU32(element));
            }
        }
    }
    if (IR::IsMrt(attr)) {
        const u32 index{u32(attr) - u32(IR::Attribute::RenderTarget0)};
        const auto& info{ctx.frag_outputs.at(index)};
        if (info.num_components == 1) {
            return info.id;
        } else {
            return ctx.OpAccessChain(info.pointer_type, info.id, ctx.ConstU32(element));
        }
    }
    switch (attr) {
    case IR::Attribute::Position0: {
        return ctx.OpAccessChain(ctx.output_f32, ctx.output_position, ctx.ConstU32(element));
    }
    case IR::Attribute::Position1:
    case IR::Attribute::Position2:
    case IR::Attribute::Position3: {
        const u32 index = u32(attr) - u32(IR::Attribute::Position1);
        return VsOutputAttrPointer(ctx, ctx.runtime_info.vs_info.outputs[index][element]);
    }
    case IR::Attribute::Depth:
        return ctx.frag_depth;
    default:
        throw NotImplementedException("Write attribute {}", attr);
    }
}

std::pair<Id, bool> OutputAttrComponentType(EmitContext& ctx, IR::Attribute attr) {
    if (IR::IsParam(attr)) {
        if (ctx.stage == Stage::Local && ctx.runtime_info.ls_info.links_with_tcs) {
            return {ctx.F32[1], false};
        } else {
            const u32 index{u32(attr) - u32(IR::Attribute::Param0)};
            const auto& info{ctx.output_params.at(index)};
            return {info.component_type, info.is_integer};
        }
    }
    if (IR::IsMrt(attr)) {
        const u32 index{u32(attr) - u32(IR::Attribute::RenderTarget0)};
        const auto& info{ctx.frag_outputs.at(index)};
        return {info.component_type, info.is_integer};
    }
    switch (attr) {
    case IR::Attribute::Position0:
    case IR::Attribute::Position1:
    case IR::Attribute::Position2:
    case IR::Attribute::Position3:
    case IR::Attribute::Depth:
        return {ctx.F32[1], false};
    default:
        throw NotImplementedException("Write attribute {}", attr);
    }
}
} // Anonymous namespace

Id EmitGetUserData(EmitContext& ctx, IR::ScalarReg reg) {
    const u32 index = ctx.binding.user_data + ctx.info.ud_mask.Index(reg);
    const u32 half = PushData::UdRegsIndex + (index >> 2);
    const Id ud_ptr{ctx.OpAccessChain(ctx.TypePointer(spv::StorageClass::PushConstant, ctx.U32[1]),
                                      ctx.push_data_block, ctx.ConstU32(half),
                                      ctx.ConstU32(index & 3))};
    const Id ud_reg{ctx.OpLoad(ctx.U32[1], ud_ptr)};
    ctx.Name(ud_reg, fmt::format("ud_{}", u32(reg)));
    return ud_reg;
}

void EmitGetThreadBitScalarReg(EmitContext& ctx) {
    UNREACHABLE_MSG("Unreachable instruction");
}

void EmitSetThreadBitScalarReg(EmitContext& ctx) {
    UNREACHABLE_MSG("Unreachable instruction");
}

void EmitGetScalarRegister(EmitContext&) {
    UNREACHABLE_MSG("Unreachable instruction");
}

void EmitSetScalarRegister(EmitContext&) {
    UNREACHABLE_MSG("Unreachable instruction");
}

void EmitGetVectorRegister(EmitContext& ctx) {
    UNREACHABLE_MSG("Unreachable instruction");
}

void EmitSetVectorRegister(EmitContext& ctx) {
    UNREACHABLE_MSG("Unreachable instruction");
}

void EmitSetGotoVariable(EmitContext&) {
    UNREACHABLE_MSG("Unreachable instruction");
}

void EmitGetGotoVariable(EmitContext&) {
    UNREACHABLE_MSG("Unreachable instruction");
}

using BufferAlias = EmitContext::BufferAlias;

Id EmitReadConst(EmitContext& ctx, IR::Inst* inst) {
    const u32 flatbuf_off_dw = inst->Flags<u32>();
    const auto& srt_flatbuf = ctx.buffers.back();
    ASSERT(srt_flatbuf.binding >= 0 && flatbuf_off_dw > 0 &&
           srt_flatbuf.buffer_type == BufferType::ReadConstUbo);
    const auto [id, pointer_type] = srt_flatbuf[BufferAlias::U32];
    const Id ptr{
        ctx.OpAccessChain(pointer_type, id, ctx.u32_zero_value, ctx.ConstU32(flatbuf_off_dw))};
    return ctx.OpLoad(ctx.U32[1], ptr);
}

Id EmitReadConstBuffer(EmitContext& ctx, u32 handle, Id index) {
    const auto& buffer = ctx.buffers[handle];
    index = ctx.OpIAdd(ctx.U32[1], index, buffer.offset_dwords);
    const auto [id, pointer_type] = buffer[BufferAlias::U32];
    const Id ptr{ctx.OpAccessChain(pointer_type, id, ctx.u32_zero_value, index)};
    return ctx.OpLoad(ctx.U32[1], ptr);
}

Id EmitReadStepRate(EmitContext& ctx, int rate_idx) {
    return ctx.OpLoad(
        ctx.U32[1], ctx.OpAccessChain(ctx.TypePointer(spv::StorageClass::PushConstant, ctx.U32[1]),
                                      ctx.push_data_block,
                                      rate_idx == 0 ? ctx.u32_zero_value : ctx.u32_one_value));
}

static Id EmitGetAttributeForGeometry(EmitContext& ctx, IR::Attribute attr, u32 comp, Id index) {
    if (IR::IsPosition(attr)) {
        ASSERT(attr == IR::Attribute::Position0);
        const auto position_arr_ptr = ctx.TypePointer(spv::StorageClass::Input, ctx.F32[4]);
        const auto pointer{ctx.OpAccessChain(position_arr_ptr, ctx.gl_in, index, ctx.ConstU32(0u))};
        const auto position_comp_ptr = ctx.TypePointer(spv::StorageClass::Input, ctx.F32[1]);
        return ctx.OpLoad(ctx.F32[1],
                          ctx.OpAccessChain(position_comp_ptr, pointer, ctx.ConstU32(comp)));
    }

    if (IR::IsParam(attr)) {
        const u32 param_id{u32(attr) - u32(IR::Attribute::Param0)};
        const auto param = ctx.input_params.at(param_id).id;
        const auto param_arr_ptr = ctx.TypePointer(spv::StorageClass::Input, ctx.F32[4]);
        const auto pointer{ctx.OpAccessChain(param_arr_ptr, param, index)};
        const auto position_comp_ptr = ctx.TypePointer(spv::StorageClass::Input, ctx.F32[1]);
        return ctx.OpLoad(ctx.F32[1],
                          ctx.OpAccessChain(position_comp_ptr, pointer, ctx.ConstU32(comp)));
    }
    UNREACHABLE();
}

Id EmitGetAttribute(EmitContext& ctx, IR::Attribute attr, u32 comp, Id index) {
    if (ctx.info.l_stage == LogicalStage::Geometry) {
        return EmitGetAttributeForGeometry(ctx, attr, comp, index);
    } else if (ctx.info.l_stage == LogicalStage::TessellationControl ||
               ctx.info.l_stage == LogicalStage::TessellationEval) {
        if (IR::IsTessCoord(attr)) {
            const u32 component = attr == IR::Attribute::TessellationEvaluationPointU ? 0 : 1;
            const auto component_ptr = ctx.TypePointer(spv::StorageClass::Input, ctx.F32[1]);
            const auto pointer{
                ctx.OpAccessChain(component_ptr, ctx.tess_coord, ctx.ConstU32(component))};
            return ctx.OpLoad(ctx.F32[1], pointer);
        }
        UNREACHABLE();
    }

    if (IR::IsParam(attr)) {
        const u32 index{u32(attr) - u32(IR::Attribute::Param0)};
        const auto& param{ctx.input_params.at(index)};
        if (param.buffer_handle >= 0) {
            const auto step_rate = EmitReadStepRate(ctx, param.id.value);
            const auto offset = ctx.OpIAdd(
                ctx.U32[1],
                ctx.OpIMul(
                    ctx.U32[1],
                    ctx.OpUDiv(ctx.U32[1], ctx.OpLoad(ctx.U32[1], ctx.instance_id), step_rate),
                    ctx.ConstU32(param.num_components)),
                ctx.ConstU32(comp));
            return EmitReadConstBuffer(ctx, param.buffer_handle, offset);
        }

        Id result;
        if (param.is_loaded) {
            // Attribute is either default or manually interpolated. The id points to an already
            // loaded vector.
            result = ctx.OpCompositeExtract(param.component_type, param.id, comp);
        } else if (param.num_components > 1) {
            // Attribute is a vector and we need to access a specific component.
            const Id pointer{ctx.OpAccessChain(param.pointer_type, param.id, ctx.ConstU32(comp))};
            result = ctx.OpLoad(param.component_type, pointer);
        } else {
            // Attribute is a single float or interger, simply load it.
            result = ctx.OpLoad(param.component_type, param.id);
        }
        if (param.is_integer) {
            result = ctx.OpBitcast(ctx.F32[1], result);
        }
        return result;
    }

    switch (attr) {
    case IR::Attribute::FragCoord: {
        const Id coord = ctx.OpLoad(
            ctx.F32[1], ctx.OpAccessChain(ctx.input_f32, ctx.frag_coord, ctx.ConstU32(comp)));
        if (comp == 3) {
            return ctx.OpFDiv(ctx.F32[1], ctx.ConstF32(1.f), coord);
        }
        return coord;
    }
    case IR::Attribute::TessellationEvaluationPointU:
        return ctx.OpLoad(ctx.F32[1],
                          ctx.OpAccessChain(ctx.input_f32, ctx.tess_coord, ctx.u32_zero_value));
    case IR::Attribute::TessellationEvaluationPointV:
        return ctx.OpLoad(ctx.F32[1],
                          ctx.OpAccessChain(ctx.input_f32, ctx.tess_coord, ctx.ConstU32(1U)));
    default:
        UNREACHABLE_MSG("Read attribute {}", attr);
    }
}

Id EmitGetAttributeU32(EmitContext& ctx, IR::Attribute attr, u32 comp) {
    switch (attr) {
    case IR::Attribute::VertexId:
        return ctx.OpLoad(ctx.U32[1], ctx.vertex_index);
    case IR::Attribute::InstanceId:
        return ctx.OpLoad(ctx.U32[1], ctx.instance_id);
    case IR::Attribute::InstanceId0:
        return EmitReadStepRate(ctx, 0);
    case IR::Attribute::InstanceId1:
        return EmitReadStepRate(ctx, 1);
    case IR::Attribute::WorkgroupIndex:
        return ctx.workgroup_index_id;
    case IR::Attribute::WorkgroupId:
        return ctx.OpCompositeExtract(ctx.U32[1], ctx.OpLoad(ctx.U32[3], ctx.workgroup_id), comp);
    case IR::Attribute::LocalInvocationId:
        return ctx.OpCompositeExtract(ctx.U32[1], ctx.OpLoad(ctx.U32[3], ctx.local_invocation_id),
                                      comp);
    case IR::Attribute::IsFrontFace:
        return ctx.OpSelect(ctx.U32[1], ctx.OpLoad(ctx.U1[1], ctx.front_facing), ctx.u32_one_value,
                            ctx.u32_zero_value);
    case IR::Attribute::PrimitiveId:
        return ctx.OpLoad(ctx.U32[1], ctx.primitive_id);
    case IR::Attribute::InvocationId:
        ASSERT(ctx.info.l_stage == LogicalStage::Geometry ||
               ctx.info.l_stage == LogicalStage::TessellationControl);
        return ctx.OpLoad(ctx.U32[1], ctx.invocation_id);
    case IR::Attribute::PatchVertices:
        ASSERT(ctx.info.l_stage == LogicalStage::TessellationControl);
        return ctx.OpLoad(ctx.U32[1], ctx.patch_vertices);
    case IR::Attribute::PackedHullInvocationInfo: {
        ASSERT(ctx.info.l_stage == LogicalStage::TessellationControl);
        // [0:8]: patch id within VGT
        // [8:12]: output control point id
        // But 0:8 should be treated as 0 for attribute addressing purposes
        if (ctx.runtime_info.hs_info.IsPassthrough()) {
            // Gcn shader would run with 1 thread, but we need to run a thread for
            // each output control point.
            // If Gcn shader uses this value, we should make sure all threads in the
            // Vulkan shader use 0
            return ctx.ConstU32(0u);
        } else {
            const Id invocation_id = ctx.OpLoad(ctx.U32[1], ctx.invocation_id);
            return ctx.OpShiftLeftLogical(ctx.U32[1], invocation_id, ctx.ConstU32(8u));
        }
    }
    default:
        UNREACHABLE_MSG("Read U32 attribute {}", attr);
    }
}

void EmitSetAttribute(EmitContext& ctx, IR::Attribute attr, Id value, u32 element) {
    if (attr == IR::Attribute::Position1) {
        LOG_WARNING(Render_Vulkan, "Ignoring pos1 export");
        return;
    }
    const Id pointer{OutputAttrPointer(ctx, attr, element)};
    const auto component_type{OutputAttrComponentType(ctx, attr)};
    if (component_type.second) {
        ctx.OpStore(pointer, ctx.OpBitcast(component_type.first, value));
    } else {
        ctx.OpStore(pointer, value);
    }
}

Id EmitGetTessGenericAttribute(EmitContext& ctx, Id vertex_index, Id attr_index, Id comp_index) {
    const auto attr_comp_ptr = ctx.TypePointer(spv::StorageClass::Input, ctx.F32[1]);
    return ctx.OpLoad(ctx.F32[1], ctx.OpAccessChain(attr_comp_ptr, ctx.input_attr_array,
                                                    vertex_index, attr_index, comp_index));
}

Id EmitReadTcsGenericOuputAttribute(EmitContext& ctx, Id vertex_index, Id attr_index,
                                    Id comp_index) {
    const auto attr_comp_ptr = ctx.TypePointer(spv::StorageClass::Output, ctx.F32[1]);
    return ctx.OpLoad(ctx.F32[1], ctx.OpAccessChain(attr_comp_ptr, ctx.output_attr_array,
                                                    vertex_index, attr_index, comp_index));
}

void EmitSetTcsGenericAttribute(EmitContext& ctx, Id value, Id attr_index, Id comp_index) {
    // Implied vertex index is invocation_id
    const auto component_ptr = ctx.TypePointer(spv::StorageClass::Output, ctx.F32[1]);
    Id pointer =
        ctx.OpAccessChain(component_ptr, ctx.output_attr_array,
                          ctx.OpLoad(ctx.U32[1], ctx.invocation_id), attr_index, comp_index);
    ctx.OpStore(pointer, value);
}

Id EmitGetPatch(EmitContext& ctx, IR::Patch patch) {
    const u32 index{IR::GenericPatchIndex(patch)};
    const Id element{ctx.ConstU32(IR::GenericPatchElement(patch))};
    const Id type{ctx.l_stage == LogicalStage::TessellationControl ? ctx.output_f32
                                                                   : ctx.input_f32};
    const Id pointer{ctx.OpAccessChain(type, ctx.patches.at(index), element)};
    return ctx.OpLoad(ctx.F32[1], pointer);
}

void EmitSetPatch(EmitContext& ctx, IR::Patch patch, Id value) {
    const Id pointer{[&] {
        if (IR::IsGeneric(patch)) {
            const u32 index{IR::GenericPatchIndex(patch)};
            const Id element{ctx.ConstU32(IR::GenericPatchElement(patch))};
            return ctx.OpAccessChain(ctx.output_f32, ctx.patches.at(index), element);
        }
        switch (patch) {
        case IR::Patch::TessellationLodLeft:
        case IR::Patch::TessellationLodRight:
        case IR::Patch::TessellationLodTop:
        case IR::Patch::TessellationLodBottom: {
            const u32 index{static_cast<u32>(patch) - u32(IR::Patch::TessellationLodLeft)};
            const Id index_id{ctx.ConstU32(index)};
            return ctx.OpAccessChain(ctx.output_f32, ctx.output_tess_level_outer, index_id);
        }
        case IR::Patch::TessellationLodInteriorU:
            return ctx.OpAccessChain(ctx.output_f32, ctx.output_tess_level_inner,
                                     ctx.u32_zero_value);
        case IR::Patch::TessellationLodInteriorV:
            return ctx.OpAccessChain(ctx.output_f32, ctx.output_tess_level_inner, ctx.ConstU32(1u));
        default:
            UNREACHABLE_MSG("Patch {}", u32(patch));
        }
    }()};
    ctx.OpStore(pointer, value);
}

template <u32 N, BufferAlias alias>
static Id EmitLoadBufferB32xN(EmitContext& ctx, u32 handle, Id address) {
    const auto& spv_buffer = ctx.buffers[handle];
    if (Sirit::ValidId(spv_buffer.offset)) {
        address = ctx.OpIAdd(ctx.U32[1], address, spv_buffer.offset);
    }
    const Id index = ctx.OpShiftRightLogical(ctx.U32[1], address, ctx.ConstU32(2u));
    const auto& data_types = alias == BufferAlias::U32 ? ctx.U32 : ctx.F32;
    const auto [id, pointer_type] = spv_buffer[alias];
    if constexpr (N == 1) {
        const Id ptr{ctx.OpAccessChain(pointer_type, id, ctx.u32_zero_value, index)};
        return ctx.OpLoad(data_types[1], ptr);
    } else {
        boost::container::static_vector<Id, N> ids;
        for (u32 i = 0; i < N; i++) {
            const Id index_i = ctx.OpIAdd(ctx.U32[1], index, ctx.ConstU32(i));
            const Id ptr{ctx.OpAccessChain(pointer_type, id, ctx.u32_zero_value, index_i)};
            ids.push_back(ctx.OpLoad(data_types[1], ptr));
        }
        return ctx.OpCompositeConstruct(data_types[N], ids);
    }
}

Id EmitLoadBufferU8(EmitContext& ctx, IR::Inst*, u32 handle, Id address) {
    const auto& spv_buffer = ctx.buffers[handle];
    if (Sirit::ValidId(spv_buffer.offset)) {
        address = ctx.OpIAdd(ctx.U32[1], address, spv_buffer.offset);
    }
    const auto [id, pointer_type] = spv_buffer[BufferAlias::U8];
    const Id ptr{ctx.OpAccessChain(pointer_type, id, ctx.u32_zero_value, address)};
    return ctx.OpUConvert(ctx.U32[1], ctx.OpLoad(ctx.U8, ptr));
}

Id EmitLoadBufferU16(EmitContext& ctx, IR::Inst*, u32 handle, Id address) {
    const auto& spv_buffer = ctx.buffers[handle];
    if (Sirit::ValidId(spv_buffer.offset)) {
        address = ctx.OpIAdd(ctx.U32[1], address, spv_buffer.offset);
    }
    const auto [id, pointer_type] = spv_buffer[BufferAlias::U16];
    const Id index = ctx.OpShiftRightLogical(ctx.U32[1], address, ctx.ConstU32(1u));
    const Id ptr{ctx.OpAccessChain(pointer_type, id, ctx.u32_zero_value, index)};
    return ctx.OpUConvert(ctx.U32[1], ctx.OpLoad(ctx.U16, ptr));
}

Id EmitLoadBufferU32(EmitContext& ctx, IR::Inst*, u32 handle, Id address) {
    return EmitLoadBufferB32xN<1, BufferAlias::U32>(ctx, handle, address);
}

Id EmitLoadBufferU32x2(EmitContext& ctx, IR::Inst*, u32 handle, Id address) {
    return EmitLoadBufferB32xN<2, BufferAlias::U32>(ctx, handle, address);
}

Id EmitLoadBufferU32x3(EmitContext& ctx, IR::Inst*, u32 handle, Id address) {
    return EmitLoadBufferB32xN<3, BufferAlias::U32>(ctx, handle, address);
}

Id EmitLoadBufferU32x4(EmitContext& ctx, IR::Inst*, u32 handle, Id address) {
    return EmitLoadBufferB32xN<4, BufferAlias::U32>(ctx, handle, address);
}

Id EmitLoadBufferF32(EmitContext& ctx, IR::Inst* inst, u32 handle, Id address) {
    return EmitLoadBufferB32xN<1, BufferAlias::F32>(ctx, handle, address);
}

Id EmitLoadBufferF32x2(EmitContext& ctx, IR::Inst* inst, u32 handle, Id address) {
    return EmitLoadBufferB32xN<2, BufferAlias::F32>(ctx, handle, address);
}

Id EmitLoadBufferF32x3(EmitContext& ctx, IR::Inst* inst, u32 handle, Id address) {
    return EmitLoadBufferB32xN<3, BufferAlias::F32>(ctx, handle, address);
}

Id EmitLoadBufferF32x4(EmitContext& ctx, IR::Inst* inst, u32 handle, Id address) {
    return EmitLoadBufferB32xN<4, BufferAlias::F32>(ctx, handle, address);
}

Id EmitLoadBufferFormatF32(EmitContext& ctx, IR::Inst* inst, u32 handle, Id address) {
    UNREACHABLE_MSG("SPIR-V instruction");
}

template <u32 N, BufferAlias alias>
static void EmitStoreBufferB32xN(EmitContext& ctx, u32 handle, Id address, Id value) {
    const auto& spv_buffer = ctx.buffers[handle];
    if (Sirit::ValidId(spv_buffer.offset)) {
        address = ctx.OpIAdd(ctx.U32[1], address, spv_buffer.offset);
    }
    const Id index = ctx.OpShiftRightLogical(ctx.U32[1], address, ctx.ConstU32(2u));
    const auto& data_types = alias == BufferAlias::U32 ? ctx.U32 : ctx.F32;
    const auto [id, pointer_type] = spv_buffer[alias];
    if constexpr (N == 1) {
        const Id ptr{ctx.OpAccessChain(pointer_type, id, ctx.u32_zero_value, index)};
        ctx.OpStore(ptr, value);
    } else {
        for (u32 i = 0; i < N; i++) {
            const Id index_i = ctx.OpIAdd(ctx.U32[1], index, ctx.ConstU32(i));
            const Id ptr = ctx.OpAccessChain(pointer_type, id, ctx.u32_zero_value, index_i);
            ctx.OpStore(ptr, ctx.OpCompositeExtract(data_types[1], value, i));
        }
    }
}

void EmitStoreBufferU8(EmitContext& ctx, IR::Inst*, u32 handle, Id address, Id value) {
    const auto& spv_buffer = ctx.buffers[handle];
    if (Sirit::ValidId(spv_buffer.offset)) {
        address = ctx.OpIAdd(ctx.U32[1], address, spv_buffer.offset);
    }
    const auto [id, pointer_type] = spv_buffer[BufferAlias::U8];
    const Id ptr{ctx.OpAccessChain(pointer_type, id, ctx.u32_zero_value, address)};
    ctx.OpStore(ptr, ctx.OpUConvert(ctx.U8, value));
}

void EmitStoreBufferU16(EmitContext& ctx, IR::Inst*, u32 handle, Id address, Id value) {
    const auto& spv_buffer = ctx.buffers[handle];
    if (Sirit::ValidId(spv_buffer.offset)) {
        address = ctx.OpIAdd(ctx.U32[1], address, spv_buffer.offset);
    }
    const auto [id, pointer_type] = spv_buffer[BufferAlias::U16];
    const Id index = ctx.OpShiftRightLogical(ctx.U32[1], address, ctx.ConstU32(1u));
    const Id ptr{ctx.OpAccessChain(pointer_type, id, ctx.u32_zero_value, index)};
    ctx.OpStore(ptr, ctx.OpUConvert(ctx.U16, value));
}

void EmitStoreBufferU32(EmitContext& ctx, IR::Inst*, u32 handle, Id address, Id value) {
    EmitStoreBufferB32xN<1, BufferAlias::U32>(ctx, handle, address, value);
}

void EmitStoreBufferU32x2(EmitContext& ctx, IR::Inst*, u32 handle, Id address, Id value) {
    EmitStoreBufferB32xN<2, BufferAlias::U32>(ctx, handle, address, value);
}

void EmitStoreBufferU32x3(EmitContext& ctx, IR::Inst*, u32 handle, Id address, Id value) {
    EmitStoreBufferB32xN<3, BufferAlias::U32>(ctx, handle, address, value);
}

void EmitStoreBufferU32x4(EmitContext& ctx, IR::Inst*, u32 handle, Id address, Id value) {
    EmitStoreBufferB32xN<4, BufferAlias::U32>(ctx, handle, address, value);
}

void EmitStoreBufferF32(EmitContext& ctx, IR::Inst* inst, u32 handle, Id address, Id value) {
    EmitStoreBufferB32xN<1, BufferAlias::F32>(ctx, handle, address, value);
}

void EmitStoreBufferF32x2(EmitContext& ctx, IR::Inst* inst, u32 handle, Id address, Id value) {
    EmitStoreBufferB32xN<2, BufferAlias::F32>(ctx, handle, address, value);
}

void EmitStoreBufferF32x3(EmitContext& ctx, IR::Inst* inst, u32 handle, Id address, Id value) {
    EmitStoreBufferB32xN<3, BufferAlias::F32>(ctx, handle, address, value);
}

void EmitStoreBufferF32x4(EmitContext& ctx, IR::Inst* inst, u32 handle, Id address, Id value) {
    EmitStoreBufferB32xN<4, BufferAlias::F32>(ctx, handle, address, value);
}

void EmitStoreBufferFormatF32(EmitContext& ctx, IR::Inst* inst, u32 handle, Id address, Id value) {
    UNREACHABLE_MSG("SPIR-V instruction");
}

} // namespace Shader::Backend::SPIRV
