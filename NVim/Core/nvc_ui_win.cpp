//
//  nvc_ui_win.cpp
//  NVim
//
//  Created by wizjin on 2023/10/28.
//

#include "nvc_ui_win.h"

namespace nvc {

UIWin::UIWin(int grid_id, nvc_rpc_object_handler_t win, const UIRect& frame)
: m_grid_id(grid_id), m_win(win), m_frame(frame), m_dirty(), m_hidden(false) {
    
}

}
