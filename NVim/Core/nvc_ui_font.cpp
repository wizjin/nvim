//
//  nvc_ui_font.cpp
//  NVim
//
//  Created by wizjin on 2023/10/14.
//

#include "nvc_ui_font.h"
#include "nvc_util.h"

#define kNvcUiFontUserMax               32
#define kNvcUiFontGlyphCacheSize        2048
#define kNvcUiFontAppleColorEmoji       "Apple Color Emoji"

namespace nvc {
#pragma mark - Helper
static inline bool load_fonts(const std::string& value, CGFloat default_font_size, CTFontList& fonts) {
    for (const auto& item : token_spliter(value, ',')) {
        std::string family;
        CGFloat font_size = default_font_size;
        for (const auto& p : token_spliter(item, ':')) {
            if (family.empty()) {
                if (p.empty() || p.front() == '*') break;
                family = p;
            } else if (!p.empty()) {
                size_t idx = 0;
                if (p[idx] == 'h') idx++;
                if (idx < p.size() && isdigit(p[idx])) {
                    CGFloat size = std::stod(std::string(p.substr(idx)));
                    if (size > 0) {
                        font_size = size;
                    }
                }
            }
        }
        if (!family.empty()) {
            CFStringRef family_name = CFStringCreateWithBytes(nullptr, (const UInt8 *)family.c_str(), family.size(), kCFStringEncodingUTF8, false);
            if (likely(family_name)) {
                CTFontRef font = CTFontCreateWithName(family_name, font_size, nullptr);
                if (likely(font != nullptr)) {
                    fonts.push_back(font);
                    CFRelease(font);
                }
                CFRelease(family_name);
                if (unlikely(fonts.size() >= kNvcUiFontUserMax)) break;
            }
        }
    }
    return !fonts.empty();
}

static inline void find_glyph(const CTFontList& fonts, UniChar *c, CFIndex n, CTGlyphInfo& info) {
    CGGlyph glyphs[2] = { 0, 0 };
    for (const auto& p : fonts) {
        info.font = static_cast<CTFontRef>(p);
        if (CTFontGetGlyphsForCharacters(info.font, c, glyphs, n) && glyphs[0] != 0) {
            info.glyph = glyphs[0];
            break;
        }
    }
}

#pragma mark - UIFont
UIFont::UIFont(CGFloat font_size) : m_glyph_size(CGSizeZero), m_font_offset(0), m_font_size(font_size), m_emoji(true) {
    CTFontRef font = CTFontCreateUIFontForLanguage(kCTFontUIFontUserFixedPitch, m_font_size, nullptr);
    if (likely(font != nullptr)) {
        m_fonts.push_back(font);
        CFRelease(font);
    }
    update();
}

void UIFont::update(void) {
    if (!m_fonts.empty()) {
        CTFontRef font = static_cast<CTFontRef>(m_fonts.front());
        
        CGFloat ascent = CTFontGetAscent(font);
        CGFloat descent = CTFontGetDescent(font);
        CGFloat leading = CTFontGetLeading(font);
        CGFloat height = ceil(ascent + descent + leading);
        CGGlyph glyph = (CGGlyph) 0;
        UniChar capitalM = '\u004D';
        CGSize advancement = CGSizeZero;
        CTFontGetGlyphsForCharacters(font, &capitalM, &glyph, 1);
        CTFontGetAdvancesForGlyphs(font, kCTFontOrientationHorizontal, &glyph, &advancement, 1);
        CGFloat width = ceil(advancement.width);

        m_glyph_size = CGSizeMake(width, height);
        m_font_size = CTFontGetSize(font);
        m_font_offset = descent;
    }
    m_glyph_cache.clear();
}

void UIFont::emoji(bool on) {
    if (m_emoji != on) {
        m_emoji = on;
        m_glyph_cache.clear();
    }
}

bool UIFont::load(const std::string& value) {
    bool res = false;
    CTFontList  fonts;
    if (load_fonts(value, m_font_size, fonts)) {
        m_fonts = fonts;
        update();
        res = true;
    }
    return res;
}

bool UIFont::load_wide(const std::string& value) {
    bool res = false;
    CTFontList  fonts;
    if (load_fonts(value, m_font_size, fonts)) {
        m_font_wides = fonts;
        m_glyph_cache.clear();
        res = true;
    }
    return res;
}

CTFontRef UIFont::load_emoji(void) {
    if (m_font_emoji == nullptr && likely(!m_fonts.empty())) {
        CFStringRef family = CFStringCreateWithCString(nullptr, kNvcUiFontAppleColorEmoji, kCFStringEncodingASCII);
        if (family != nullptr) {
            bool found = false;
            for (const auto& p : m_fonts) {
                CFStringRef name = CTFontCopyFamilyName(static_cast<CTFontRef>(p));
                if (name != nullptr) {
                    CFRange range = CFStringFind(name, family, kCFCompareCaseInsensitive);
                    CFRelease(name);
                    if (range.location != kCFNotFound) {
                        found = true;
                        break;
                    }
                }
            }
            if (!found) {
                CTFontRef font = CTFontCreateCopyWithFamily(static_cast<CTFontRef>(m_fonts.front()), 0, nullptr, family);
                if (font != nullptr) {
                    m_font_emoji = font;
                    CFRelease(font);
                }
            }
            CFRelease(family);
        }
    }
    return static_cast<CTFontRef>(m_font_emoji);
}

CTGlyphInfo *UIFont::load_glyph(UnicodeChar ch) {
    CTGlyphInfo *result = nullptr;
    const auto& p = m_glyph_cache.find(ch);
    if (p != m_glyph_cache.end()) {
        result = &p->second;
    } else {
        CTGlyphInfo info;
        bzero(&info, sizeof(info));
        const auto& chars = UICharacter::instance();
        info.is_wide = chars.is_wide(ch);
        if (chars.is_skip(ch)) {
            info.is_skip = true;
        } else {
            UniChar c[2];
            int n = 1;
            if (ch < 0x10000) {
                c[0] = (UniChar) ch;
            } else if ((uint16_t)ch == 0xFE0F) { // VARIATION SELECTOR-16
                c[0] = (UniChar) (ch >> 16);
                info.is_emoji = m_emoji && chars.is_emoji(c[0]);
            } else {
                UnicodeChar cp = ch - 0x10000;
                c[0] = 0xD800 + ((cp >> 10)&0x03FF);
                c[1] = 0xDC00 + (cp & 0x03FF);
                n = 2;
            }
            CGGlyph glyphs[2] = { 0, 0 };
            if (!info.is_emoji) {
                if (info.is_wide) find_glyph(m_font_wides, c, n, info);
                if (info.glyph == 0) find_glyph(m_fonts, c, n, info);
                if (info.glyph == 0) info.is_emoji = chars.is_emoji(ch);
            }
            if (info.glyph == 0 && info.is_emoji) {
                info.font = load_emoji();
                if (likely(info.font != nullptr && CTFontGetGlyphsForCharacters(info.font, c, glyphs, n) && glyphs[0] != 0)) {
                    info.glyph = glyphs[0];
                }
            }
            info.is_skip = (info.glyph == 0);
        }
        if (m_glyph_cache.size() >= kNvcUiFontGlyphCacheSize) {
            m_glyph_cache.clear();
        }
        m_glyph_cache[ch] = info;
        result = &m_glyph_cache[ch];
    }
    return result;
}

void UIFont::draw(CGContextRef context, UnicodeChar ch, ui_color_t color, const UIPoint& pt) {
    CTGlyphInfo *info = load_glyph(ch);
    if (likely(info != nullptr) && !info->is_skip) {
        UIColor::set_fill_color(context, !info->is_emoji ? color : NVC_UI_COLOR_WHITE);
        CGContextSetTextPosition(context, pt.x, pt.y);
        CTFontDrawGlyphs(info->font, &info->glyph, &CGPointZero, 1, context);
    }
}

}
