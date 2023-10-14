//
//  nvc_ui_font.cpp
//  NVim
//
//  Created by wizjin on 2023/10/14.
//

#include "nvc_ui_font.h"
#include "nvc_util.h"

namespace nvc {

class UICharacterSet {
private:
    CFMutableCharacterSetRef   m_emojiSet;

    explicit UICharacterSet();
public:
    ~UICharacterSet() {
        if (m_emojiSet != nullptr) {
            CFRelease(m_emojiSet);
            m_emojiSet = nullptr;
        }
    }
    
    inline bool is_emoji(UTF32Char ch) const {
        return m_emojiSet != nullptr && CFCharacterSetIsLongCharacterMember(m_emojiSet, ch);
    }
    
    inline const UICharacterSet& shared(void) {
        return g_character_set;
    }
private:
    static UICharacterSet g_character_set;
};

UICharacterSet UICharacterSet::g_character_set;

UIFont::UIFont() {
    
}

UIFont::~UIFont() {
    clear();
}

void UIFont::clear(void) {
    std::for_each(m_fonts.begin(), m_fonts.end(), CFRelease);
    m_fonts.clear();
}

UIFont& UIFont::operator=(const UIFont& other) {
    if (this != &other) {
        clear();
        for (const auto& p : other.m_fonts) {
            m_fonts.push_back((CTFontRef)CFRetain(p));
        }
    }
    return *this;
}


#pragma mark - UICharacterSet
#define NVC_UI_CHAR_RANGE(_c, _n)   { .location = _c, .length = _n }
UICharacterSet::UICharacterSet() {
    CFRangeMake(0, 1);
    CFRange emojis[] = {
        // Note: https://www.unicode.org/Public/15.1.0/ucd/emoji/emoji-data.txt
        NVC_UI_CHAR_RANGE(0x2139, 1),       // (ℹ️)        information
        NVC_UI_CHAR_RANGE(0x2194, 6),       // (↔️..↙️)    left-right arrow..down-left arrow
        NVC_UI_CHAR_RANGE(0x21A9, 2),       // (↩️..↪️)    right arrow curving left..left arrow curving right
        NVC_UI_CHAR_RANGE(0x231A, 2),       // (⌚..⌛)    watch..hourglass done
        NVC_UI_CHAR_RANGE(0x2328, 1),       // (⌨️)        keyboard
        NVC_UI_CHAR_RANGE(0x23CF, 1),       // (⏏️)        eject button
        NVC_UI_CHAR_RANGE(0x23E9, 11),      // (⏩..⏳)    fast-forward button..hourglass not done
        NVC_UI_CHAR_RANGE(0x23F8, 3),       // (⏸️..⏺️)    pause button..record button
        NVC_UI_CHAR_RANGE(0x24C2, 1),       // (Ⓜ️)        circled M
        NVC_UI_CHAR_RANGE(0x25AA, 2),       // (▪️..▫️)    black small square..white small square
        NVC_UI_CHAR_RANGE(0x25B6, 1),       // (▶️)        play button
        NVC_UI_CHAR_RANGE(0x25C0, 1),       // (◀️)        reverse button
        NVC_UI_CHAR_RANGE(0x25FB, 4),       // (◻️..◾)    white medium square..black medium-small square
        NVC_UI_CHAR_RANGE(0x2600, 5),       // (☀️..☄️)    sun..comet
        NVC_UI_CHAR_RANGE(0x260E, 1),       // (☎️)        telephone
        NVC_UI_CHAR_RANGE(0x2611, 1),       // (☑️)        check box with check
        NVC_UI_CHAR_RANGE(0x2614, 2),       // (☔..☕)    umbrella with rain drops..hot beverage
        NVC_UI_CHAR_RANGE(0x2618, 1),       // (☘️)        shamrock
        NVC_UI_CHAR_RANGE(0x261D, 1),       // (☝️)        index pointing up
        NVC_UI_CHAR_RANGE(0x2620, 1),       // (☠️)        skull and crossbones
        NVC_UI_CHAR_RANGE(0x2622, 2),       // (☢️..☣️)    radioactive..biohazard
        NVC_UI_CHAR_RANGE(0x2626, 1),       // (☦️)        orthodox cross
        NVC_UI_CHAR_RANGE(0x262A, 1),       // (☪️)        star and crescent
        NVC_UI_CHAR_RANGE(0x262E, 2),       // (☮️..☯️)    peace symbol..yin yang
        NVC_UI_CHAR_RANGE(0x2638, 3),       // (☸️..☺️)    wheel of dharma..smiling face
        NVC_UI_CHAR_RANGE(0x2640, 1),       // (♀️)        female sign
        NVC_UI_CHAR_RANGE(0x2642, 1),       // (♂️)        male sign
        NVC_UI_CHAR_RANGE(0x2648, 12),      // (♈..♓)    Aries..Pisces
        NVC_UI_CHAR_RANGE(0x265F, 2),       // (♟️..♠️)    chess pawn..spade suit
        NVC_UI_CHAR_RANGE(0x2663, 1),       // (♣️)        club suit
        NVC_UI_CHAR_RANGE(0x2665, 2),       // (♥️..♦️)    heart suit..diamond suit
        NVC_UI_CHAR_RANGE(0x2668, 1),       // (♨️)        hot springs
        NVC_UI_CHAR_RANGE(0x267B, 1),       // (♻️)        recycling symbol
        NVC_UI_CHAR_RANGE(0x267E, 2),       // (♾️..♿)    infinity..wheelchair symbol
        NVC_UI_CHAR_RANGE(0x2692, 6),       // (⚒️..⚗️)    hammer and pick..balance scale..alembic
        NVC_UI_CHAR_RANGE(0x2699, 1),       // (⚙️)        gear
        NVC_UI_CHAR_RANGE(0x269B, 2),       // (⚛️..⚜️)    atom symbol..fleur-de-lis
        NVC_UI_CHAR_RANGE(0x26A0, 2),       // (⚠️..⚡)    warning..high voltage
        NVC_UI_CHAR_RANGE(0x26A7, 1),       // (⚧️)        transgender symbol
        NVC_UI_CHAR_RANGE(0x26AA, 2),       // (⚪..⚫)    white circle..black circle
        NVC_UI_CHAR_RANGE(0x26B0, 2),       // (⚰️..⚱️)    coffin..funeral urn
        NVC_UI_CHAR_RANGE(0x26BD, 2),       // (⚽..⚾)    soccer ball..baseball
        NVC_UI_CHAR_RANGE(0x26C4, 2),       // (⛄..⛅)    snowman without snow..sun behind cloud
        NVC_UI_CHAR_RANGE(0x26C8, 1),       // (⛈️)        cloud with lightning and rain
        NVC_UI_CHAR_RANGE(0x26CE, 2),       // (⛎..⛏️)    Ophiuchus..pick
        NVC_UI_CHAR_RANGE(0x26D1, 1),       // (⛑️)        rescue worker’s helmet
        NVC_UI_CHAR_RANGE(0x26D3, 2),       // (⛓️..⛔)    chains..no entry
        NVC_UI_CHAR_RANGE(0x26E9, 2),       // (⛩️..⛪)    shinto shrine..church
        NVC_UI_CHAR_RANGE(0x26F0, 6),       // (⛰️..⛵)    mountain..sailboat
        NVC_UI_CHAR_RANGE(0x26F7, 4),       // (⛷️..⛺)    skier..tent
        NVC_UI_CHAR_RANGE(0x26FD, 1),       // (⛽)        fuel pump
        NVC_UI_CHAR_RANGE(0x2702, 1),       // (✂️)        scissors
        NVC_UI_CHAR_RANGE(0x2705, 1),       // (✅)        check mark button
        NVC_UI_CHAR_RANGE(0x2708, 8),       // (✈️..✏️)    airplane..pencil
        NVC_UI_CHAR_RANGE(0x2712, 1),       // (✒️)        black nib
        NVC_UI_CHAR_RANGE(0x2714, 1),       // (✔️)        check mark
        NVC_UI_CHAR_RANGE(0x2716, 1),       // (✖️)        multiply
        NVC_UI_CHAR_RANGE(0x271D, 1),       // (✝️)        latin cross
        NVC_UI_CHAR_RANGE(0x2721, 1),       // (✡️)        star of David
        NVC_UI_CHAR_RANGE(0x2728, 1),       // (✨)        sparkles
        NVC_UI_CHAR_RANGE(0x2733, 2),       // (✳️..✴️)    eight-spoked asterisk..eight-pointed star
        NVC_UI_CHAR_RANGE(0x2744, 1),       // (❄️)        snowflakes
        NVC_UI_CHAR_RANGE(0x2747, 1),       // (❇️)        sparkle
        NVC_UI_CHAR_RANGE(0x274C, 1),       // (❌)        cross mark
        NVC_UI_CHAR_RANGE(0x274E, 1),       // (❎)        cross mark button
        NVC_UI_CHAR_RANGE(0x2753, 3),       // (❓..❕)    red question mark..exclamation mark
        NVC_UI_CHAR_RANGE(0x2757, 1),       // (❗)        red exclamation mark
        NVC_UI_CHAR_RANGE(0x2763, 2),       // (❣️..❤️)    heart exclamation..red heart
        NVC_UI_CHAR_RANGE(0x2795, 3),       // (➕..➗)    plus
        NVC_UI_CHAR_RANGE(0x27A1, 1),       // (➡️)        right arrow
        NVC_UI_CHAR_RANGE(0x27B0, 1),       // (➰)        curly loop
        NVC_UI_CHAR_RANGE(0x27BF, 1),       // (➿)        double curly loop
        NVC_UI_CHAR_RANGE(0x2934, 2),       // (⤴️..⤵️)    right arrow curving up..arrow curving down
        NVC_UI_CHAR_RANGE(0x2B05, 3),       // (⬅️..⬇️)    left arrow..down arrow
        NVC_UI_CHAR_RANGE(0x2B1B, 2),       // (⬛..⬜)    black large square..large square
        NVC_UI_CHAR_RANGE(0x2B50, 1),       // (⭐)        star
        NVC_UI_CHAR_RANGE(0x2B55, 1),       // (⭕)        hollow red circle
        NVC_UI_CHAR_RANGE(0x3030, 1),       // (〰️)        wavy dash
        NVC_UI_CHAR_RANGE(0x303D, 1),       // (〽️)        part alternation mark
        NVC_UI_CHAR_RANGE(0x3297, 1),       // (㊗️)        Japanese “congratulations” button
        NVC_UI_CHAR_RANGE(0x3299, 1),       // (㊙️)        Japanese “secret” button
        NVC_UI_CHAR_RANGE(0x1F004, 1),      // (🀄)        mahjong red dragon
        NVC_UI_CHAR_RANGE(0x1F0CF, 1),      // (🃏)        joker
        NVC_UI_CHAR_RANGE(0x1F170, 2),      // (🅰️..🅱️)    A button (blood type)..button (blood type)
        NVC_UI_CHAR_RANGE(0x1F17E, 2),      // (🅾️..🅿️)    O button (blood type)..button
        NVC_UI_CHAR_RANGE(0x1F18E, 1),      // (🆎)        AB button (blood type)
        NVC_UI_CHAR_RANGE(0x1F191, 10),     // (🆑..🆚)    CL button button
        NVC_UI_CHAR_RANGE(0x1F201, 2),      // (🈁..🈂️)    Japanese “here” button..“service charge” button
        NVC_UI_CHAR_RANGE(0x1F21A, 1),      // (🈚)        Japanese “free of charge” button
        NVC_UI_CHAR_RANGE(0x1F22F, 1),      // (🈯)        Japanese “reserved” button
        NVC_UI_CHAR_RANGE(0x1F232, 9),      // (🈲..🈺)    Japanese “prohibited” button..“open for business” button
        NVC_UI_CHAR_RANGE(0x1F250, 2),      // (🉐..🉑)    Japanese “bargain” button..“acceptable” button
        NVC_UI_CHAR_RANGE(0x1F300, 34),     // (🌀..🌡️)    cyclone..thermometer
        NVC_UI_CHAR_RANGE(0x1F324, 112),    // (🌤️..🎓)    sun behind small cloud..graduation cap
        NVC_UI_CHAR_RANGE(0x1F396, 2),      // (🎖️..🎗️)    military medal..reminder ribbon
        NVC_UI_CHAR_RANGE(0x1F399, 3),      // (🎙️..🎛️)    studio microphone..control knobs
        NVC_UI_CHAR_RANGE(0x1F39E, 83),     // (🎞️..🏰)    film frames..castle
        NVC_UI_CHAR_RANGE(0x1F3F3, 3),      // (🏳️..🏵️)    white flag..rosette
        NVC_UI_CHAR_RANGE(0x1F3F7, 263),    // (🏷️..📽️)    label..film projector
        NVC_UI_CHAR_RANGE(0x1F4FF, 63),     // (📿..🔽)    prayer beads..downwards button
        NVC_UI_CHAR_RANGE(0x1F549, 6),      // (🕉️..🕎)    om..menorah
        NVC_UI_CHAR_RANGE(0x1F550, 24),     // (🕐..🕧)    one o’clock..twelve-thirty
        NVC_UI_CHAR_RANGE(0x1F56F, 2),      // (🕯️..🕰️)    candle..mantelpiece clock
        NVC_UI_CHAR_RANGE(0x1F573, 7),      // (🕳️..🕹️)    hole..joystick
        NVC_UI_CHAR_RANGE(0x1F57A, 1),      // (🕺)        man dancing
        NVC_UI_CHAR_RANGE(0x1F587, 1),      // (🖇️)        linked paperclips
        NVC_UI_CHAR_RANGE(0x1F58A, 4),      // (🖊️..🖍️)    pen..crayon
        NVC_UI_CHAR_RANGE(0x1F590, 1),      // (🖐️)        hand with fingers splayed
        NVC_UI_CHAR_RANGE(0x1F595, 2),      // (🖕..🖖)    middle finger..vulcan salute
        NVC_UI_CHAR_RANGE(0x1F5A4, 2),      // (🖤..🖥️)    black heart..desktop computer
        NVC_UI_CHAR_RANGE(0x1F5A8, 1),      // (🖨️)        printer
        NVC_UI_CHAR_RANGE(0x1F5B1, 2),      // (🖱️..🖲️)    computer mouse..trackball
        NVC_UI_CHAR_RANGE(0x1F5BC, 1),      // (🖼️)        framed picture
        NVC_UI_CHAR_RANGE(0x1F5C2, 3),      // (🗂️..🗄️)    card index dividers cabinet
        NVC_UI_CHAR_RANGE(0x1F5D1, 3),      // (🗑️..🗓️)    wastebasket..spiral calendar
        NVC_UI_CHAR_RANGE(0x1F5DC, 3),      // (🗜️..🗞️)    clamp..rolled-up newspaper
        NVC_UI_CHAR_RANGE(0x1F5E1, 1),      // (🗡️)        dagger
        NVC_UI_CHAR_RANGE(0x1F5E3, 1),      // (🗣️)        speaking head
        NVC_UI_CHAR_RANGE(0x1F5E8, 1),      // (🗨️)        left speech bubble
        NVC_UI_CHAR_RANGE(0x1F5EF, 1),      // (🗯️)        right anger bubble
        NVC_UI_CHAR_RANGE(0x1F5F3, 1),      // (🗳️)        ballot box with ballot
        NVC_UI_CHAR_RANGE(0x1F5FA, 204),    // (🗺️..🛅)    world map..left luggage
        NVC_UI_CHAR_RANGE(0x1F6CB, 8),      // (🛋️..🛒)    couch and lamp..shopping cart
        NVC_UI_CHAR_RANGE(0x1F6D5, 3),      // (🛕..🛗)    hindu temple..elevator
        NVC_UI_CHAR_RANGE(0x1F6DC, 10),     // (🛜..🛥️)    wireless..motor boat
        NVC_UI_CHAR_RANGE(0x1F6E9, 1),      // (🛩️)        small airplane
        NVC_UI_CHAR_RANGE(0x1F6EB, 2),      // (🛫..🛬)    airplane departure..airplane arrival
        NVC_UI_CHAR_RANGE(0x1F6F0, 1),      // (🛰️)        satellite
        NVC_UI_CHAR_RANGE(0x1F6F3, 10),     // (🛳️..🛼)    passenger ship..roller skate
        NVC_UI_CHAR_RANGE(0x1F7E0, 12),     // (🟠..🟫)    orange circle..brown square
        NVC_UI_CHAR_RANGE(0x1F7F0, 1),      // (🟰)        heavy equals sign
        NVC_UI_CHAR_RANGE(0x1F90C, 47),     // (🤌..🤺)    pinched fingers..person fencing
        NVC_UI_CHAR_RANGE(0x1F93C, 10),     // (🤼..🥅)    people wrestling..goal net
        NVC_UI_CHAR_RANGE(0x1F947, 185),    // (🥇..🧿)    1st place medal..nazar amulet
        NVC_UI_CHAR_RANGE(0x1FA70, 13),     // (🩰..🩼)    ballet shoes..crutch
        NVC_UI_CHAR_RANGE(0x1FA80, 9),      // (🪀..🪈)    yo-yo..flute
        NVC_UI_CHAR_RANGE(0x1FA90, 46),     // (🪐..🪽)    ringed planet..wing
        NVC_UI_CHAR_RANGE(0x1FABF, 7),      // (🪿..🫅)    goose..person with crown
        NVC_UI_CHAR_RANGE(0x1FACE, 14),     // (🫎..🫛)    moose..pea pod
        NVC_UI_CHAR_RANGE(0x1FAE0, 9),      // (🫠..🫨)    melting face..shaking face
        NVC_UI_CHAR_RANGE(0x1FAF0, 9),      // (🫰..🫸)    hand with index finger and thumb crossed..rightwards pushing hand
    };
    m_emojiSet = CFCharacterSetCreateMutable(kCFAllocatorDefault);
    if (likely(m_emojiSet != nullptr)) {
        for (int i = 0; i < countof(emojis); i++) {
            CFCharacterSetAddCharactersInRange(m_emojiSet, emojis[i]);
        }
    }
}

}

