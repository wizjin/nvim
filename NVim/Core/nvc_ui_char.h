//
//  nvc_ui_char.h
//  NVim
//
//  Created by wizjin on 2023/10/16.
//

#ifndef __NVC_UI_CHAR_H__
#define __NVC_UI_CHAR_H__

#include "nvc_util.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
namespace nvc {

typedef uint32_t UnicodeChar;

class UICharacter {
private:
    CFCharacterSetRef   m_spaceSet;
    CFCharacterSetRef   m_wideSet;
    CFCharacterSetRef   m_cjkSet;
    CFCharacterSetRef   m_emojiSet;

    explicit UICharacter();
public:
    ~UICharacter();
    
    inline bool is_space(UTF32Char ch) const { return CFCharacterSetIsLongCharacterMember(m_spaceSet, ch); }
    inline bool is_wide(UTF32Char ch) const { return CFCharacterSetIsLongCharacterMember(m_wideSet, ch); }
    inline bool is_cjk(UTF32Char ch) const { return CFCharacterSetIsLongCharacterMember(m_cjkSet, ch); }
    inline bool is_emoji(UTF32Char ch) const { return CFCharacterSetIsLongCharacterMember(m_emojiSet, ch); }
    inline bool is_colorful(UnicodeChar ch) const { return (ch & 0xFE0F0000) == 0xFE0F0000; } // VARIATION SELECTOR-16

    static inline UnicodeChar utf8_to_unicode(const uint8_t *str, uint8_t len) {
        UnicodeChar unicode = 0;
        switch (len) {
            case 0:
                break;
            case 1:
                unicode = str[0] & 0x7F;
                break;
            case 2:
                if (likely(str[1] == 0x80)) {
                    unicode = ((str[0] & 0x1F) << 6)
                            | (str[1] & 0x3F);
                }
                break;
            case 3:
                if (likely(((str[1]&0xC0) == 0x80) && ((str[2]&0xC0) == 0x80))) {
                    unicode = ((str[0] & 0x0F) << 12)
                            | ((str[1] & 0x3F) << 6)
                            | (str[2] & 0x3F);
                }
                break;
            case 4:
                if (likely(((str[1]&0xC0) == 0x80) && ((str[2]&0xC0) == 0x80) && ((str[3]&0xC0) == 0x80))) {
                    unicode = ((str[0] & 0x07) << 18)
                            | ((str[1] & 0x3F) << 12)
                            | ((str[2] & 0x3F) << 6)
                            | (str[3] & 0x3F);
                }
                break;
            case 6:
                if (likely(((str[1]&0xC0) == 0x80) && ((str[2]&0xC0) == 0x80) && ((str[4]&0xC0) == 0x80) && ((str[5]&0xC0) == 0x80))) {
                    unicode = ((str[3] & 0x0F) << 28)
                            | ((str[4] & 0x3F) << 22)
                            | ((str[5] & 0x3F) << 16)
                            | ((str[0] & 0x0F) << 12)
                            | ((str[1] & 0x3F) << 6)
                            | (str[2] & 0x3F);
                }
                break;
            default:
                NVLogW("nvc ui invalid utf8 %d", len);
                break;
        }
        return unicode;
    }

    static inline const UICharacter& instance(void) { return g_character_set; }
private:
    static UICharacter g_character_set;
};

}
#endif

#ifdef __cplusplus
}
#endif

#endif /* __NVC_UI_CHAR_H__ */
