//
//  nvc_ui_grid.cpp
//  NVim
//
//  Created by wizjin on 2023/10/15.
//

#include "nvc_ui_grid.h"
#include "nvc_ui_render.h"

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
    if (likely(rows != 0 && !rect.empty())) {
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

void UIGrid::draw(UIRender& render, const UIRect& dirty) const {
    CGPoint pt;
    CGSize size = render.ctx().cell_size();
    UISize wndSize = render.ctx().window_size();
    int32_t width = std::min(wndSize.width, dirty.right());
    int32_t height = std::min(wndSize.height, dirty.bottom());
    bool need_cursor = render.need_cursor() && dirty.contains(m_cursor);
    for (int j = dirty.y(); j < height; j++) {
        int i = dirty.x();
        pt.y = j * size.height;
        const UICell *cell = m_cells.data() + j * m_size.width + i;
        if (cell->is_skip && i > 0) {
            cell--; i--;
        }
        for (; i < width; i++) {
            if (!cell->is_skip) {
                pt.x = i * size.width;
                const auto cell_hl = render.update_hl_id(cell->hl_id);
                CGFloat cellWidth = size.width;
                if (cell->is_wide) cellWidth *= 2;
                render.draw_background(CGRectMake(pt.x, pt.y, cellWidth, size.height));
                if (need_cursor && m_cursor.x == i && m_cursor.y == j) {
                    const auto info = render.mode_info();
                    if (likely(info != nullptr)) {
                        const auto& hl_attrs = render.hl_attrs();
                        render.set_fill_color(hl_attrs.find_hl_foreground(info->attr_id));
                        switch (info->cursor_shape) {
                            default: break;
                            case ui_cursor_shape_horizontal:
                                render.draw_rect(CGRectMake(pt.x, pt.y, size.width, info->calc_cell_percentage(size.height)));
                                break;
                            case ui_cursor_shape_vertical:
                                render.draw_rect(CGRectMake(pt.x, pt.y, info->calc_cell_percentage(size.width), size.height));
                                break;
                            case ui_cursor_shape_block:
                                render.draw_rect(CGRectMake(pt.x, pt.y, cellWidth, size.height));
                                render.text_color(hl_attrs.find_hl_background(info->attr_id));
                                break;
                        }
                    }
                }
                UIFont &font = render.font();
                CGFloat ypos = pt.y + render.font_offset();
                if (cell_hl == nullptr) {
                    render.draw(font, cell->ch, ui_font_traits_none, UIPoint(pt.x, ypos));
                } else {
                    render.draw(font, cell->ch, cell_hl->traits, UIPoint(pt.x, ypos));
                    if (cell_hl->understyle != ui_under_style_none) {
                        CGFloat underline_position = font.underline_position();
                        CGFloat line_position = ypos - underline_position;
                        render.set_stroke_color(render.stroke_color());
                        render.line_width(font.underline_thickness());
                        switch (cell_hl->understyle) {
                            case ui_under_style_underline:
                                render.line_dash(nullptr, 0);
                                render.draw_line(pt.x, line_position, pt.x + cellWidth, line_position);
                                break;
                            case ui_under_style_undercurl:
                                render.line_dash(nullptr, 0);
                                render.draw_wavy_line(pt.x, line_position, size.width, std::floor(underline_position - 1)*0.5, cell->is_wide);
                                break;
                            case ui_under_style_underdouble:
                                render.line_dash(nullptr, 0);
                                line_position -= render.one_pixel() * 2;
                                render.draw_line(pt.x, line_position, pt.x + cellWidth, line_position);
                                line_position += render.one_pixel() * 4;
                                render.draw_line(pt.x, line_position, pt.x + cellWidth, line_position);
                                break;
                            case ui_under_style_underdotted:
                                render.line_dash((CGFloat[2]){ 1, size.width/3.0 - 1 }, 2);
                                render.draw_line(pt.x, line_position, pt.x + cellWidth, line_position);
                                break;
                            case ui_under_style_underdashed:
                                render.line_dash((CGFloat[4]){ size.width*0.4, size.width*0.3 - 0.5, 1, size.width*0.3 - 0.5 }, 4);
                                render.draw_line(pt.x, line_position, pt.x + cellWidth, line_position);
                                break;
                            default: break;
                        }
                    }
                    if (cell_hl->strikethrough) {
                        CGFloat line_position = pt.y + size.height * 0.5;
                        render.set_stroke_color(render.stroke_color());
                        render.line_dash(nullptr, 0);
                        render.draw_line(pt.x, line_position, pt.x + cellWidth, line_position);
                    }
                }
            }
            cell++;
        }
    }

}

}
