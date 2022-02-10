/*
* Copyright (c) 2021, Intel Corporation
*
* Permission is hereby granted, free of charge, to any person obtaining a
* copy of this software and associated documentation files (the "Software"),
* to deal in the Software without restriction, including without limitation
* the rights to use, copy, modify, merge, publish, distribute, sublicense,
* and/or sell copies of the Software, and to permit persons to whom the
* Software is furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included
* in all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
* OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
* THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
* OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
* ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
* OTHER DEALINGS IN THE SOFTWARE.
*/
//!
//! \file     decode_vp9_packet_xe_m_base.cpp
//! \brief    Defines the interface for vp9 decode packet
//!
#include "codechal_utilities.h"
#include "decode_vp9_packet_xe_m_base.h"
#include "decode_status_report_defs.h"
#include "decode_predication_packet.h"
#include "decode_marker_packet.h"

namespace decode {

Vp9DecodePktXe_M_Base::Vp9DecodePktXe_M_Base(MediaPipeline *pipeline, MediaTask *task, CodechalHwInterface *hwInterface)
    : CmdPacket(task)
{
    if (pipeline != nullptr)
    {
        m_statusReport   = pipeline->GetStatusReportInstance();
        m_featureManager = pipeline->GetFeatureManager();
        m_vp9Pipeline    = dynamic_cast<Vp9Pipeline *>(pipeline);
    }
    if (hwInterface != nullptr)
    {
        m_hwInterface    = hwInterface;
        m_miInterface    = hwInterface->GetMiInterface();
        m_osInterface    = hwInterface->GetOsInterface();
        m_vdencInterface = hwInterface->GetVdencInterface();
    }
}

MOS_STATUS Vp9DecodePktXe_M_Base::Init()
{
    DECODE_FUNC_CALL();
    DECODE_CHK_NULL(m_miInterface);
    DECODE_CHK_NULL(m_statusReport);
    DECODE_CHK_NULL(m_featureManager);
    DECODE_CHK_NULL(m_vp9Pipeline);
    DECODE_CHK_NULL(m_osInterface);
    DECODE_CHK_NULL(m_vdencInterface);

    DECODE_CHK_STATUS(CmdPacket::Init());

    m_vp9BasicFeature = dynamic_cast<Vp9BasicFeature*>(m_featureManager->GetFeature(FeatureIDs::basicFeature));
    DECODE_CHK_NULL(m_vp9BasicFeature);

    m_allocator = m_vp9Pipeline->GetDecodeAllocator();
    DECODE_CHK_NULL(m_allocator);

    return MOS_STATUS_SUCCESS;
}

MOS_STATUS Vp9DecodePktXe_M_Base::Prepare()
{
    DECODE_FUNC_CALL();

    m_phase = static_cast<DecodePhase *>(m_vp9Pipeline->GetComponentState());
    DECODE_CHK_NULL(m_phase);

    DECODE_CHK_NULL(m_vp9BasicFeature);
    DECODE_CHK_NULL(m_vp9BasicFeature->m_vp9PicParams);
    m_vp9PicParams = m_vp9BasicFeature->m_vp9PicParams;

    return MOS_STATUS_SUCCESS;
}

MOS_STATUS Vp9DecodePktXe_M_Base::Destroy()
{
    m_statusReport->UnregistObserver(this);
    return MOS_STATUS_SUCCESS;
}

void Vp9DecodePktXe_M_Base::SetPerfTag(CODECHAL_MODE mode, uint16_t picCodingType)
{
    DECODE_FUNC_CALL();

    uint16_t perfTag = ((mode << 4) & 0xF0) | (picCodingType & 0xF);
    m_osInterface->pfnIncPerfFrameID(m_osInterface);
    m_osInterface->pfnSetPerfTag(m_osInterface, perfTag);
    m_osInterface->pfnResetPerfBufferID(m_osInterface);
}

MOS_STATUS Vp9DecodePktXe_M_Base::AddForceWakeup(MOS_COMMAND_BUFFER& cmdBuffer)
{
    DECODE_FUNC_CALL();

    MHW_MI_FORCE_WAKEUP_PARAMS forceWakeupParams;
    MOS_ZeroMemory(&forceWakeupParams, sizeof(MHW_MI_FORCE_WAKEUP_PARAMS));
    forceWakeupParams.bMFXPowerWellControl      = false;
    forceWakeupParams.bMFXPowerWellControlMask  = true;
    forceWakeupParams.bHEVCPowerWellControl     = true;
    forceWakeupParams.bHEVCPowerWellControlMask = true;

    DECODE_CHK_STATUS(m_miInterface->AddMiForceWakeupCmd(&cmdBuffer, &forceWakeupParams));

    return MOS_STATUS_SUCCESS;
}

MOS_STATUS Vp9DecodePktXe_M_Base::SendPrologWithFrameTracking(MOS_COMMAND_BUFFER& cmdBuffer, bool frameTrackingRequested)
{
    DecodeSubPacket* subPacket = m_vp9Pipeline->GetSubPacket(DecodePacketId(m_vp9Pipeline, markerSubPacketId));
    DecodeMarkerPkt *makerPacket = dynamic_cast<DecodeMarkerPkt*>(subPacket);
    DECODE_CHK_NULL(makerPacket);
    DECODE_CHK_STATUS(makerPacket->Execute(cmdBuffer));

#ifdef _MMC_SUPPORTED
    DecodeMemComp *mmcState = m_vp9Pipeline->GetMmcState();
    bool isMmcEnabled = (mmcState != nullptr && mmcState->IsMmcEnabled());
    if (isMmcEnabled)
    {
        DECODE_CHK_STATUS(mmcState->SendPrologCmd(&cmdBuffer, false));
    }
#endif

    MHW_GENERIC_PROLOG_PARAMS  genericPrologParams;
    MOS_ZeroMemory(&genericPrologParams, sizeof(genericPrologParams));
    genericPrologParams.pOsInterface = m_osInterface;
    genericPrologParams.pvMiInterface = m_miInterface;
#ifdef _MMC_SUPPORTED
    genericPrologParams.bMmcEnabled = isMmcEnabled;
#endif

    DECODE_CHK_STATUS(Mhw_SendGenericPrologCmd(&cmdBuffer, &genericPrologParams));

    subPacket = m_vp9Pipeline->GetSubPacket(DecodePacketId(m_vp9Pipeline, predicationSubPacketId));
    DecodePredicationPkt *predicationPacket = dynamic_cast<DecodePredicationPkt*>(subPacket);
    DECODE_CHK_NULL(predicationPacket);
    DECODE_CHK_STATUS(predicationPacket->Execute(cmdBuffer));

    return MOS_STATUS_SUCCESS;
}

MOS_STATUS Vp9DecodePktXe_M_Base::MiFlush(MOS_COMMAND_BUFFER &cmdBuffer)
{
    DECODE_FUNC_CALL();

    MHW_MI_FLUSH_DW_PARAMS flushDwParams;
    MOS_ZeroMemory(&flushDwParams, sizeof(flushDwParams));
    DECODE_CHK_STATUS(m_miInterface->AddMiFlushDwCmd(&cmdBuffer, &flushDwParams));

    return MOS_STATUS_SUCCESS;
}

MOS_STATUS Vp9DecodePktXe_M_Base::Completed(void *mfxStatus, void *rcsStatus, void *statusReport)
{
    DECODE_FUNC_CALL();

    DECODE_CHK_NULL(mfxStatus);
    DECODE_CHK_NULL(statusReport);

    DecodeStatusMfx *       decodeStatusMfx  = (DecodeStatusMfx *)mfxStatus;
    DecodeStatusReportData *statusReportData = (DecodeStatusReportData *)statusReport;

    MhwVdboxHcpInterface *hcpInterface = m_hwInterface->GetHcpInterface();
    if (hcpInterface != nullptr)
    {
        if ((decodeStatusMfx->m_mmioErrorStatusReg & hcpInterface->GetHcpCabacErrorFlagsMask()) != 0)
        {
            statusReportData->codecStatus    = CODECHAL_STATUS_ERROR;
            statusReportData->numMbsAffected = (decodeStatusMfx->m_mmioMBCountReg & 0xFFFC0000) >> 18;
        }

        statusReportData->frameCrc = decodeStatusMfx->m_mmioFrameCrcReg;
    }

    DECODE_VERBOSEMESSAGE("Index = %d", statusReportData->currDecodedPic.FrameIdx);
    DECODE_VERBOSEMESSAGE("FrameCrc = 0x%x", statusReportData->frameCrc);

    return MOS_STATUS_SUCCESS;
}

MOS_STATUS Vp9DecodePktXe_M_Base::ReadVdboxId(MOS_COMMAND_BUFFER &cmdBuffer)
{
    DECODE_FUNC_CALL();
    DECODE_CHK_NULL(m_phase);
    DECODE_CHK_NULL(m_statusReport);

    uint8_t curPipe = m_phase->GetPipe();
    DECODE_CHK_COND(curPipe >= csInstanceIdMax, "Invalid pipe index.");
    uint32_t csEngineIdOffsetIdx = decode::DecodeStatusReportType::CsEngineIdOffset_0 + curPipe;

    MHW_MI_STORE_REGISTER_MEM_PARAMS params;
    MOS_ZeroMemory(&params, sizeof(MHW_MI_STORE_REGISTER_MEM_PARAMS));

    auto mmioRegistersHcp = m_hwInterface->GetHcpInterface()->GetMmioRegisters(MHW_VDBOX_NODE_1);

    MOS_RESOURCE* osResource = nullptr;
    uint32_t      offset     = 0;
    DECODE_CHK_STATUS(m_statusReport->GetAddress(csEngineIdOffsetIdx, osResource, offset));
    params.presStoreBuffer   = osResource;
    params.dwOffset          = offset;
    params.dwRegister        = mmioRegistersHcp->csEngineIdOffset;

    DECODE_CHK_STATUS(m_miInterface->AddMiStoreRegisterMemCmd(
        &cmdBuffer,
        &params));

    return MOS_STATUS_SUCCESS;
}

MOS_STATUS Vp9DecodePktXe_M_Base::ReadHcpStatus(MediaStatusReport *statusReport, MOS_COMMAND_BUFFER &cmdBuffer)
{
    DECODE_FUNC_CALL();

    MOS_STATUS eStatus = MOS_STATUS_SUCCESS;

    DECODE_CHK_NULL(statusReport);

    MOS_RESOURCE *osResource = nullptr;
    uint32_t      offset     = 0;

    MHW_MI_STORE_REGISTER_MEM_PARAMS params;
    MOS_ZeroMemory(&params, sizeof(MHW_MI_STORE_REGISTER_MEM_PARAMS));

    auto mmioRegistersHcp = m_hwInterface->GetHcpInterface()->GetMmioRegisters(MHW_VDBOX_NODE_1);

    DECODE_CHK_STATUS(statusReport->GetAddress(decode::DecodeStatusReportType::DecErrorStatusOffset, osResource, offset));
    params.presStoreBuffer = osResource;
    params.dwOffset        = offset;
    params.dwRegister      = mmioRegistersHcp->hcpCabacStatusRegOffset;

    DECODE_CHK_STATUS(m_miInterface->AddMiStoreRegisterMemCmd(
        &cmdBuffer,
        &params));

    DECODE_CHK_STATUS(statusReport->GetAddress(decode::DecodeStatusReportType::DecFrameCrcOffset, osResource, offset));
    params.presStoreBuffer = osResource;
    params.dwOffset        = offset;
    params.dwRegister      = mmioRegistersHcp->hcpFrameCrcRegOffset;

    DECODE_CHK_STATUS(m_miInterface->AddMiStoreRegisterMemCmd(
        &cmdBuffer,
        &params));

    DECODE_CHK_STATUS(statusReport->GetAddress(decode::DecodeStatusReportType::DecMBCountOffset, osResource, offset));
    params.presStoreBuffer = osResource;
    params.dwOffset        = offset;
    params.dwRegister      = mmioRegistersHcp->hcpDecStatusRegOffset;

    DECODE_CHK_STATUS(m_miInterface->AddMiStoreRegisterMemCmd(
        &cmdBuffer,
        &params));

    return MOS_STATUS_SUCCESS;
}

MOS_STATUS Vp9DecodePktXe_M_Base::StartStatusReport(uint32_t srType, MOS_COMMAND_BUFFER* cmdBuffer)
{
    DECODE_FUNC_CALL();
    DECODE_CHK_NULL(cmdBuffer);
    DECODE_CHK_STATUS(MediaPacket::StartStatusReport(srType, cmdBuffer));

    MediaPerfProfiler* perfProfiler = MediaPerfProfiler::Instance();
    DECODE_CHK_NULL(perfProfiler);
    DECODE_CHK_STATUS(perfProfiler->AddPerfCollectStartCmd(
        (void*)m_vp9Pipeline, m_osInterface, m_miInterface, cmdBuffer));

    return MOS_STATUS_SUCCESS;
}

MOS_STATUS Vp9DecodePktXe_M_Base::EndStatusReport(uint32_t srType, MOS_COMMAND_BUFFER* cmdBuffer)
{
    DECODE_FUNC_CALL();
    DECODE_CHK_NULL(cmdBuffer);
    DECODE_CHK_STATUS(ReadHcpStatus(m_statusReport, *cmdBuffer));
    DECODE_CHK_STATUS(MediaPacket::EndStatusReport(srType, cmdBuffer));

    SetPerfTag(CODECHAL_DECODE_MODE_VP9VLD, m_vp9BasicFeature->m_pictureCodingType);
    MediaPerfProfiler* perfProfiler = MediaPerfProfiler::Instance();
    DECODE_CHK_NULL(perfProfiler);
    DECODE_CHK_STATUS(perfProfiler->AddPerfCollectEndCmd(
        (void*)m_vp9Pipeline, m_osInterface, m_miInterface, cmdBuffer));

    // Add Mi flush here to ensure end status tag flushed to memory earlier than completed count
    DECODE_CHK_STATUS(MiFlush(*cmdBuffer));

    return MOS_STATUS_SUCCESS;
}

#if USE_CODECHAL_DEBUG_TOOL
MOS_STATUS Vp9DecodePktXe_M_Base::DumpSecondaryCommandBuffer(MOS_COMMAND_BUFFER &cmdBuffer)
{
    DECODE_ASSERT(m_phase != nullptr);
    CodechalDebugInterface *debugInterface = m_vp9Pipeline->GetDebugInterface();
    DECODE_CHK_NULL(debugInterface);
    std::string cmdName = "DEC_secondary_" + std::to_string(m_phase->GetCmdBufIndex());
    DECODE_CHK_STATUS(debugInterface->DumpCmdBuffer(
        &cmdBuffer, CODECHAL_NUM_MEDIA_STATES, cmdName.c_str()));

    return MOS_STATUS_SUCCESS;
}
#endif

}
