//
//  nvc_ui_context.h
//  NVim
//
//  Created by wizjin on 2023/10/15.
//

#ifndef __NVC_UI_CONTEXT_H__
#define __NVC_UI_CONTEXT_H__

#include <mutex>
#include "nvc_rpc.h"
#include "nvc_ui.h"
#include "mvc_ui_mode.h"
#include "nvc_ui_grid.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
namespace nvc {

class UIContext {
public:
    using UIGridMap         = std::map<int, UIGrid *, std::less<int>>;
    using RPCCallback       = std::function<int(UIContext *, int, int)>;
    using RPCCallbackMap    = std::unordered_map<int64_t, RPCCallback>;
private:
    nvc_rpc_context_t   m_rpc;
    nvc_ui_callback_t   m_cb;
    void                *m_userdata;
    bool                m_attached;
    bool                m_mode_enabled;
    bool                m_show_cursor;
    std::mutex          m_locker;
    UIHLAttrGroups      m_hl_attrs;
    UIFont              m_font;
    UIMode              m_mode;
    UISize              m_window_size;
    CGRect              m_dirty_rect;
    UIGridMap           m_grids;
    RPCCallbackMap      m_rpc_callbacks;
public:
    explicit UIContext(const nvc_ui_callback_t& cb, const nvc_ui_config_t& config, void *userdata);
    inline nvc_rpc_context_t* rpc() { return &m_rpc; }
    inline const nvc_ui_callback_t& cb() const { return m_cb; }
    inline void *userdata(void) const { return m_userdata; }
    inline bool attached(void) const { return m_attached; }
    inline bool mode_enabled(void) const { return m_mode_enabled; }
    inline bool show_cursor(void) const { return m_show_cursor; }
    inline void show_cursor(bool value) { m_show_cursor = value; }
    inline UIHLAttrGroups& hl_attrs(void) { return m_hl_attrs; }
    inline UIFont& font(void) { return m_font; }
    inline UIMode& mode(void) { return m_mode; }
    inline const CGSize& cell_size(void) const { return m_font.glyph_size(); }
    inline const UISize& window_size(void) const { return m_window_size; }
    inline const CGRect& dirty(void) const { return m_dirty_rect; }
    inline void clear_dirty(void) { m_dirty_rect = CGRectZero; }
    inline UIGridMap& grids(void) { return m_grids; }

    inline UISize size2cell(const CGSize& size) const {
        return UISize(floor(size.width/cell_size().width), floor(size.height/cell_size().height));
    }
    inline CGPoint cell2point(const nvc::UIPoint& pt) const {
        return CGPointMake(cell_size().width * pt.x, cell_size().height * pt.y);
    }
    inline CGSize cell2size(const nvc::UISize& size) const {
        return CGSizeMake(cell_size().width * size.width, cell_size().height * size.height);
    }
    inline UIRect rect2cell(const CGRect& rect) const {
        UIRect rc;
        rc.origin.x = MAX(floor(rect.origin.x/cell_size().width), 0);
        rc.origin.y = MAX(floor(rect.origin.y/cell_size().height), 0);
        rc.size.width = MIN(ceil(rect.size.width/cell_size().width), window_size().width - rc.x());
        rc.size.height = MIN(ceil(rect.size.height/cell_size().height), window_size().height - rc.y());
        return rc;
    }
    
    void close(void);
    bool attach(const CGSize& size);
    bool detach(void);
    void update_dirty(const UIRect& dirty);
    bool update_size(const CGSize& size);
    void draw(CGContextRef context);
    CGPoint find_cursor(void);
    void change_mode(const std::string& mode, int index);
    void resize_grid(int grid_id, int32_t width, int32_t height);
    void destroy_grid(int grid_id);
    void clear_grid(int grid_id);
    void update_grid_cursor(int grid_id, const UIPoint& pt);
    void scroll_grid(int grid_id, const UIRect& rc, int32_t rows);
    void update_grid_line(int items);

    void set_rpc_callback(int64_t msgid, const RPCCallback& cb);
    bool find_rpc_callback(int64_t msgid, RPCCallback& cb);
};

}
#endif

#ifdef __cplusplus
}
#endif

#endif /* __NVC_UI_CONTEXT_H__ */
