//
//  nvc_ui_hl.h
//  NVim
//
//  Created by wizjin on 2023/10/17.
//

#ifndef __NVC_UI_HL_H__
#define __NVC_UI_HL_H__

#include <map>
#include "nvc_util.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
namespace nvc {

typedef uint32_t ui_color_t;

#define kNvcUiColorWhite            ((ui_color_t)0xffffffff)

enum UIUnderStyle: uint8_t {
    ui_under_style_none             = 0,
    ui_under_style_underline        = 1,
    ui_under_style_undercurl        = 2,
    ui_under_style_underdouble      = 3,
    ui_under_style_underdotted      = 4,
    ui_under_style_underdashed      = 5,
};

struct UIHLAttr {
    ui_color_t      foreground;
    ui_color_t      background;
    ui_color_t      special;
    uint8_t         blend;
    UIUnderStyle    understyle;
    bool            has_foreground  :1;
    bool            has_background  :1;
    bool            has_special     :1;
    bool            has_blend       :1;
    bool            strikethrough   :1;
    bool            bold            :1;
    bool            italic          :1;
    bool            reverse         :1;
    bool            nocombine       :1;
};

class UIHLAttrGroups {
private:
    typedef std::unordered_map<int32_t, UIHLAttr>       UIHLAttrMap;
    typedef std::unordered_map<std::string, int32_t>    UIGroupMap;

    ui_color_t      m_default_foreground;
    ui_color_t      m_default_background;
    ui_color_t      m_default_special;
    UIHLAttrMap     m_hl_attrs;
    UIGroupMap      m_hl_groups;
public:
    inline ui_color_t default_foreground(void) const { return m_default_foreground; }
    inline ui_color_t default_background(void) const { return m_default_background; }
    inline ui_color_t default_special(void) const { return m_default_special; }
    inline void default_foreground(uint32_t color) { m_default_foreground = color; }
    inline void default_background(uint32_t color) { m_default_background = color; }
    inline void default_special(uint32_t color) { m_default_special = color; }
    inline void update_hl_attrs(int32_t hl, const UIHLAttr& hl_attr) { m_hl_attrs[hl] = hl_attr; }
    inline void update_hl_groups(const std::string& name, int32_t hl) { m_hl_groups[name] = hl; }
    const UIHLAttr* find_hl_attr(const std::string& name) const;
    const UIHLAttr* find_hl_attr(int32_t hl) const;
    ui_color_t find_hl_foreground(int32_t hl) const;
    ui_color_t find_hl_background(int32_t hl) const;
    ui_color_t find_hl_special(int32_t hl) const;
};

}
#endif

#ifdef __cplusplus
}
#endif

#endif /* __NVC_UI_HL_H__ */
