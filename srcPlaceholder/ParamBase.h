/******************************************************************************/
/* Copyright (C) Teledyne Photometrics. All rights reserved.                  */
/******************************************************************************/
#pragma once
#ifndef PM_PARAM_BASE_H
#define PM_PARAM_BASE_H

/* Local */
#include "backend/ParamValue.h"

/* System */
#include <cstddef> // size_t
#include <cstdint>
#include <functional>
#include <mutex>
#include <map>
#include <memory> // unique_ptr
#include <mutex>
#include <string>
#include <vector>

struct smart_stream_type; // Forward declaration from pvcam.h

namespace pm {

class Camera;

class ParamBase
{
public:
    explicit ParamBase(Camera* camera, uint32_t id);
    virtual ~ParamBase();

public:
    Camera* GetCamera() const;
    uint32_t GetId() const;

    const ParamValueBase* GetValue(int16_t attrId) const;

public: // Base attributes are cached (read only once)
    virtual bool IsAvail() const;
    virtual uint16_t GetAccess() const;
    virtual uint16_t GetType() const;
    virtual const ParamValue<bool>* GetIsAvailValue() const;
    virtual const ParamValue<uint16_t>* GetAccessValue() const;
    virtual const ParamValue<uint16_t>* GetTypeValue() const;

public: // Range attributes are cached (read only once)
    virtual uint32_t GetCount() const;
    //virtual T GetDef() const = 0;
    //virtual T GetMin() const = 0;
    //virtual T GetMax() const = 0;
    //virtual T GetInc() const = 0;
    virtual const ParamValue<uint32_t>* GetCountValue() const;
    virtual const ParamValueBase* GetDefValue() const;
    virtual const ParamValueBase* GetMinValue() const;
    virtual const ParamValueBase* GetMaxValue() const;
    virtual const ParamValueBase* GetIncValue() const;

public: // Current value is also range attr. and cached by default (read only once)
    //virtual T GetCurNoCache() const = 0;
    //virtual T GetCur() const = 0;
    //virtual void SetCur(const T value) = 0;

    virtual const ParamValueBase* GetCurValue() const;
    virtual void SetCurValue(const ParamValueBase* value);

    virtual void SetFromString(const std::string& str);

public: // Reset cache flags so the values are read from camera again
    virtual void ResetCacheAllFlags();
    virtual void ResetCacheRangeFlags();
    virtual void ResetCacheFlag(int16_t attrId);

protected: // Called from const getters, updates mutable variables, thus const
    virtual void UpdateIsAvailCache() const;
    virtual void UpdateAccessCache() const;
    virtual void UpdateTypeCache() const;
    virtual void UpdateCountCache() const;
    virtual void UpdateDefCache() const = 0;
    virtual void UpdateMinCache() const = 0;
    virtual void UpdateMaxCache() const = 0;
    virtual void UpdateIncCache() const = 0;
    virtual void UpdateCurCache() const = 0;

protected:
    virtual void ReadValueCached(void* value, int16_t attrId) const;
    virtual void ReadValue(void* value, int16_t attrId) const;
    virtual void WriteValue(const void* value, const char* valueAsStr);

    // Has to be called from final Param class constructor
    void InitAttrValueMap();

public:
    // allAttrsChanged is false if only current value of this parameter is
    // changed, and true for all depending parameters that could change all
    // attributes.
    // It is always true for dependent read-only parameters.
    // It is always false for parameters changed upon a call to acq. setup
    // function.
    using ChangeHandlerProto = void(ParamBase& param, bool allAttrsChanged);
    using ChangeHandler = std::function<ChangeHandlerProto>;

    // Returns unique handle that is required for unregistration
    uint64_t RegisterChangeHandler(const ChangeHandler& handler);
    void UnregisterChangeHandler(uint64_t handle);

protected:
    struct ChangeHandlerStorage {
        ChangeHandler handler;
        uint64_t handle;
    };
    friend class Camera;
    void InvokeChangeHandlers(bool allAttrsChanged = false);

protected:
    template<typename TT> friend class FakeParamBase;

    Camera* m_camera;
    const uint32_t m_id;

    std::map<int16_t, ParamValueBase*> m_attrValueMap{};

    using UpdateCacheFn = void(ParamBase::*)() const;
    const std::map<int16_t, UpdateCacheFn> m_attrUpdateFnMap{
        { (int16_t)ATTR_AVAIL    , &ParamBase::UpdateIsAvailCache },
        { (int16_t)ATTR_ACCESS   , &ParamBase::UpdateAccessCache },
        { (int16_t)ATTR_TYPE     , &ParamBase::UpdateTypeCache },
        { (int16_t)ATTR_COUNT    , &ParamBase::UpdateCountCache },
        { (int16_t)ATTR_DEFAULT  , &ParamBase::UpdateDefCache },
        { (int16_t)ATTR_MIN      , &ParamBase::UpdateMinCache },
        { (int16_t)ATTR_MAX      , &ParamBase::UpdateMaxCache },
        { (int16_t)ATTR_INCREMENT, &ParamBase::UpdateIncCache },
        { (int16_t)ATTR_CURRENT  , &ParamBase::UpdateCurCache },
    };

    // Some members are mutable due to caching and lazy initialization

    // Base attributes are cached (read only once)
    mutable std::unique_ptr<ParamValue<bool>> m_avail{ nullptr };
    mutable std::unique_ptr<ParamValue<uint16_t>> m_access{ nullptr };
    mutable std::unique_ptr<ParamValue<uint16_t>> m_type{ nullptr };

    // Range attributes are cached (read only once)
    mutable std::unique_ptr<ParamValue<uint32_t>> m_count{ nullptr };
    mutable std::unique_ptr<ParamValueBase> m_def{ nullptr };
    mutable std::unique_ptr<ParamValueBase> m_min{ nullptr };
    mutable std::unique_ptr<ParamValueBase> m_max{ nullptr };
    mutable std::unique_ptr<ParamValueBase> m_inc{ nullptr };
    mutable std::unique_ptr<ParamValueBase> m_cur{ nullptr };

    // Temporary storage required by SetFromString method
    mutable std::unique_ptr<ParamValueBase> m_curTmp{ nullptr };

    mutable std::map<int16_t, bool> m_attrIdCacheSetMap{
        { (int16_t)ATTR_AVAIL    , false },
        { (int16_t)ATTR_ACCESS   , false },
        { (int16_t)ATTR_TYPE     , false },
        { (int16_t)ATTR_COUNT    , false },
        { (int16_t)ATTR_DEFAULT  , false },
        { (int16_t)ATTR_MIN      , false },
        { (int16_t)ATTR_MAX      , false },
        { (int16_t)ATTR_INCREMENT, false },
        { (int16_t)ATTR_CURRENT  , false },
    };

    std::vector<ChangeHandlerStorage> m_changeHandlers{};
    std::mutex                        m_changeHandlersMutex{};
    // Upper half set to m_id in constructor
    uint64_t                          m_changeHandlersNextHandle;
};

} // namespace pm

#endif /* PM_PARAM_BASE_H */
