//
//  nvc_ui_color.cpp
//  NVim
//
//  Created by wizjin on 2023/10/15.
//

#include "nvc_ui_color.h"

namespace nvc {

ui_color_t UIColor::default_color(UIColorCode code) const {
    ui_color_t c = 0;
    const auto& p = m_default_colors.find(code);
    if (likely(p != m_default_colors.end())) {
        c = p->second;
    }
    return c;
}

ui_color_t UIColor::find_hl_color(int32_t hl, UIColorCode code) const {
    const auto& p = m_hl_attrs.find(hl);
    if (likely(p != m_hl_attrs.end())) {
        const auto& q = p->second.find(code);
        if (q != p->second.end()) {
            return q->second;
        }
    }
    return default_color(code);
}

const UIColorSet* UIColor::find_hl_colors(const std::string& name) const {
    const UIColorSet* colors = nullptr;
    const auto& p = m_hl_groups.find(name);
    if (likely(p != m_hl_groups.end())) {
        const auto& q = m_hl_attrs.find(p->second);
        if (likely(q != m_hl_attrs.end())) {
            colors = &(q->second);
        }
    }
    return colors;
}

}
