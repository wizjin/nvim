//
//  nvc_ui_cell.h
//  NVim
//
//  Created by wizjin on 2023/10/15.
//

#ifndef __NVC_UI_CELL_H__
#define __NVC_UI_CELL_H__

#include "nvc_ui_char.h"

#ifdef __cplusplus
extern "C" {
#endif


#ifdef __cplusplus
namespace nvc {

struct UICell {
    UnicodeChar ch;
    int32_t     hl_id;
    bool        is_skip;
    bool        is_wide;
};


}
#endif

#ifdef __cplusplus
}
#endif

#endif /* __NVC_UI_CELL_H__ */
