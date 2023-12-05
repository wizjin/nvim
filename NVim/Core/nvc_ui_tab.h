//
//  nvc_ui_tab.h
//  NVim
//
//  Created by wizjin on 2023/10/30.
//

#ifndef __NVC_UI_TAB_H__
#define __NVC_UI_TAB_H__

#include "nvc_util.h"
#include "nvc_rpc.h"
#include <vector>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
namespace nvc {

class UITab {
private:
    nvc_rpc_object_handler_t    m_handler;
    std::string                 m_name;
public:
    inline explicit UITab() : m_handler(0) {}
    inline void handler(nvc_rpc_object_handler_t h) { m_handler = h; }
    inline void name(const std::string& name) { m_name = name; }
    inline bool operator==(const struct UITab& rhs) const {
        return this->m_handler == rhs.m_handler && this->m_name == rhs.m_name;
    }
    inline bool operator!=(const struct UITab& rhs) const {
        return this->m_handler != rhs.m_handler || this->m_name != rhs.m_name;
    }
    
};

typedef std::vector<UITab> UITabs;

class UITabList {
private:
    nvc_rpc_object_handler_t    m_active;
    UITabs                      m_tabs;
public:
    inline explicit UITabList() : m_active(0) {}
    inline const UITabs& tabs(void) const { return m_tabs; }
    inline void update(nvc_rpc_object_handler_t active) { m_active = active; }
    inline void update(nvc_rpc_object_handler_t active, const UITabs& tabs) {
        m_active = active;
        m_tabs = tabs;
    }
};

}
#endif

#ifdef __cplusplus
}
#endif

#endif /* __NVC_UI_TAB_H__ */
