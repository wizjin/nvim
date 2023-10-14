//
//  nvc_ui_font.h
//  NVim
//
//  Created by wizjin on 2023/10/14.
//

#ifndef __NVC_UI_FONT_H__
#define __NVC_UI_FONT_H__

#include <CoreText/CoreText.h>
#include <string>
#include <vector>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
namespace nvc {

class UIFont {
private:
    std::vector<CTFontRef>  m_fonts;
protected:
    void clear(void);
public:
    explicit UIFont();
    virtual ~UIFont();
    inline bool empty(void) const { return m_fonts.empty(); }
    UIFont& operator=(const UIFont& other);
};

}
#endif

#ifdef __cplusplus
}
#endif

#endif /* __NVC_UI_FONT_H__ */
