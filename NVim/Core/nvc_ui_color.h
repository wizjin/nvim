//
//  nvc_ui_color.h
//  NVim
//
//  Created by wizjin on 2023/10/15.
//

#ifndef __NVC_UI_COLOR_H__
#define __NVC_UI_COLOR_H__

#include <map>
#include "nvc_util.h"

#ifdef __cplusplus
extern "C" {
#endif


#ifdef __cplusplus
namespace nvc {

#define kNvcUiColorWhite                    ((ui_color_t)0xffffffff)

#define NVC_UI_COLOR_CODE_LIST              \
    NVC_UI_COLOR_CODE(foreground),          \
    NVC_UI_COLOR_CODE(background),          \
    NVC_UI_COLOR_CODE(special),             \
    NVC_UI_COLOR_CODE(reverse),             \
    NVC_UI_COLOR_CODE(italic),              \
    NVC_UI_COLOR_CODE(bold),                \
    NVC_UI_COLOR_CODE(nocombine),           \
    NVC_UI_COLOR_CODE(strikethrough),       \
    NVC_UI_COLOR_CODE(underline),           \
    NVC_UI_COLOR_CODE(undercurl),           \
    NVC_UI_COLOR_CODE(underdouble),         \
    NVC_UI_COLOR_CODE(underdotted),         \
    NVC_UI_COLOR_CODE(underdashed),         \
    NVC_UI_COLOR_CODE(altfont),             \
    NVC_UI_COLOR_CODE(blend)

#define NVC_UI_COLOR_CODE(_code)            ui_color_code_##_code
enum UIColorCode : uint8_t {
    NVC_UI_COLOR_CODE_LIST
};

typedef uint32_t ui_color_t;
typedef std::map<UIColorCode, ui_color_t>   UIColorSet;

class UIColor {
private:
    typedef std::map<int32_t, UIColorSet>       UIColorSetMap;
    typedef std::map<std::string, int32_t>      UIGroupMap;

    UIColorSet      m_default_colors;
    UIColorSetMap   m_hl_attrs;
    UIGroupMap      m_hl_groups;
public:
    ui_color_t default_color(UIColorCode code) const;
    ui_color_t find_hl_color(int32_t hl, UIColorCode code) const;
    const UIColorSet* find_hl_colors(const std::string& name) const;
    inline void default_color(UIColorCode code, ui_color_t color) { m_default_colors[code] = color; }
    inline void update_hl_attrs(int32_t hl, const UIColorSet& colorSet) { m_hl_attrs[hl] = colorSet; }
    inline void update_hl_groups(const std::string& name, int32_t hl) { m_hl_groups[name] = hl; }
    
    static inline void set_fill_color(CGContextRef context, ui_color_t rgb) {
        const uint8_t *c = (const uint8_t *)&rgb;
        CGContextSetRGBFillColor(context, c[2]/255.0, c[1]/255.0, c[0]/255.0, 1.0);
    }
    
    static inline void set_stroke_color(CGContextRef context, ui_color_t rgb) {
        const uint8_t *c = (const uint8_t *)&rgb;
        CGContextSetRGBStrokeColor(context, c[2]/255.0, c[1]/255.0, c[0]/255.0, 1.0);
    }

};

}
#endif

#ifdef __cplusplus
}
#endif

#endif /* __NVC_UI_COLOR_H__ */
