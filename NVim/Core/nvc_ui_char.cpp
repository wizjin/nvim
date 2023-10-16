//
//  nvc_ui_char.cpp
//  NVim
//
//  Created by wizjin on 2023/10/16.
//

#include "nvc_ui_char.h"

namespace nvc {

UICharacter UICharacter::g_character_set;

UICharacter::~UICharacter() {
    if (m_wideSet != nullptr) {
        CFRelease(m_wideSet);
        m_wideSet = nullptr;
    }
    if (m_skipSet != nullptr) {
        CFRelease(m_skipSet);
        m_skipSet = nullptr;
    }
    if (m_cjkSet != nullptr) {
        CFRelease(m_cjkSet);
        m_cjkSet = nullptr;
    }
    if (m_emojiSet != nullptr) {
        CFRelease(m_emojiSet);
        m_emojiSet = nullptr;
    }
}

static inline CFCharacterSetRef load_character_set(CFRange ranges[], int len) {
    CFMutableCharacterSetRef characterSet = CFCharacterSetCreateMutable(kCFAllocatorDefault);
    if (likely(characterSet != nullptr)) {
        for (int i = 0; i < len; i++) {
            CFCharacterSetAddCharactersInRange(characterSet, ranges[i]);
        }
    }
    return characterSet;
}

#define NVC_UI_CHAR_RANGE(_c1, _c2)     { .location = _c1, .length = (_c2 - _c1 + 1) }
UICharacter::UICharacter() {
    // Note: https://www.unicode.org/Public/15.1.0/ucd/extracted/DerivedEastAsianWidth.txt
    CFRange wide_ranges[] = {
        NVC_UI_CHAR_RANGE(0x3400, 0x4DBF),      // CJK_Unified_Ideographs_Extension_A
        NVC_UI_CHAR_RANGE(0x4E00, 0x9FFF),      // CJK_Unified_Ideographs
        NVC_UI_CHAR_RANGE(0xF900, 0xFAFF),      // CJK_Compatibility_Ideographs
        NVC_UI_CHAR_RANGE(0x20000, 0x2A6DF),    // CJK_Unified_Ideographs_Extension_B
        NVC_UI_CHAR_RANGE(0x2A700, 0x2B73F),    // CJK_Unified_Ideographs_Extension_C
        NVC_UI_CHAR_RANGE(0x2B740, 0x2B81F),    // CJK_Unified_Ideographs_Extension_D
        NVC_UI_CHAR_RANGE(0x2B820, 0x2CEAF),    // CJK_Unified_Ideographs_Extension_E
        NVC_UI_CHAR_RANGE(0x2CEB0, 0x2EBEF),    // CJK_Unified_Ideographs_Extension_F
        NVC_UI_CHAR_RANGE(0x2EBF0, 0x2EE5F),    // CJK_Unified_Ideographs_Extension_I
        NVC_UI_CHAR_RANGE(0x2F800, 0x2FA1F),    // CJK_Compatibility_Ideographs_Supplement
        NVC_UI_CHAR_RANGE(0x30000, 0x3134F),    // CJK_Unified_Ideographs_Extension_G
        NVC_UI_CHAR_RANGE(0x31350, 0x323AF),    // CJK_Unified_Ideographs_Extension_H
    };
    
    // Note: https://www.unicode.org/reports/tr38/#N10106
    CFRange cjk_ranges[] = {
        // CJK Radicals Supplement
        NVC_UI_CHAR_RANGE(0x2E80, 0x2E99),
        NVC_UI_CHAR_RANGE(0x2E9B, 0x2EF3),
        // Kangxi Radicals
        NVC_UI_CHAR_RANGE(0x2F00, 0x2FD5),
        // CJK Symbols and Punctuation
        NVC_UI_CHAR_RANGE(0x3000, 0x303F),
        // Kanbun
        NVC_UI_CHAR_RANGE(0x3190, 0x319F),
        // CJK Strokes
        NVC_UI_CHAR_RANGE(0x31C0, 0x31E3),
        // Enclosed CJK Letters and Months
        NVC_UI_CHAR_RANGE(0x3220, 0x324F),
        NVC_UI_CHAR_RANGE(0x3280, 0x32B0),
        NVC_UI_CHAR_RANGE(0x32C0, 0x32CB),
        NVC_UI_CHAR_RANGE(0x32D0, 0x32FF),
        // CJK Compatibility
        NVC_UI_CHAR_RANGE(0x3358, 0x3370),
        NVC_UI_CHAR_RANGE(0x337B, 0x337F),
        NVC_UI_CHAR_RANGE(0x33E0, 0x33FE),
        // Enclosed Ideographic Supplement
        NVC_UI_CHAR_RANGE(0x1F210, 0x1F23B),
        NVC_UI_CHAR_RANGE(0x1F240, 0x1F248),
        NVC_UI_CHAR_RANGE(0x1F250, 0x1F251),
        // CJK Unified Ideographs Extension A
        NVC_UI_CHAR_RANGE(0x3400, 0x4DBF),
        // CJK Unified Ideographs
        NVC_UI_CHAR_RANGE(0x4E00, 0x9FFF),
        // CJK Compatibility Ideographs
        NVC_UI_CHAR_RANGE(0xF900, 0xFA6D),
        NVC_UI_CHAR_RANGE(0xFA70, 0xFAD9),
        // CJK Unified Ideographs Extension B
        NVC_UI_CHAR_RANGE(0x20000, 0x2A6DF),
        // CJK Unified Ideographs Extension C
        NVC_UI_CHAR_RANGE(0x2A700, 0x2B739),
        // CJK Unified Ideographs Extension D
        NVC_UI_CHAR_RANGE(0x2B740, 0x2B81D),
        // CJK Unified Ideographs Extension E
        NVC_UI_CHAR_RANGE(0x2B820, 0x2CEA1),
        // CJK Unified Ideographs Extension F
        NVC_UI_CHAR_RANGE(0x2CEB0, 0x2EBE0),
        // CJK Unified Ideographs Extension I
        NVC_UI_CHAR_RANGE(0x2EBF0, 0x2EE5D),
        // CJK Compatibility Ideographs Supplement
        NVC_UI_CHAR_RANGE(0x2F800, 0x2FA1D),
        // CJK Unified Ideographs Extension G
        NVC_UI_CHAR_RANGE(0x30000, 0x3134A),
        // CJK Unified Ideographs Extension H
        NVC_UI_CHAR_RANGE(0x31350, 0x323AF),
    };

    // Note: https://www.unicode.org/Public/15.1.0/ucd/emoji/emoji-data.txt
    CFRange emoji_ranges[] = {
        NVC_UI_CHAR_RANGE(0x2139, 0x2139),      // (ℹ️)        information
        NVC_UI_CHAR_RANGE(0x2194, 0x2199),      // (↔️..↙️)    left-right arrow..down-left arrow
        NVC_UI_CHAR_RANGE(0x21A9, 0x21AA),      // (↩️..↪️)    right arrow curving left..left arrow curving right
        NVC_UI_CHAR_RANGE(0x231A, 0x231B),      // (⌚..⌛)    watch..hourglass done
        NVC_UI_CHAR_RANGE(0x2328, 0x2328),      // (⌨️)        keyboard
        NVC_UI_CHAR_RANGE(0x23CF, 0x23CF),      // (⏏️)        eject button
        NVC_UI_CHAR_RANGE(0x23E9, 0x23F3),      // (⏩..⏳)    fast-forward button..hourglass not done
        NVC_UI_CHAR_RANGE(0x23F8, 0x23FA),      // (⏸️..⏺️)    pause button..record button
        NVC_UI_CHAR_RANGE(0x24C2, 0x24C2),      // (Ⓜ️)        circled M
        NVC_UI_CHAR_RANGE(0x25AA, 0x25AB),      // (▪️..▫️)    black small square..white small square
        NVC_UI_CHAR_RANGE(0x25B6, 0x25B6),      // (▶️)        play button
        NVC_UI_CHAR_RANGE(0x25C0, 0x25C0),      // (◀️)        reverse button
        NVC_UI_CHAR_RANGE(0x25FB, 0x25FE),      // (◻️..◾)    white medium square..black medium-small square
        NVC_UI_CHAR_RANGE(0x2600, 0x2604),      // (☀️..☄️)    sun..comet
        NVC_UI_CHAR_RANGE(0x260E, 0x260E),      // (☎️)        telephone
        NVC_UI_CHAR_RANGE(0x2611, 0x2611),      // (☑️)        check box with check
        NVC_UI_CHAR_RANGE(0x2614, 0x2615),      // (☔..☕)    umbrella with rain drops..hot beverage
        NVC_UI_CHAR_RANGE(0x2618, 0x2618),      // (☘️)        shamrock
        NVC_UI_CHAR_RANGE(0x261D, 0x261D),      // (☝️)        index pointing up
        NVC_UI_CHAR_RANGE(0x2620, 0x2620),      // (☠️)        skull and crossbones
        NVC_UI_CHAR_RANGE(0x2622, 0x2623),      // (☢️..☣️)    radioactive..biohazard
        NVC_UI_CHAR_RANGE(0x2626, 0x2626),      // (☦️)        orthodox cross
        NVC_UI_CHAR_RANGE(0x262A, 0x262A),      // (☪️)        star and crescent
        NVC_UI_CHAR_RANGE(0x262E, 0x262F),      // (☮️..☯️)    peace symbol..yin yang
        NVC_UI_CHAR_RANGE(0x2638, 0x263A),      // (☸️..☺️)    wheel of dharma..smiling face
        NVC_UI_CHAR_RANGE(0x2640, 0x2640),      // (♀️)        female sign
        NVC_UI_CHAR_RANGE(0x2642, 0x2642),      // (♂️)        male sign
        NVC_UI_CHAR_RANGE(0x2648, 0x2653),      // (♈..♓)    Aries..Pisces
        NVC_UI_CHAR_RANGE(0x265F, 0x2660),      // (♟️..♠️)    chess pawn..spade suit
        NVC_UI_CHAR_RANGE(0x2663, 0x2663),      // (♣️)        club suit
        NVC_UI_CHAR_RANGE(0x2665, 0x2666),      // (♥️..♦️)    heart suit..diamond suit
        NVC_UI_CHAR_RANGE(0x2668, 0x2668),      // (♨️)        hot springs
        NVC_UI_CHAR_RANGE(0x267B, 0x267B),      // (♻️)        recycling symbol
        NVC_UI_CHAR_RANGE(0x267E, 0x267F),      // (♾️..♿)    infinity..wheelchair symbol
        NVC_UI_CHAR_RANGE(0x2692, 0x2697),      // (⚒️..⚗️)    hammer and pick..balance scale..alembic
        NVC_UI_CHAR_RANGE(0x2699, 0x2699),      // (⚙️)        gear
        NVC_UI_CHAR_RANGE(0x269B, 0x269C),      // (⚛️..⚜️)    atom symbol..fleur-de-lis
        NVC_UI_CHAR_RANGE(0x26A0, 0x26A1),      // (⚠️..⚡)    warning..high voltage
        NVC_UI_CHAR_RANGE(0x26A7, 0x26A7),      // (⚧️)        transgender symbol
        NVC_UI_CHAR_RANGE(0x26AA, 0x26AB),      // (⚪..⚫)    white circle..black circle
        NVC_UI_CHAR_RANGE(0x26B0, 0x26B1),      // (⚰️..⚱️)    coffin..funeral urn
        NVC_UI_CHAR_RANGE(0x26BD, 0x26BE),      // (⚽..⚾)    soccer ball..baseball
        NVC_UI_CHAR_RANGE(0x26C4, 0x26C5),      // (⛄..⛅)    snowman without snow..sun behind cloud
        NVC_UI_CHAR_RANGE(0x26C8, 0x26C8),      // (⛈️)        cloud with lightning and rain
        NVC_UI_CHAR_RANGE(0x26CE, 0x26CF),      // (⛎..⛏️)    Ophiuchus..pick
        NVC_UI_CHAR_RANGE(0x26D1, 0x26D1),      // (⛑️)        rescue worker’s helmet
        NVC_UI_CHAR_RANGE(0x26D3, 0x26D4),      // (⛓️..⛔)    chains..no entry
        NVC_UI_CHAR_RANGE(0x26E9, 0x26EA),      // (⛩️..⛪)    shinto shrine..church
        NVC_UI_CHAR_RANGE(0x26F0, 0x26F5),      // (⛰️..⛵)    mountain..sailboat
        NVC_UI_CHAR_RANGE(0x26F7, 0x26FA),      // (⛷️..⛺)    skier..tent
        NVC_UI_CHAR_RANGE(0x26FD, 0x26FD),      // (⛽)        fuel pump
        NVC_UI_CHAR_RANGE(0x2702, 0x2702),      // (✂️)        scissors
        NVC_UI_CHAR_RANGE(0x2705, 0x2705),      // (✅)        check mark button
        NVC_UI_CHAR_RANGE(0x2708, 0x270F),      // (✈️..✏️)    airplane..pencil
        NVC_UI_CHAR_RANGE(0x2712, 0x2712),      // (✒️)        black nib
        NVC_UI_CHAR_RANGE(0x2714, 0x2714),      // (✔️)        check mark
        NVC_UI_CHAR_RANGE(0x2716, 0x2716),      // (✖️)        multiply
        NVC_UI_CHAR_RANGE(0x271D, 0x271D),      // (✝️)        latin cross
        NVC_UI_CHAR_RANGE(0x2721, 0x2721),      // (✡️)        star of David
        NVC_UI_CHAR_RANGE(0x2728, 0x2728),      // (✨)        sparkles
        NVC_UI_CHAR_RANGE(0x2733, 0x2734),      // (✳️..✴️)    eight-spoked asterisk..eight-pointed star
        NVC_UI_CHAR_RANGE(0x2744, 0x2744),      // (❄️)        snowflakes
        NVC_UI_CHAR_RANGE(0x2747, 0x2747),      // (❇️)        sparkle
        NVC_UI_CHAR_RANGE(0x274C, 0x274C),      // (❌)        cross mark
        NVC_UI_CHAR_RANGE(0x274E, 0x274E),      // (❎)        cross mark button
        NVC_UI_CHAR_RANGE(0x2753, 0x2755),      // (❓..❕)    red question mark..exclamation mark
        NVC_UI_CHAR_RANGE(0x2757, 0x2757),      // (❗)        red exclamation mark
        NVC_UI_CHAR_RANGE(0x2763, 0x2764),      // (❣️..❤️)    heart exclamation..red heart
        NVC_UI_CHAR_RANGE(0x2795, 0x2797),      // (➕..➗)    plus
        NVC_UI_CHAR_RANGE(0x27A1, 0x27A1),      // (➡️)        right arrow
        NVC_UI_CHAR_RANGE(0x27B0, 0x27B0),      // (➰)        curly loop
        NVC_UI_CHAR_RANGE(0x27BF, 0x27BF),      // (➿)        double curly loop
        NVC_UI_CHAR_RANGE(0x2934, 0x2935),      // (⤴️..⤵️)    right arrow curving up..arrow curving down
        NVC_UI_CHAR_RANGE(0x2B05, 0x2B07),      // (⬅️..⬇️)    left arrow..down arrow
        NVC_UI_CHAR_RANGE(0x2B1B, 0x2B1C),      // (⬛..⬜)    black large square..large square
        NVC_UI_CHAR_RANGE(0x2B50, 0x2B50),      // (⭐)        star
        NVC_UI_CHAR_RANGE(0x2B55, 0x2B55),      // (⭕)        hollow red circle
        NVC_UI_CHAR_RANGE(0x3030, 0x3030),      // (〰️)        wavy dash
        NVC_UI_CHAR_RANGE(0x303D, 0x303D),      // (〽️)        part alternation mark
        NVC_UI_CHAR_RANGE(0x3297, 0x3297),      // (㊗️)        Japanese “congratulations” button
        NVC_UI_CHAR_RANGE(0x3299, 0x3299),      // (㊙️)        Japanese “secret” button
        NVC_UI_CHAR_RANGE(0x1F004, 0x1F004),    // (🀄)        mahjong red dragon
        NVC_UI_CHAR_RANGE(0x1F0CF, 0x1F0CF),    // (🃏)        joker
        NVC_UI_CHAR_RANGE(0x1F170, 0x1F171),    // (🅰️..🅱️)    A button (blood type)..button (blood type)
        NVC_UI_CHAR_RANGE(0x1F17E, 0x1F17F),    // (🅾️..🅿️)    O button (blood type)..button
        NVC_UI_CHAR_RANGE(0x1F18E, 0x1F18E),    // (🆎)        AB button (blood type)
        NVC_UI_CHAR_RANGE(0x1F191, 0x1F19A),    // (🆑..🆚)    CL button button
        NVC_UI_CHAR_RANGE(0x1F201, 0x1F202),    // (🈁..🈂️)    Japanese “here” button..“service charge” button
        NVC_UI_CHAR_RANGE(0x1F21A, 0x1F21A),    // (🈚)        Japanese “free of charge” button
        NVC_UI_CHAR_RANGE(0x1F22F, 0x1F22F),    // (🈯)        Japanese “reserved” button
        NVC_UI_CHAR_RANGE(0x1F232, 0x1F23A),    // (🈲..🈺)    Japanese “prohibited” button..“open for business” button
        NVC_UI_CHAR_RANGE(0x1F250, 0x1F251),    // (🉐..🉑)    Japanese “bargain” button..“acceptable” button
        NVC_UI_CHAR_RANGE(0x1F300, 0x1F321),    // (🌀..🌡️)    cyclone..thermometer
        NVC_UI_CHAR_RANGE(0x1F324, 0x1F393),    // (🌤️..🎓)    sun behind small cloud..graduation cap
        NVC_UI_CHAR_RANGE(0x1F396, 0x1F397),    // (🎖️..🎗️)    military medal..reminder ribbon
        NVC_UI_CHAR_RANGE(0x1F399, 0x1F39B),    // (🎙️..🎛️)    studio microphone..control knobs
        NVC_UI_CHAR_RANGE(0x1F39E, 0x1F3F0),    // (🎞️..🏰)    film frames..castle
        NVC_UI_CHAR_RANGE(0x1F3F3, 0x1F3F5),    // (🏳️..🏵️)    white flag..rosette
        NVC_UI_CHAR_RANGE(0x1F3F7, 0x1F4FD),    // (🏷️..📽️)    label..film projector
        NVC_UI_CHAR_RANGE(0x1F4FF, 0x1F53D),    // (📿..🔽)    prayer beads..downwards button
        NVC_UI_CHAR_RANGE(0x1F549, 0x1F54E),    // (🕉️..🕎)    om..menorah
        NVC_UI_CHAR_RANGE(0x1F550, 0x1F567),    // (🕐..🕧)    one o’clock..twelve-thirty
        NVC_UI_CHAR_RANGE(0x1F56F, 0x1F570),    // (🕯️..🕰️)    candle..mantelpiece clock
        NVC_UI_CHAR_RANGE(0x1F573, 0x1F579),    // (🕳️..🕹️)    hole..joystick
        NVC_UI_CHAR_RANGE(0x1F57A, 0x1F57A),    // (🕺)        man dancing
        NVC_UI_CHAR_RANGE(0x1F587, 0x1F587),    // (🖇️)        linked paperclips
        NVC_UI_CHAR_RANGE(0x1F58A, 0x1F58D),    // (🖊️..🖍️)    pen..crayon
        NVC_UI_CHAR_RANGE(0x1F590, 0x1F590),    // (🖐️)        hand with fingers splayed
        NVC_UI_CHAR_RANGE(0x1F595, 0x1F596),    // (🖕..🖖)    middle finger..vulcan salute
        NVC_UI_CHAR_RANGE(0x1F5A4, 0x1F5A5),    // (🖤..🖥️)    black heart..desktop computer
        NVC_UI_CHAR_RANGE(0x1F5A8, 0x1F5A8),    // (🖨️)        printer
        NVC_UI_CHAR_RANGE(0x1F5B1, 0x1F5B2),    // (🖱️..🖲️)    computer mouse..trackball
        NVC_UI_CHAR_RANGE(0x1F5BC, 0x1F5BC),    // (🖼️)        framed picture
        NVC_UI_CHAR_RANGE(0x1F5C2, 0x1F5C4),    // (🗂️..🗄️)    card index dividers cabinet
        NVC_UI_CHAR_RANGE(0x1F5D1, 0x1F5D3),    // (🗑️..🗓️)    wastebasket..spiral calendar
        NVC_UI_CHAR_RANGE(0x1F5DC, 0x1F5DE),    // (🗜️..🗞️)    clamp..rolled-up newspaper
        NVC_UI_CHAR_RANGE(0x1F5E1, 0x1F5E1),    // (🗡️)        dagger
        NVC_UI_CHAR_RANGE(0x1F5E3, 0x1F5E3),    // (🗣️)        speaking head
        NVC_UI_CHAR_RANGE(0x1F5E8, 0x1F5E8),    // (🗨️)        left speech bubble
        NVC_UI_CHAR_RANGE(0x1F5EF, 0x1F5EF),    // (🗯️)        right anger bubble
        NVC_UI_CHAR_RANGE(0x1F5F3, 0x1F5F3),    // (🗳️)        ballot box with ballot
        NVC_UI_CHAR_RANGE(0x1F5FA, 0x1F6C5),    // (🗺️..🛅)    world map..left luggage
        NVC_UI_CHAR_RANGE(0x1F6CB, 0x1F6D2),    // (🛋️..🛒)    couch and lamp..shopping cart
        NVC_UI_CHAR_RANGE(0x1F6D5, 0x1F6D7),    // (🛕..🛗)    hindu temple..elevator
        NVC_UI_CHAR_RANGE(0x1F6DC, 0x1F6E5),    // (🛜..🛥️)    wireless..motor boat
        NVC_UI_CHAR_RANGE(0x1F6E9, 0x1F6E9),    // (🛩️)        small airplane
        NVC_UI_CHAR_RANGE(0x1F6EB, 0x1F6EC),    // (🛫..🛬)    airplane departure..airplane arrival
        NVC_UI_CHAR_RANGE(0x1F6F0, 0x1F6F0),    // (🛰️)        satellite
        NVC_UI_CHAR_RANGE(0x1F6F3, 0x1F6FC),    // (🛳️..🛼)    passenger ship..roller skate
        NVC_UI_CHAR_RANGE(0x1F7E0, 0x1F7EB),    // (🟠..🟫)    orange circle..brown square
        NVC_UI_CHAR_RANGE(0x1F7F0, 0x1F7F0),    // (🟰)        heavy equals sign
        NVC_UI_CHAR_RANGE(0x1F90C, 0x1F93A),    // (🤌..🤺)    pinched fingers..person fencing
        NVC_UI_CHAR_RANGE(0x1F93C, 0x1F945),    // (🤼..🥅)    people wrestling..goal net
        NVC_UI_CHAR_RANGE(0x1F947, 0x1F9FF),    // (🥇..🧿)    1st place medal..nazar amulet
        NVC_UI_CHAR_RANGE(0x1FA70, 0x1FA7C),    // (🩰..🩼)    ballet shoes..crutch
        NVC_UI_CHAR_RANGE(0x1FA80, 0x1FA88),    // (🪀..🪈)    yo-yo..flute
        NVC_UI_CHAR_RANGE(0x1FA90, 0x1FABD),    // (🪐..🪽)    ringed planet..wing
        NVC_UI_CHAR_RANGE(0x1FABF, 0x1FAC5),    // (🪿..🫅)    goose..person with crown
        NVC_UI_CHAR_RANGE(0x1FACE, 0x1FADB),    // (🫎..🫛)    moose..pea pod
        NVC_UI_CHAR_RANGE(0x1FAE0, 0x1FAE8),    // (🫠..🫨)    melting face..shaking face
        NVC_UI_CHAR_RANGE(0x1FAF0, 0x1FAF8),    // (🫰..🫸)    hand with index finger and thumb crossed..rightwards pushing hand
    };

    m_wideSet = load_character_set(wide_ranges, countof(wide_ranges));
    m_cjkSet = load_character_set(cjk_ranges, countof(cjk_ranges));
    m_emojiSet = load_character_set(emoji_ranges, countof(emoji_ranges));
    
    CFMutableCharacterSetRef skipSet = CFCharacterSetCreateMutable(kCFAllocatorDefault);
    if (likely(skipSet != nullptr)) {
        CFCharacterSetAddCharactersInRange(skipSet, CFRangeMake(0, 1));
        CFCharacterSetRef whitespace = CFCharacterSetGetPredefined(kCFCharacterSetWhitespace);
        if (likely(whitespace != nullptr)) {
            CFCharacterSetUnion(skipSet, whitespace);
            CFRelease(whitespace);
        }
    }
    m_skipSet = skipSet;
}


}
