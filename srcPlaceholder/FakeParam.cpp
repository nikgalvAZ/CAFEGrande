/******************************************************************************/
/* Copyright (C) Teledyne Photometrics. All rights reserved.                  */
/******************************************************************************/
#include "backend/FakeParam.h"

/* System */
#include <algorithm>

// pm::FakeParamEnum

pm::FakeParamEnum::FakeBase& pm::FakeParamEnum::ChangeRangeAttrs(T def,
        const std::vector<ParamEnumItem>& items)
{
    assert(!items.empty());
    const bool hasDefInItems = HasValue(items, def);
    assert(hasDefInItems);

    // In fake camera this differs from real one.
    // Although min/max/inc are not supported for enum params, we return here
    // true min/max values, not first/last item from the list.
    const auto min = std::min_element(std::begin(items), std::end(items),
                [](ParamEnumItem a, ParamEnumItem b)
                { return a.GetValue() < b.GetValue(); })
            ->GetValue();
    const auto max = std::max_element(std::begin(items), std::end(items),
                [](ParamEnumItem a, ParamEnumItem b)
                { return a.GetValue() < b.GetValue(); })
            ->GetValue();

    m_count->SetValue(static_cast<uint32_t>(items.size()));
    m_def->SetValueT<T>(def);
    m_min->SetValueT<T>(min);
    m_max->SetValueT<T>(max);
    m_inc->SetValueT<T>(0);
    m_cur->SetValueT<T>(def);

    m_items.clear();
    m_names.clear();
    m_values.clear();
    m_valueNameMap.clear();
    m_valueItemMap.clear();
    for (auto& item : items)
    {
        m_items.push_back(item);
        m_values.push_back(item.GetValue());
        m_names.push_back(item.GetName());
        m_valueNameMap[item.GetValue()] = item.GetName();
        m_valueItemMap[item.GetValue()] = item;
    }

    m_rangeAttrsSet = true;

    return *this;
}
