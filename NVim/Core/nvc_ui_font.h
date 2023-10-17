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
    bool        m_underline;
public:
    inline CTFontHolder() : m_font(nullptr), m_underline(false) {}
    inline CTFontHolder(CTFontRef font) {
        m_underline = false;
        if (font == nullptr) {
            m_font = nullptr;
        } else {
            m_font = reinterpret_cast<CTFontRef>(CFRetain(font));
        }
    }
    inline explicit CTFontHolder(const CTFontHolder& holder) { *this = holder; }

    inline ~CTFontHolder() {
        if (m_font != nullptr) {
            CFRelease(m_font);
            m_font = nullptr;
        }
    }
    inline explicit operator CTFontRef() const { return m_font; }
    inline bool operator!() const { return m_font == nullptr; }
    inline bool underline(void) const { return m_underline; }
    inline void underline(bool enable) { m_underline = enable; }

    inline CTFontHolder& operator=(const CTFontHolder& other) {
        if (this != &other) {
            if (m_font != nullptr) CFRelease(m_font);
            m_font = (other.m_font != nullptr ? reinterpret_cast<CTFontRef>(CFRetain(other.m_font)) : nullptr);
            m_underline = other.m_underline;
        }
        return *this;
    }
    
    inline bool find_glyphs(const UniChar chs[], CGGlyph glyphs[], uint8_t count) const {
        return (likely(m_font != nullptr) && CTFontGetGlyphsForCharacters(m_font, chs, glyphs, count));
    }
    
    inline void draw(CGContextRef context, CGGlyph glyph, ui_color_t color, const UIPoint& pt) const {
        if (likely(m_font != nullptr)) {
            UIColor::set_fill_color(context, color);
            CGContextSetTextPosition(context, pt.x, pt.y);
            CTFontDrawGlyphs(m_font, &glyph, &CGPointZero, 1, context);
        }
    }

};

typedef std::vector<CTFontHolder> CTFontList;

struct CTGlyphInfo {
    const CTFontHolder* font;
    CGGlyph             glyph;
    bool                is_skip;
    bool                is_wide;
    bool                is_space;
    bool                is_emoji;
};

class UIFont {
private:
    typedef std::map<UnicodeChar, CTGlyphInfo> CTGlyphInfoMap;

    CGSize          m_glyph_size;
    CGFloat         m_font_size;
    CGFloat         m_font_offset;
    CGFloat         m_underline;
    CGFloat         m_underline_position;
    bool            m_emoji;
    CTFontList      m_fonts;
    CTFontList      m_font_wides;
    CTFontHolder    m_font_emoji;
    CTGlyphInfoMap  m_glyph_cache;

    void update(void);
    const CTFontHolder *load_emoji(void);
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
