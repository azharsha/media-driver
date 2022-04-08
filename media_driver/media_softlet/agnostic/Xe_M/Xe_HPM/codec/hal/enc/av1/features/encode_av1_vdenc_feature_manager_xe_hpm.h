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
//! \file     encode_av1_vdenc_feature_manager_xe_hpm.h
//! \brief    Defines the common interface for xe hpm av1 vdenc feature manager
//! \details  The encode feature manager is further sub-divided by codec type
//!           this file is for the base interface which is shared by all components.
//!

#ifndef __ENCODE_AV1_VDENC_FEATURE_MANAGER_XE_HPM_H__
#define __ENCODE_AV1_VDENC_FEATURE_MANAGER_XE_HPM_H__
#include "encode_av1_vdenc_feature_manager.h"

namespace encode
{
class EncodeAv1VdencFeatureManagerXe_Hpm : public EncodeAv1VdencFeatureManager
{

public:
    //!
    //! \brief  EncodeAv1VdencFeatureManagerXe_Hpm constructor
    //! \param  [in] allocator
    //!         Pointer to EncodeAllocator
    //! \param  [in] hwInterface
    //!         Pointer to CodechalHwInterface
    //! \param  [in] trackedBuf
    //!         Pointer to TrackedBuffer
    //! \param  [in] recycleBuf
    //!         Pointer to RecycleResource
    //!
     EncodeAv1VdencFeatureManagerXe_Hpm(EncodeAllocator *allocator,
                             CodechalHwInterface *hwInterface,
                             TrackedBuffer       *trackedBuf,
                             RecycleResource     *recycleBuf): EncodeAv1VdencFeatureManager(allocator, hwInterface, trackedBuf, recycleBuf){};

    //!
    //! \brief  EncodeAv1VdencFeatureManagerXe_Hpm deconstructor
    //!
    virtual ~EncodeAv1VdencFeatureManagerXe_Hpm() {}

protected:

    //!
    //! \brief  Create feature const settings
    //! \return MOS_STATUS
    //!         MOS_STATUS_SUCCESS if success, else fail reason
    //!
    MOS_STATUS CreateConstSettings() override;

    //!
    //! \brief  Create features
    //! \param  [in] constsettings
    //!         feature const settings
    //! \return MOS_STATUS
    //!         MOS_STATUS_SUCCESS if success, else fail reason
    //!
    virtual MOS_STATUS CreateFeatures(void *constSettings) override;
MEDIA_CLASS_DEFINE_END(EncodeAv1VdencFeatureManagerXe_Hpm)
};


}
#endif // !__ENCODE_AV1_VDENC_FEATURE_MANAGER_XE_HPM_H__
