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
class UIFontInfo;

struct CTGlyphInfo {
    UIFontInfo*     font;
    CGGlyph         glyph;
    UniChar         chs[2];
    uint8_t         chs_n;
    bool            is_skip         :1;
    bool            is_wide         :1;
    bool            is_space        :1;
    bool            is_emoji        :1;
    bool            chk_bold        :1;
    bool            can_bold        :1;
    bool            chk_italic      :1;
    bool            can_italic      :1;
    bool            chk_bold_italic :1;
    bool            can_bold_italic :1;
    
};

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
    
    inline CTFontHolder copy(CTFontSymbolicTraits traits) const {
        return likely(m_font != nullptr) ? CTFontCreateCopyWithSymbolicTraits(m_font, 0, nullptr, traits, traits) : nullptr;
    }

    inline bool find_glyphs(const UniChar chs[], CGGlyph glyphs[], uint8_t count) const {
        return likely(m_font != nullptr) && CTFontGetGlyphsForCharacters(m_font, chs, glyphs, count) && glyphs[0] != 0;
    }
    
    inline bool has_char(const UniChar chs[2], uint8_t n) const {
        CGGlyph glyphs[2];
        return find_glyphs(chs, glyphs, n) && glyphs[0] != 0;
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
    inline CTFontRef find(UIFontTraits traits, CTGlyphInfo *info) {
        CTFontRef font = nullptr;
        if (likely(info != nullptr && info->chs_n > 0)) {
            switch(traits) {
                default: break;
                case ui_font_traits_bold:
                    if (!info->chk_bold) {
                        info->chk_bold = true;
                        if (!m_font_bold) m_font_bold = m_font.copy(kCTFontTraitBold);
                        if (m_font_bold.has_char(info->chs, info->chs_n)) {
                            info->can_bold = true;
                        }
                    }
                    if (info->can_bold) {
                        font = static_cast<CTFontRef>(m_font_bold);
                    }
                    break;
                case ui_font_traits_italic:
                    if (!info->chk_italic) {
                        info->chk_italic = true;
                        if (!m_font_italic) m_font_italic = m_font.copy(kCTFontTraitItalic);
                        if (m_font_italic.has_char(info->chs, info->chs_n)) {
                            info->can_italic = true;
                        }
                    }
                    if (info->can_italic) {
                        font = static_cast<CTFontRef>(m_font_italic);
                    }
                    break;
                case ui_font_traits_bold_italic:
                    if (!info->chk_bold_italic) {
                        info->chk_bold_italic = true;
                        if (!m_font_bold_italic) m_font_bold_italic = m_font.copy(kCTFontTraitBold|kCTFontTraitItalic);
                        if (m_font_bold_italic.has_char(info->chs, info->chs_n)) {
                            info->can_bold_italic = true;
                        }
                    }
                    if (info->can_bold_italic) {
                        font = static_cast<CTFontRef>(m_font_bold_italic);
                    }
                    break;
            }
        }
        if (font == nullptr) {
            font = static_cast<CTFontRef>(m_font);
        }
        return font;
    }
};

typedef std::vector<UIFontInfo> CTFontList;

class UIFont {
private:
    typedef std::map<UnicodeChar, CTGlyphInfo> CTGlyphInfoMap;

    CGSize          m_glyph_size;
    CGFloat         m_font_size;
    CGFloat         m_font_offset;
    CGFloat         m_scale_factor;
    CGFloat         m_underline_position;
    CGFloat         m_underline_thickness;
    bool            m_emoji;
    CTFontList      m_fonts;
    CTFontList      m_font_wides;
    UIFontInfo      m_font_emoji;
    CTGlyphInfoMap  m_glyph_cache;

    void update(void);
    UIFontInfo *load_emoji(void);
public:
    explicit UIFont(CGFloat font_size, CGFloat scale_factor);
    inline const CGSize& glyph_size(void) const { return m_glyph_size; }
    inline CGFloat font_offset(void) const { return m_font_offset; }
    inline CGFloat underline_position(void) const { return m_underline_position; }
    inline CGFloat underline_thickness(void) const { return m_underline_thickness; }
    inline CGFloat scale_factor(void) const { return m_scale_factor; }
    void emoji(bool on);
    bool load(const std::string& value);
    bool load_wide(const std::string& value);
    CTGlyphInfo *load_glyph(UnicodeChar ch);

};

}
#endif

#ifdef __cplusplus
}
#endif

#endif /* __NVC_UI_FONT_H__ */
