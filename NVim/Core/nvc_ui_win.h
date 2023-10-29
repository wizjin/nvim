//
//  nvc_ui_win.h
//  NVim
//
//  Created by wizjin on 2023/10/28.
//

#ifndef __NVC_UI_WIN_H__
#define __NVC_UI_WIN_H__

#include "nvc_ui_grid.h"
#include "nvc_rpc.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
namespace nvc {

class UIWin {
private:
    int                         m_grid_id;
    nvc_rpc_object_handler_t    m_win;
    UIRect                      m_frame;
    bool                        m_hidden;
public:
    inline explicit UIWin(int grid_id, nvc_rpc_object_handler_t win, const UIRect& frame) : m_grid_id(grid_id), m_win(win), m_frame(frame), m_hidden(false) {}

    inline int grid_id(void) const { return m_grid_id; }
    inline const UIRect& frame(void) const { return m_frame; }
    inline bool hidden(void) const { return m_hidden; }
    inline void hidden(bool hidden) { m_hidden = hidden; }
    inline void update(nvc_rpc_object_handler_t win, const UIRect& frame) {
        m_win = win;
        m_frame = frame;
        m_hidden = false;
    }

};

}
#endif

#ifdef __cplusplus
}
#endif

#endif /* __NVC_UI_WIN_H__ */
