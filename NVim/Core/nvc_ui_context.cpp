//
//  nvc_ui_context.cpp
//  NVim
//
//  Created by wizjin on 2023/10/15.
//

#include "nvc_ui_context.h"

namespace nvc {

UIContext::UIContext(const nvc_ui_callback_t& cb, CGFloat font_size, void *userdata) : m_cb(cb), m_font(font_size), m_userdata(userdata) {
    m_attached = false;
    m_mode_enabled = true;
    m_show_cursor = true;
    m_dirty_rect = CGRectZero;
    bzero(&m_rpc, sizeof(m_rpc));
}

bool UIContext::attach(const CGSize& size) {
    bool res = false;
    if (likely(!m_attached)) {
        m_attached = true;
        update_size(size);
        res = true;
    }
    return res;
}

bool UIContext::detach(void) {
    bool res = false;
    if (likely(m_attached)) {
        m_attached = false;
        res = true;
    }
    return res;
}

void UIContext::update_dirty(const UIRect& dirty) {
    CGRect rc = CGRectMake(dirty.x(), dirty.y(), dirty.width(), dirty.height());
    if (CGRectIsEmpty(m_dirty_rect)) {
        m_dirty_rect = rc;
    } else {
        m_dirty_rect = CGRectUnion(m_dirty_rect, rc);
    }
}

bool UIContext::update_size(const CGSize& size) {
    bool res = false;
    if (likely(m_attached)) {
        UISize wnd_size = size2cell(size);
        if (m_window_size != wnd_size) {
            m_window_size = wnd_size;
            res = true;
        }
    }
    return res;
}

void UIContext::draw(CGContextRef context) {
    if (likely(m_attached)) {
        nvc_lock_guard_t guard(m_locker);
        UIRect rc = rect2cell(CGContextGetClipBoundingBox(context));
        for (const auto& [key, grid] : m_grids) {
            grid->draw(*this, context, rc);
        }
    }
}

CGPoint UIContext::find_cursor(void) {
    CGPoint pt = CGPointZero;
    if (likely(m_attached)) {
        nvc_lock_guard_t guard(m_locker);
        for (const auto& [key, grid] : m_grids) {
            if (grid->has_cursor()) {
                pt = cell2point(grid->cursor());
                break;
            }
        }
    }
    return pt;
}

void UIContext::change_mode(const std::string& mode, int index) {
    nvc_lock_guard_t guard(m_locker);
    m_mode.change(mode, index);
    for (const auto& [key, grid] : m_grids) {
        if (grid->has_cursor()) {
            update_dirty(UIRect(grid->cursor(), UISize(1, 1)));
        }
    }
}

void UIContext::resize_grid(int grid_id, int32_t width, int32_t height) {
    nvc_lock_guard_t guard(m_locker);
    nvc::UIGrid *grid;
    const auto& p = m_grids.find(grid_id);
    if (p == m_grids.end()) {
        grid = new nvc::UIGrid(nvc::UISize(width, height));
        m_grids[grid_id] = grid;
    } else {
        grid = p->second;
        grid->resize(nvc::UISize(width, height));
    }
}

void UIContext::destroy_grid(int grid_id) {
    nvc_lock_guard_t guard(m_locker);
    const auto& p = m_grids.find(grid_id);
    if (likely(p != m_grids.end())) {
        const auto& grid = p->second;
        m_grids.erase(p);
        update_dirty(UIRect(0, 0, grid->size().width, grid->size().height));
        delete grid;
    }
}

void UIContext::clear_grid(int grid_id) {
    nvc_lock_guard_t guard(m_locker);
    const auto& p = m_grids.find(grid_id);
    if (likely(p != m_grids.end())) {
        const auto& grid = p->second;
        grid->clear();
        update_dirty(UIRect(0, 0, grid->size().width, grid->size().height));
    }
}

void UIContext::update_grid_cursor(int grid_id, const UIPoint& pt) {
    nvc_lock_guard_t guard(m_locker);
    const auto& p = m_grids.find(grid_id);
    if (likely(p != m_grids.end())) {
        const auto& grid = p->second;
        if (grid->has_cursor()) {
            update_dirty(UIRect(grid->cursor(), UISize(1, 1)));
        }
        grid->set_cursor(pt);
        update_dirty(UIRect(pt, UISize(1, 1)));
    }
}

void UIContext::scroll_grid(int grid_id, const UIRect& rc, int32_t rows) {
    if (!rc.empty()) {
        nvc_lock_guard_t guard(m_locker);
        const auto& p = m_grids.find(grid_id);
        if (likely(p != m_grids.end())) {
            const auto& dirty = p->second->scroll(rc, rows);
            update_dirty(dirty);
        }
    }
}

void UIContext::update_grid_line(int items) {
    int last_grid_id = -1;
    nvc::UIGrid *grid = nullptr;
    nvc_lock_guard_t guard(m_locker);
    while (items-- > 0) {
        int narg = nvc_rpc_read_array_size(rpc());
        if (likely(narg > 4)) {
            narg -= 4;
            int grid_id = nvc_rpc_read_int(rpc());
            int row = nvc_rpc_read_int(rpc());
            int col_start = nvc_rpc_read_int(rpc());
            int cells = nvc_rpc_read_array_size(rpc());
            if (last_grid_id != grid_id || grid == nullptr) {
                last_grid_id = grid_id;
                const auto& p = m_grids.find(grid_id);
                if (likely(p != m_grids.end())) {
                    grid = p->second;
                }
            }
            if (likely(grid != nullptr)) {
                int last_hl_id = 0;
                int offset = col_start;
                while (cells-- > 0) {
                    int cnum = nvc_rpc_read_array_size(rpc());
                    if (likely(cnum-- > 0)) {
                        int repeat = 1;
                        uint32_t len = 0;
                        const uint8_t *str = (const uint8_t *)nvc_rpc_read_str(rpc(), &len);
                        if (len == 0) {
                            grid->skip_cell(nvc::UIPoint(offset, row));
                        } else {
                            UnicodeChar ch = UICharacter::utf8_to_unicode(str, len);
                            if (cnum-- > 0) {
                                int hl_id = nvc_rpc_read_int(rpc());
                                if (hl_id != last_hl_id) {
                                    last_hl_id = hl_id;
                                }
                            }
                            if (cnum-- > 0) {
                                repeat = nvc_rpc_read_int(rpc());
                            }
                            grid->update(nvc::UIPoint(offset, row), repeat, ch, last_hl_id);
                        }
                        offset += repeat;
                    }
                    nvc_rpc_read_skip_items(rpc(), cnum);
                }
                update_dirty(UIRect(col_start, row, offset - col_start, 1));
            }
            nvc_rpc_read_skip_items(rpc(), cells);
        }
        nvc_rpc_read_skip_items(rpc(), narg);
    }

}

}
