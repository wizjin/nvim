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
#include "nvc_ui_color.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
namespace nvc {

class CTFontHolder {
private:
    CTFontRef   m_font;
public:
    inline CTFontHolder() : m_font(nullptr) {}
    inline CTFontHolder(CTFontRef font) : m_font(font != nullptr ? reinterpret_cast<CTFontRef>(CFRetain(font)) : nullptr) {}
    inline explicit CTFontHolder(const CTFontHolder& font) : CTFontHolder(font.m_font) {}

    inline ~CTFontHolder() {
        if (m_font != nullptr) {
            CFRelease(m_font);
            m_font = nullptr;
        }
    }

    inline explicit operator CTFontRef() const { return m_font; }
    
    inline bool operator==(CTFontRef rhs) const { return m_font == rhs; }
    inline bool operator!=(CTFontRef rhs) const { return m_font != rhs; }

    inline CTFontHolder& operator=(CTFontRef other) {
        if (m_font != other) {
            if (m_font != nullptr) CFRelease(m_font);
            m_font = reinterpret_cast<CTFontRef>(CFRetain(other));
        }
        return *this;
    }

    inline CTFontHolder& operator=(const CTFontHolder& other) {
        if (this != &other) {
            if (m_font != nullptr) CFRelease(m_font);
            m_font = reinterpret_cast<CTFontRef>(CFRetain(other.m_font));
        }
        return *this;
    }

};

typedef std::vector<CTFontHolder> CTFontList;

struct CTGlyphInfo {
    CTFontRef   font;
    CGGlyph     glyph;
    bool        is_wide;
    bool        is_skip;
    bool        is_emoji;
};

class UIFont {
private:
    typedef std::map<UnicodeChar, CTGlyphInfo> CTGlyphInfoMap;

    CGSize          m_glyph_size;
    CGFloat         m_font_size;
    CGFloat         m_font_offset;
    bool            m_emoji;
    CTFontList      m_fonts;
    CTFontList      m_font_wides;
    CTFontHolder    m_font_emoji;
    CTGlyphInfoMap  m_glyph_cache;

    void update(void);
    CTFontRef load_emoji(void);
    CTGlyphInfo *load_glyph(UnicodeChar ch);
public:
    explicit UIFont(CGFloat font_size);
    inline CGFloat font_offset(void) const { return m_font_offset; }
    inline const CGSize& glyph_size(void) const { return m_glyph_size; }
    void emoji(bool on);
    bool load(const std::string& value);
    bool load_wide(const std::string& value);
    void draw(CGContextRef context, UnicodeChar ch, ui_color_t color, const UIPoint& pt);

};

}
#endif

#ifdef __cplusplus
}
#endif

#endif /* __NVC_UI_FONT_H__ */
