//
//  nvc_ui_hl.cpp
//  NVim
//
//  Created by wizjin on 2023/10/17.
//

#include "nvc_ui_hl.h"

namespace nvc {

ui_color_t UIHLAttrGroups::find_hl_foreground(int32_t hl) const {
    const auto& p = m_hl_attrs.find(hl);
    if (likely(p != m_hl_attrs.end())) {
        if (p->second.has_foreground) {
            return p->second.foreground;
        }
    }
    return default_foreground();
}

ui_color_t UIHLAttrGroups::find_hl_background(int32_t hl) const {
    const auto& p = m_hl_attrs.find(hl);
    if (likely(p != m_hl_attrs.end())) {
        if (p->second.has_background) {
            return p->second.background;
        }
    }
    return default_background();
}

ui_color_t UIHLAttrGroups::find_hl_special(int32_t hl) const {
    const auto& p = m_hl_attrs.find(hl);
    if (likely(p != m_hl_attrs.end())) {
        if (p->second.has_special) {
            return p->second.special;
        }
    }
    return default_special();
}

const UIHLAttr* UIHLAttrGroups::find_hl_attr(const std::string& name) const {
    const UIHLAttr* hl_attr = nullptr;
    const auto& p = m_hl_groups.find(name);
    if (likely(p != m_hl_groups.end())) {
        const auto& q = m_hl_attrs.find(p->second);
        if (likely(q != m_hl_attrs.end())) {
            hl_attr = &q->second;
        }
    }
    return hl_attr;
}


}
