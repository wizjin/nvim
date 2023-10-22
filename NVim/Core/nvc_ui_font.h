//
//  nvc_ui_font.h
//  NVim
//
//  Created by wizjin on 2023/10/14.
//

#ifndef __NVC_UI_FONT_H__
#define __NVC_UI_FONT_H__

#include "nvc_ui_hl.h"
#include "nvc_ui_cell.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
namespace nvc {

class UIRender;

class CTFontHolder {
private:
    CTFontRef   m_font;
public:
    inline CTFontHolder() : m_font(nullptr) {}
    inline CTFontHolder(CTFontRef font) {
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

    inline CTFontHolder& operator=(const CTFontHolder& other) {
        if (this != &other) {
            if (m_font != nullptr) CFRelease(m_font);
            m_font = (other.m_font != nullptr ? reinterpret_cast<CTFontRef>(CFRetain(other.m_font)) : nullptr);
        }
        return *this;
    }
    
    inline CTFontHolder copy(CTFontSymbolicTraits traits) {
        return likely(m_font != nullptr) ? CTFontCreateCopyWithSymbolicTraits(m_font, 0, nullptr, traits, traits) : nullptr;
    }
    
    inline bool find_glyphs(const UniChar chs[], CGGlyph glyphs[], uint8_t count) const {
        return likely(m_font != nullptr) && CTFontGetGlyphsForCharacters(m_font, chs, glyphs, count) && glyphs[0] != 0;
    }

};

class UIFontInfo {
private:
    CTFontHolder    m_font;
    CTFontHolder    m_font_bold;
    CTFontHolder    m_font_italic;
    CTFontHolder    m_font_bold_italic;
    bool            m_underline;
public:
    inline UIFontInfo() : m_underline(false) {}
    inline UIFontInfo(CTFontRef font) : m_font(font), m_underline(false) {}
    inline bool operator!() const { return !m_font; }
    inline bool underline(void) const { return m_underline; }
    inline void underline(bool enable) { m_underline = enable; }
    inline explicit operator CTFontRef() const { return static_cast<CTFontRef>(m_font); }
    inline bool find_glyphs(const UniChar chs[], CGGlyph glyphs[], uint8_t count) const { return m_font.find_glyphs(chs, glyphs, count); }
    inline CTFontRef find(UIFontTraits traits) {
        CTFontRef font = nullptr;
        switch(traits) {
            default:
            case ui_font_traits_none:
                font = static_cast<CTFontRef>(m_font);
                break;
            case ui_font_traits_bold:
                if (!m_font_bold) m_font_bold = m_font.copy(kCTFontTraitBold);
                font = static_cast<CTFontRef>(m_font_bold);
                break;
            case ui_font_traits_italic:
                if (!m_font_italic) m_font_italic = m_font.copy(kCTFontTraitItalic);
                font = static_cast<CTFontRef>(m_font_italic);
                break;
            case ui_font_traits_bold_italic:
                if (!m_font_bold_italic) m_font_bold_italic = m_font.copy(kCTFontTraitBold|kCTFontTraitItalic);
                font = static_cast<CTFontRef>(m_font_bold_italic);
                break;
        }
        return font;
    }
};

typedef std::vector<UIFontInfo> CTFontList;

struct CTGlyphInfo {
    UIFontInfo*     font;
    CGGlyph         glyph;
    bool            is_skip;
    bool            is_wide;
    bool            is_space;
    bool            is_emoji;
};

class UIFont {
private:
    typedef std::map<UnicodeChar, CTGlyphInfo> CTGlyphInfoMap;

    CGSize          m_glyph_size;
    CGFloat         m_font_size;
    CGFloat         m_font_offset;
    CGFloat         m_scale_factor;
    CGFloat         m_underline;
    CGFloat         m_underline_position;
    bool            m_emoji;
    CTFontList      m_fonts;
    CTFontList      m_font_wides;
    UIFontInfo      m_font_emoji;
    CTGlyphInfoMap  m_glyph_cache;

    void update(void);
    UIFontInfo *load_emoji(void);
    CTGlyphInfo *load_glyph(UnicodeChar ch);
public:
    explicit UIFont(CGFloat font_size, CGFloat scale_factor);
    inline CGFloat font_offset(void) const { return m_font_offset; }
    inline const CGSize& glyph_size(void) const { return m_glyph_size; }
    inline CGFloat underline_position(void) const { return m_underline_position; }
    inline CGFloat scale_factor(void) const { return m_scale_factor; }
    void emoji(bool on);
    bool load(const std::string& value);
    bool load_wide(const std::string& value);
    void draw(UIRender& render, UnicodeChar ch, UIFontTraits traits, const UIPoint& pt);

};

}
#endif

#ifdef __cplusplus
}
#endif

#endif /* __NVC_UI_FONT_H__ */
