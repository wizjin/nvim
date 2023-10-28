//
//  mvc_ui_mode.h
//  NVim
//
//  Created by wizjin on 2023/10/15.
//

#ifndef __NVC_UI_MODE_H__
#define __NVC_UI_MODE_H__

#include "nvc_util.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
namespace nvc {

enum UICursorShape: uint8_t {
    ui_cursor_shape_none            = 0,
    ui_cursor_shape_horizontal      = 1,
    ui_cursor_shape_vertical        = 2,
    ui_cursor_shape_block           = 3,
};

struct UIModeInfo {
    std::string     name;
    std::string     short_name;
    UICursorShape   cursor_shape;
    int             cell_percentage;
    int             blinkwait;
    int             blinkon;
    int             blinkoff;
    int             attr_id;
    int             attr_id_lm;

    inline CGFloat calc_cell_percentage(int value) const {
        return (CGFloat)(value * cell_percentage)/100.0;
    }
};

typedef std::vector<UIModeInfo> UIModeInfoList;

class UIMode {
private:
    int             m_index;
    bool            m_enabled;
    std::string     m_mode;
    UIModeInfoList  m_infos;
public:
    explicit UIMode();
    inline void enabled(bool value) { m_enabled = value; }
    inline const std::string& current(void) const { return m_mode; }
    inline void infos(const UIModeInfoList& infos) { m_infos = infos; }
    inline void change(const std::string& mode, int index) {
        m_mode = mode;
        m_index = index;
    }
    inline const UIModeInfo *info(void) const {
        const UIModeInfo *res = nullptr;
        if (likely(m_index >= 0 && m_index < m_infos.size())) {
            res = m_infos.data() + m_index;
        }
        return res;
    }
};

}
#endif

#ifdef __cplusplus
}
#endif

#endif /* __NVC_UI_MODE_H__ */
