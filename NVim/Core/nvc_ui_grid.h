//
//  nvc_ui_grid.h
//  NVim
//
//  Created by wizjin on 2023/10/15.
//

#ifndef __NVC_UI_GRID_H__
#define __NVC_UI_GRID_H__

#include <vector>
#include "nvc_ui_font.h"

#ifdef __cplusplus
extern "C" {
#endif


#ifdef __cplusplus
namespace nvc {

class UIRender;

class UIGrid {
private:
    UISize              m_size;
    UIPoint             m_cursor;
    std::vector<UICell> m_cells;
public:
    explicit UIGrid(const UISize& size);
    inline const UISize& size(void) const { return m_size; }
    inline const UIPoint& cursor(void) const { return m_cursor; }
    inline bool has_cursor(void) const { return m_cursor.x >= 0 && m_cursor.y >= 0; }
    inline void set_cursor(const UIPoint& cursor) { m_cursor = cursor; }
    void clear(void);
    void resize(const UISize& size);
    void skip_cell(const UIPoint& pt);
    UIRect scroll(const UIRect& rect, int32_t rows);
    void update(const UIPoint& pt, int32_t count, UnicodeChar ch, int32_t hl_id);
    void draw(UIRender& render, const UIRect& dirty, const UIPoint& offset) const;
};


}
#endif

#ifdef __cplusplus
}
#endif

#endif /* __NVC_UI_GRID_H__ */
