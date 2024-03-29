/******************************************************************************/
/* Copyright (C) Teledyne Photometrics. All rights reserved.                  */
/******************************************************************************/
#pragma once
#ifndef PM_FAKE_PARAM_BASE_H
#define PM_FAKE_PARAM_BASE_H

/* Local */
#include "backend/exceptions/ParamGetException.h"
#include "backend/exceptions/ParamSetException.h"
#include "backend/FakeCamera.h"
#include "backend/FakeCameraErrors.h"
#include "backend/Param.h"

/* System */
#include <cassert>

namespace pm {

template<typename T>
class FakeParamBase
{
public:
    explicit FakeParamBase(Param<T>* param)
        : m_param(param)
    {
    }
    virtual ~FakeParamBase()
    {}

protected:
    friend class FakeCamera;

    virtual FakeParamBase<T>& ChangeBaseAttrs(bool avail, uint16_t access)
    {
        assert(access == ACC_READ_ONLY
            || access == ACC_READ_WRITE
            || access == ACC_EXIST_CHECK_ONLY
            || access == ACC_WRITE_ONLY);

        assert(access != ACC_EXIST_CHECK_ONLY
            || ParamTypeFromT<Param<T>>::value == TYPE_BOOLEAN);

        m_param->m_avail->SetValue(avail);
        m_param->m_access->SetValue(access);

        m_baseAttrsSet = true;

        return *this;
    }
    //virtual FakeParamBase<T>& ChangeRangeAttrs(count, def, min, max, inc, ???);

    virtual void CheckGetAccess(int16_t attrId) const
    {
        if (!m_baseAttrsSet || !m_rangeAttrsSet)
        {
            SetError(FakeCameraErrors::NotInitialized);
            throw ParamGetException("Fake parameter not initialized",
                    m_param->m_camera, m_param->m_id, attrId);
        }

        if (attrId == ATTR_AVAIL || attrId == ATTR_ACCESS || attrId == ATTR_TYPE)
            return; // Always accessible

        if (!m_param->m_avail->GetValue())
        {
            SetError(FakeCameraErrors::NotAvailable);
            throw ParamGetException("Parameter not available",
                    m_param->m_camera, m_param->m_id, attrId);
        }
        if (m_param->m_access->GetValue() == ACC_EXIST_CHECK_ONLY
                || (m_param->m_access->GetValue() == ACC_WRITE_ONLY
                    && attrId == ATTR_CURRENT))
        {
            SetError(FakeCameraErrors::CannotGetValue);
            throw ParamGetException("Parameter not readable",
                    m_param->m_camera, m_param->m_id, attrId);
        }
    }
    virtual void CheckSetAccess() const
    {
        if (!m_baseAttrsSet || !m_rangeAttrsSet)
        {
            SetError(FakeCameraErrors::NotInitialized);
            throw ParamSetException("Fake parameter not initialized",
                    m_param->m_camera, m_param->m_id);
        }

        if (!m_param->m_avail->GetValue())
        {
            SetError(FakeCameraErrors::NotAvailable);
            throw ParamSetException("Parameter not available",
                    m_param->m_camera, m_param->m_id);
        }
        if (m_param->m_access->GetValue() == ACC_EXIST_CHECK_ONLY
                || m_param->m_access->GetValue() == ACC_READ_ONLY)
        {
            SetError(FakeCameraErrors::CannotSetValue);
            throw ParamSetException("Parameter not writable",
                    m_param->m_camera, m_param->m_id);
        }
    }

    // Const so it can be set also from const methods
    void SetError(FakeCameraErrors error) const
    {
        static_cast<FakeCamera*>(m_param->m_camera)->SetError(error);
    }

protected:
    Param<T>* m_param{ nullptr };
    bool m_baseAttrsSet{ false };
    bool m_rangeAttrsSet{ false };
};

} // namespace pm

#endif /* PM_FAKE_PARAM_BASE_H */
