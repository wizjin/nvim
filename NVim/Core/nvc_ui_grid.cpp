//
//  nvc_ui_grid.cpp
//  NVim
//
//  Created by wizjin on 2023/10/15.
//

#include "nvc_ui_grid.h"
#include "nvc_ui_context.h"

namespace nvc {

UIGrid::UIGrid(const UISize& size) : m_size(size), m_cursor(-1, -1) {
    m_cells.resize(m_size.area());
}

void UIGrid::clear(void) {
    bzero(m_cells.data(), sizeof(UICell) * m_cells.size());
}

void UIGrid::resize(const UISize& size) {
    m_size = size;
    m_cells.resize(size.area());
}
 
void UIGrid::skip_cell(const UIPoint& pt) {
    if (likely(m_size.contains(pt))) {
        UICell *cell = m_cells.data() + m_size.width * pt.y + pt.x;
        bzero(cell, sizeof(UICell));
        cell->is_skip = true;
        if (likely(pt.x > 0)) {
            cell--;
            if (!cell->is_wide) {
                cell->is_wide = true;
            }
        }
    }
}

UIRect UIGrid::scroll(const UIRect& rect, int32_t rows) {
    if (likely(rows != 0 && rect.width() > 0 && rect.height() > 0)) {
        UICell *cells = m_cells.data() + rect.left();
        size_t size = sizeof(UICell) * rect.width();
        if (rows < 0) {
            int32_t dst = rect.top();
            for (int32_t i = rect.bottom() + rows - 1; i >= dst; i--) {
                memcpy(cells + (i - rows) * m_size.width, cells + i * m_size.width, size);
            }
        } else {
            int32_t dst = rect.bottom() - rows;
            for (int32_t i = rect.top(); i < dst; i++) {
                memcpy(cells + i * m_size.width, cells + (i + rows) * m_size.width, size);
            }
        }
    }
    return rect;
}

void UIGrid::update(const UIPoint& pt, int32_t count, UnicodeChar ch, int32_t hl_id) {
    if (likely(count > 0 && m_size.contains(pt))) {
        UICell cell = {
            .ch = ch,
            .hl_id = hl_id,
            .is_skip = false,
            .is_wide = false,
        };
        int32_t n = std::min(count, m_size.width - pt.x);
        std::fill_n(m_cells.begin() + m_size.width * pt.y + pt.x, n, cell);
    }
}

void UIGrid::draw(UIContext& ctx, CGContextRef context, const UIRect& dirty) const {
    CGPoint pt;
    CGSize size = ctx.cell_size();
    UISize wndSize = ctx.window_size();
    CGFloat font_offset = size.height - ctx.font().font_offset();
    int32_t last_hl_id = 0;
    int32_t width = std::min(wndSize.width, dirty.right());
    int32_t height = std::min(wndSize.height, dirty.bottom());
    bool need_cursor = ctx.mode_enabled() && ctx.show_cursor() && dirty.contains(m_cursor);
    const auto& hl_attrs = ctx.hl_attrs();
    auto& font = ctx.font();
    ui_color_t fg = hl_attrs.default_foreground();
    ui_color_t bg = hl_attrs.default_background();
    ui_color_t sp = hl_attrs.default_special();
    for (int j = dirty.y(); j < height; j++) {
        int i = dirty.x();
        pt.y = size.height * j;
        const UICell *cell = m_cells.data() + j * m_size.width + i;
        if (cell->is_skip && i > 0) {
            cell--; i--;
        }
        for (; i < width; i++) {
            if (!cell->is_skip) {
                pt.x = size.width * i;
                if (cell->hl_id != last_hl_id) {
                    last_hl_id = cell->hl_id;
                    fg = hl_attrs.find_hl_foreground(last_hl_id);
                    bg = hl_attrs.find_hl_background(last_hl_id);
                    sp = hl_attrs.find_hl_special(last_hl_id);
                }
                CGFloat cellWidth = size.width;
                if (cell->is_wide) cellWidth *= 2;
                if (last_hl_id != 0) {
                    ui_set_fill_color(context, bg);
                    CGContextFillRect(context, CGRectMake(pt.x, pt.y, cellWidth, size.height));
                }
                uint32_t tc = fg;
                if (need_cursor && m_cursor.x == i && m_cursor.y == j) {
                    const auto info = ctx.mode().info();
                    if (info != nullptr) {
                        ui_set_fill_color(context, hl_attrs.find_hl_foreground(info->attr_id));
                        if (info->cursor_shape == "block") {
                            CGContextFillRect(context, CGRectMake(pt.x, pt.y, cellWidth, size.height));
                            tc = hl_attrs.find_hl_background(info->attr_id);
                        } else if (info->cursor_shape == "horizontal") {
                            CGContextFillRect(context, CGRectMake(pt.x, pt.y, size.width, info->calc_cell_percentage(size.height)));
                        } else if (info->cursor_shape == "vertical") {
                            CGContextFillRect(context, CGRectMake(pt.x, pt.y, info->calc_cell_percentage(size.width), size.height));
                        }
                    }
                }
                font.draw(context, cell->ch, tc, UIPoint(pt.x, pt.y + font_offset));
            }
            cell++;
        }
    }
    
}

}
