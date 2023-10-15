//
//  nvc_ui_font.h
//  NVim
//
//  Created by wizjin on 2023/10/14.
//

#ifndef __NVC_UI_FONT_H__
#define __NVC_UI_FONT_H__

#include <string>
#include "nvc_ui_cell.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
namespace nvc {

class CTFontHolder {
private:
    CTFontRef   m_font;
public:
    inline CTFontHolder(CTFontRef font) : m_font(reinterpret_cast<CTFontRef>(CFRetain(font))) {
        assert(font != nullptr);
    }

    inline CTFontHolder(const CTFontHolder& font) : CTFontHolder(font.m_font) {}

    inline ~CTFontHolder() {
        CFRelease(m_font);
    }

    inline explicit operator CTFontRef() const {
        return m_font;
    }

    inline CTFontHolder& operator=(const CTFontHolder& other) {
        if (this != &other) {
            CFRelease(m_font);
            m_font = reinterpret_cast<CTFontRef>(CFRetain(other.m_font));
        }
        return *this;
    }

};

typedef std::vector<CTFontHolder> CTFontList;

class UIFont {
private:
    CGSize      m_glyph_size;
    CGFloat     m_font_size;
    CGFloat     m_font_offset;
    CTFontList  m_fonts;
    CTFontList  m_font_wides;

    void update(void);
public:
    explicit UIFont(CGFloat font_size);
    inline CGFloat font_offset(void) const { return m_font_offset; }
    inline const CGSize& glyph_size(void) const { return m_glyph_size; }
    bool load(const std::string& value);
    bool load_wide(const std::string& value);

};

}
#endif

#ifdef __cplusplus
}
#endif

#endif /* __NVC_UI_FONT_H__ */
