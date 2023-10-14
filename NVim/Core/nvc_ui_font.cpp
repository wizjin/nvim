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
        NVC_UI_CHAR_RANGE(0x2139, 1),   // (ℹ️)        information
        NVC_UI_CHAR_RANGE(0x2194, 6),   // (↔️..↙️)    left-right arrow..down-left arrow
        NVC_UI_CHAR_RANGE(0x21A9, 2),   // (↩️..↪️)    right arrow curving left..left arrow curving right
        NVC_UI_CHAR_RANGE(0x231A, 2),   // (⌚..⌛)    watch..hourglass done
        NVC_UI_CHAR_RANGE(0x2328, 1),   // (⌨️)        keyboard
        NVC_UI_CHAR_RANGE(0x23CF, 1),   // (⏏️)        eject button
        NVC_UI_CHAR_RANGE(0x23E9, 4),   // (⏩..⏬)    fast-forward button..fast down button
        NVC_UI_CHAR_RANGE(0x23ED, 2),   // (⏭️..⏮️)    next track button..last track button
        NVC_UI_CHAR_RANGE(0x23EF, 1),   // (⏯️)        play or pause button
        NVC_UI_CHAR_RANGE(0x23F0, 1),   // (⏰)        alarm clock
        NVC_UI_CHAR_RANGE(0x23F1, 2),   // (⏱️..⏲️)    stopwatch..timer clock
        NVC_UI_CHAR_RANGE(0x23F3, 1),   // (⏳)        hourglass not done
        NVC_UI_CHAR_RANGE(0x23F8, 3),   // (⏸️..⏺️)    pause button..record button
        NVC_UI_CHAR_RANGE(0x24C2, 1),   // (Ⓜ️)        circled M
        NVC_UI_CHAR_RANGE(0x25AA, 2),   // (▪️..▫️)    black small square..white small square
        NVC_UI_CHAR_RANGE(0x25B6, 1),   // (▶️)        play button
        NVC_UI_CHAR_RANGE(0x25C0, 1),   // (◀️)        reverse button
        NVC_UI_CHAR_RANGE(0x25FB, 4),   // (◻️..◾)    white medium square..black medium-small square
        NVC_UI_CHAR_RANGE(0x2600, 2),   // (☀️..☁️)    sun..cloud
        NVC_UI_CHAR_RANGE(0x2602, 2),   // (☂️..☃️)    umbrella..snowman
        NVC_UI_CHAR_RANGE(0x2604, 1),   // (☄️)        comet
        NVC_UI_CHAR_RANGE(0x260E, 1),   // (☎️)        telephone
        NVC_UI_CHAR_RANGE(0x2611, 1),   // (☑️)        check box with check
        NVC_UI_CHAR_RANGE(0x2614, 2),   // (☔..☕)    umbrella with rain drops..hot beverage
        NVC_UI_CHAR_RANGE(0x2618, 1),   // (☘️)        shamrock
        NVC_UI_CHAR_RANGE(0x261D, 1),   // (☝️)        index pointing up
        NVC_UI_CHAR_RANGE(0x2620, 1),   // (☠️)        skull and crossbones
        NVC_UI_CHAR_RANGE(0x2622, 2),   // (☢️..☣️)    radioactive..biohazard
        NVC_UI_CHAR_RANGE(0x2626, 1),   // (☦️)        orthodox cross
        NVC_UI_CHAR_RANGE(0x262A, 1),   // (☪️)        star and crescent
        NVC_UI_CHAR_RANGE(0x262E, 1),   // (☮️)        peace symbol
        NVC_UI_CHAR_RANGE(0x262F, 1),   // (☯️)        yin yang
        NVC_UI_CHAR_RANGE(0x2638, 2),   // (☸️..☹️)    wheel of dharma..frowning face
        NVC_UI_CHAR_RANGE(0x263A, 1),   // (☺️)        smiling face
        NVC_UI_CHAR_RANGE(0x2640, 1),   // (♀️)        female sign
        NVC_UI_CHAR_RANGE(0x2642, 1),   // (♂️)        male sign
        NVC_UI_CHAR_RANGE(0x2648, 12),  // (♈..♓)    Aries..Pisces
        NVC_UI_CHAR_RANGE(0x265F, 1),   // (♟️)        chess pawn
        NVC_UI_CHAR_RANGE(0x2660, 1),   // (♠️)        spade suit
        NVC_UI_CHAR_RANGE(0x2663, 1),   // (♣️)        club suit
        NVC_UI_CHAR_RANGE(0x2665, 2),   // (♥️..♦️)    heart suit..diamond suit
        NVC_UI_CHAR_RANGE(0x2668, 1),   // (♨️)        hot springs
        NVC_UI_CHAR_RANGE(0x267B, 1),   // (♻️)        recycling symbol
        NVC_UI_CHAR_RANGE(0x267E, 1),   // (♾️)        infinity
        NVC_UI_CHAR_RANGE(0x267F, 1),   // (♿)        wheelchair symbol
        NVC_UI_CHAR_RANGE(0x2692, 1),   // (⚒️)        hammer and pick
        NVC_UI_CHAR_RANGE(0x2693, 1),   // (⚓)        anchor
        NVC_UI_CHAR_RANGE(0x2694, 1),   // (⚔️)        crossed swords
        NVC_UI_CHAR_RANGE(0x2695, 1),   // (⚕️)        medical symbol
        NVC_UI_CHAR_RANGE(0x2696, 2),   // (⚖️..⚗️)    balance scale..alembic
        NVC_UI_CHAR_RANGE(0x2699, 1),   // (⚙️)        gear
        NVC_UI_CHAR_RANGE(0x269B, 2),   // (⚛️..⚜️)    atom symbol..fleur-de-lis
        NVC_UI_CHAR_RANGE(0x26A0, 2),   // (⚠️..⚡)    warning..high voltage
        NVC_UI_CHAR_RANGE(0x26A7, 1),   // (⚧️)        transgender symbol
        NVC_UI_CHAR_RANGE(0x26AA, 2),   // (⚪..⚫)    white circle..black circle
        NVC_UI_CHAR_RANGE(0x26B0, 2),   // (⚰️..⚱️)    coffin..funeral urn
        NVC_UI_CHAR_RANGE(0x26BD, 2),   // (⚽..⚾)    soccer ball..baseball
        NVC_UI_CHAR_RANGE(0x26C4, 2),   // (⛄..⛅)    snowman without snow..sun behind cloud
        NVC_UI_CHAR_RANGE(0x26C8, 1),   // (⛈️)        cloud with lightning and rain
        NVC_UI_CHAR_RANGE(0x26CE, 1),   // (⛎)        Ophiuchus
        NVC_UI_CHAR_RANGE(0x26CF, 1),   // (⛏️)        pick
        NVC_UI_CHAR_RANGE(0x26D1, 1),   // (⛑️)        rescue worker’s helmet
        NVC_UI_CHAR_RANGE(0x26D3, 1),   // (⛓️)        chains
        NVC_UI_CHAR_RANGE(0x26D4, 1),   // (⛔)        no entry
        NVC_UI_CHAR_RANGE(0x26E9, 1),   // (⛩️)        shinto shrine
        NVC_UI_CHAR_RANGE(0x26EA, 1),   // (⛪)        church
        NVC_UI_CHAR_RANGE(0x26F0, 2),   // (⛰️..⛱️)    mountain..umbrella on ground
        NVC_UI_CHAR_RANGE(0x26F2, 2),   // (⛲..⛳)    fountain..flag in hole
        NVC_UI_CHAR_RANGE(0x26F4, 1),   // (⛴️)        ferry
        NVC_UI_CHAR_RANGE(0x26F5, 1),   // (⛵)        sailboat
        NVC_UI_CHAR_RANGE(0x26F7, 3),   // (⛷️..⛹️)    skier..person bouncing ball
        NVC_UI_CHAR_RANGE(0x26FA, 1),   // (⛺)        tent
        NVC_UI_CHAR_RANGE(0x26FD, 1),   // (⛽)        fuel pump
        NVC_UI_CHAR_RANGE(0x2702, 1),   // (✂️)        scissors
        NVC_UI_CHAR_RANGE(0x2705, 1),   // (✅)        check mark button
        NVC_UI_CHAR_RANGE(0x2708, 5),   // (✈️..✌️)    airplane..victory hand
        NVC_UI_CHAR_RANGE(0x270D, 1),   // (✍️)        writing hand
        NVC_UI_CHAR_RANGE(0x270F, 1),   // (✏️)        pencil
        NVC_UI_CHAR_RANGE(0x2712, 1),   // (✒️)        black nib
        NVC_UI_CHAR_RANGE(0x2714, 1),   // (✔️)        check mark
        NVC_UI_CHAR_RANGE(0x2716, 1),   // (✖️)        multiply
        NVC_UI_CHAR_RANGE(0x271D, 1),   // (✝️)        latin cross
        NVC_UI_CHAR_RANGE(0x2721, 1),   // (✡️)        star of David
        NVC_UI_CHAR_RANGE(0x2728, 1),   // (✨)        sparkles
        NVC_UI_CHAR_RANGE(0x2733, 2),   // (✳️..✴️)    eight-spoked asterisk..eight-pointed star
        NVC_UI_CHAR_RANGE(0x2744, 1),   // (❄️)        snowflake
        NVC_UI_CHAR_RANGE(0x2747, 1),   // (❇️)        sparkle
        NVC_UI_CHAR_RANGE(0x274C, 1),   // (❌)        cross mark
        NVC_UI_CHAR_RANGE(0x274E, 1),   // (❎)        cross mark button
        NVC_UI_CHAR_RANGE(0x2753, 3),   // (❓..❕)    red question mark exclamation mark
        NVC_UI_CHAR_RANGE(0x2757, 1),   // (❗)        red exclamation mark
        NVC_UI_CHAR_RANGE(0x2763, 1),   // (❣️)        heart exclamation
        NVC_UI_CHAR_RANGE(0x2764, 1),   // (❤️)        red heart
        NVC_UI_CHAR_RANGE(0x2795, 3),   // (➕..➗)    plus
        NVC_UI_CHAR_RANGE(0x27A1, 1),   // (➡️)        right arrow
        NVC_UI_CHAR_RANGE(0x27B0, 1),   // (➰)        curly loop
        NVC_UI_CHAR_RANGE(0x27BF, 1),   // (➿)        double curly loop
        NVC_UI_CHAR_RANGE(0x2934, 2),   // (⤴️..⤵️)    right arrow curving up arrow curving down
        NVC_UI_CHAR_RANGE(0x2B05, 3),   // (⬅️..⬇️)    left arrow arrow
        NVC_UI_CHAR_RANGE(0x2B1B, 2),   // (⬛..⬜)    black large square large square
        NVC_UI_CHAR_RANGE(0x2B50, 1),   // (⭐)        star
        NVC_UI_CHAR_RANGE(0x2B55, 1),   // (⭕)        hollow red circle
        NVC_UI_CHAR_RANGE(0x3030, 1),   // (〰️)        wavy dash
        NVC_UI_CHAR_RANGE(0x303D, 1),   // (〽️)        part alternation mark
        NVC_UI_CHAR_RANGE(0x3297, 1),   // (㊗️)        Japanese “congratulations” button
        NVC_UI_CHAR_RANGE(0x3299, 1),   // (㊙️)        Japanese “secret” button
        NVC_UI_CHAR_RANGE(0x1F004, 1),   // (🀄)       mahjong red dragon
        NVC_UI_CHAR_RANGE(0x1F0CF, 1),   // (🃏)       joker
        NVC_UI_CHAR_RANGE(0x1F170, 2),   // (🅰️..🅱️)   A button (blood type) button (blood type)
        NVC_UI_CHAR_RANGE(0x1F17E, 2),   // (🅾️..🅿️)   O button (blood type) button
        NVC_UI_CHAR_RANGE(0x1F18E, 1),   // (🆎)       AB button (blood type)
        NVC_UI_CHAR_RANGE(0x1F191, 10),  // (🆑..🆚)   CL button button
        NVC_UI_CHAR_RANGE(0x1F1E6, 26),  // (🇦..🇿)   regional indicator symbol letter a indicator symbol letter z
        NVC_UI_CHAR_RANGE(0x1F201, 2),   // (🈁..🈂️)   Japanese “here” button “service charge” button
        NVC_UI_CHAR_RANGE(0x1F21A, 1),   // (🈚)       Japanese “free of charge” button
        NVC_UI_CHAR_RANGE(0x1F22F, 1),   // (🈯)       Japanese “reserved” button
        NVC_UI_CHAR_RANGE(0x1F232, 9),   // (🈲..🈺)   Japanese “prohibited” button “open for business” button
        NVC_UI_CHAR_RANGE(0x1F250, 2),   // (🉐..🉑)   Japanese “bargain” button “acceptable” button
        NVC_UI_CHAR_RANGE(0x1F300, 13),  // (🌀..🌌)   cyclone way
        NVC_UI_CHAR_RANGE(0x1F30D, 2),   // (🌍..🌎)   globe showing Europe-Africa showing Americas
        NVC_UI_CHAR_RANGE(0x1F30F, 1),   // (🌏)       globe showing Asia-Australia
        NVC_UI_CHAR_RANGE(0x1F310, 1),   // (🌐)       globe with meridians
        NVC_UI_CHAR_RANGE(0x1F311, 1),   // (🌑)       new moon
        NVC_UI_CHAR_RANGE(0x1F312, 1),   // (🌒)       waxing crescent moon
        NVC_UI_CHAR_RANGE(0x1F313, 3),   // (🌓..🌕)   first quarter moon moon
        NVC_UI_CHAR_RANGE(0x1F316, 3),   // (🌖..🌘)   waning gibbous moon crescent moon
        NVC_UI_CHAR_RANGE(0x1F319, 1),   // (🌙)       crescent moon
        NVC_UI_CHAR_RANGE(0x1F31A, 1),   // (🌚)       new moon face
        NVC_UI_CHAR_RANGE(0x1F31B, 1),   // (🌛)       first quarter moon face
        NVC_UI_CHAR_RANGE(0x1F31C, 1),   // (🌜)       last quarter moon face
        NVC_UI_CHAR_RANGE(0x1F31D, 2),   // (🌝..🌞)   full moon face with face
        NVC_UI_CHAR_RANGE(0x1F31F, 2),   // (🌟..🌠)   glowing star star
        NVC_UI_CHAR_RANGE(0x1F321, 1),   // (🌡️)       thermometer
        NVC_UI_CHAR_RANGE(0x1F324, 9),   // (🌤️..🌬️)   sun behind small cloud face
        NVC_UI_CHAR_RANGE(0x1F32D, 3),   // (🌭..🌯)   hot dog
        NVC_UI_CHAR_RANGE(0x1F330, 2),   // (🌰..🌱)   chestnut
        NVC_UI_CHAR_RANGE(0x1F332, 2),   // (🌲..🌳)   evergreen tree tree
        NVC_UI_CHAR_RANGE(0x1F334, 2),   // (🌴..🌵)   palm tree
        NVC_UI_CHAR_RANGE(0x1F336, 1),   // (🌶️)       hot pepper
        NVC_UI_CHAR_RANGE(0x1F337, 20),  // (🌷..🍊)   tulip
        NVC_UI_CHAR_RANGE(0x1F34B, 1),   // (🍋)       lemon
        NVC_UI_CHAR_RANGE(0x1F34C, 4),   // (🍌..🍏)   banana apple
        NVC_UI_CHAR_RANGE(0x1F350, 1),   // (🍐)       pear
        NVC_UI_CHAR_RANGE(0x1F351, 43),  // (🍑..🍻)   peach beer mugs
        NVC_UI_CHAR_RANGE(0x1F37C, 1),   // (🍼)       baby bottle
        NVC_UI_CHAR_RANGE(0x1F37D, 1),   // (🍽️)       fork and knife with plate
        NVC_UI_CHAR_RANGE(0x1F37E, 2),   // (🍾..🍿)   bottle with popping cork
        NVC_UI_CHAR_RANGE(0x1F380, 20),  // (🎀..🎓)   ribbon cap
        NVC_UI_CHAR_RANGE(0x1F396, 2),   // (🎖️..🎗️)   military medal ribbon
        NVC_UI_CHAR_RANGE(0x1F399, 3),   // (🎙️..🎛️)   studio microphone knobs
        NVC_UI_CHAR_RANGE(0x1F39E, 2),   // (🎞️..🎟️)   film frames tickets
        NVC_UI_CHAR_RANGE(0x1F3A0, 37),  // (🎠..🏄)   carousel horse surfing
        NVC_UI_CHAR_RANGE(0x1F3C5, 1),   // (🏅)       sports medal
        NVC_UI_CHAR_RANGE(0x1F3C6, 1),   // (🏆)       trophy
        NVC_UI_CHAR_RANGE(0x1F3C7, 1),   // (🏇)       horse racing
        NVC_UI_CHAR_RANGE(0x1F3C8, 1),   // (🏈)       american football
        NVC_UI_CHAR_RANGE(0x1F3C9, 1),   // (🏉)       rugby football
        NVC_UI_CHAR_RANGE(0x1F3CA, 1),   // (🏊)       person swimming
        NVC_UI_CHAR_RANGE(0x1F3CB, 4),   // (🏋️..🏎️)   person lifting weights car
        NVC_UI_CHAR_RANGE(0x1F3CF, 5),   // (🏏..🏓)   cricket game pong
        NVC_UI_CHAR_RANGE(0x1F3D4, 12),  // (🏔️..🏟️)   snow-capped mountain
        NVC_UI_CHAR_RANGE(0x1F3E0, 4),   // (🏠..🏣)   house post office
        NVC_UI_CHAR_RANGE(0x1F3E4, 1),   // (🏤)       post office
        NVC_UI_CHAR_RANGE(0x1F3E5, 12),  // (🏥..🏰)   hospital
        NVC_UI_CHAR_RANGE(0x1F3F3, 1),   // (🏳️)       white flag
        NVC_UI_CHAR_RANGE(0x1F3F4, 1),   // (🏴)       black flag
        NVC_UI_CHAR_RANGE(0x1F3F5, 1),   // (🏵️)       rosette
        NVC_UI_CHAR_RANGE(0x1F3F7, 1),   // (🏷️)       label
        NVC_UI_CHAR_RANGE(0x1F3F8, 16),  // (🏸..🐇)   badminton
        NVC_UI_CHAR_RANGE(0x1F408, 1),   // (🐈)       cat
        NVC_UI_CHAR_RANGE(0x1F409, 3),   // (🐉..🐋)   dragon
        NVC_UI_CHAR_RANGE(0x1F40C, 3),   // (🐌..🐎)   snail
        NVC_UI_CHAR_RANGE(0x1F40F, 2),   // (🐏..🐐)   ram
        NVC_UI_CHAR_RANGE(0x1F411, 2),   // (🐑..🐒)   ewe
        NVC_UI_CHAR_RANGE(0x1F413, 1),   // (🐓)       rooster
        NVC_UI_CHAR_RANGE(0x1F414, 1),   // (🐔)       chicken
        NVC_UI_CHAR_RANGE(0x1F415, 1),   // (🐕)       dog
        NVC_UI_CHAR_RANGE(0x1F416, 1),   // (🐖)       pig
        NVC_UI_CHAR_RANGE(0x1F417, 19),  // (🐗..🐩)   boar
        NVC_UI_CHAR_RANGE(0x1F42A, 1),   // (🐪)       camel
        NVC_UI_CHAR_RANGE(0x1F42B, 20),  // (🐫..🐾)   two-hump camel prints
        NVC_UI_CHAR_RANGE(0x1F43F, 1),   // (🐿️)       chipmunk
        NVC_UI_CHAR_RANGE(0x1F440, 1),   // (👀)       eyes
        NVC_UI_CHAR_RANGE(0x1F441, 1),   // (👁️)       eye
        NVC_UI_CHAR_RANGE(0x1F442, 35),  // (👂..👤)   ear in silhouette
        NVC_UI_CHAR_RANGE(0x1F465, 1),   // (👥)       busts in silhouette
        NVC_UI_CHAR_RANGE(0x1F466, 6),   // (👦..👫)   boy and man holding hands
        NVC_UI_CHAR_RANGE(0x1F46C, 2),   // (👬..👭)   men holding hands holding hands
        NVC_UI_CHAR_RANGE(0x1F46E, 63),  // (👮..💬)   police officer balloon
        NVC_UI_CHAR_RANGE(0x1F4AD, 1),   // (💭)       thought balloon
        NVC_UI_CHAR_RANGE(0x1F4AE, 8),   // (💮..💵)   white flower banknote
        NVC_UI_CHAR_RANGE(0x1F4B6, 2),   // (💶..💷)   euro banknote banknote
        NVC_UI_CHAR_RANGE(0x1F4B8, 52),  // (💸..📫)   money with wings mailbox with raised flag
        NVC_UI_CHAR_RANGE(0x1F4EC, 2),   // (📬..📭)   open mailbox with raised flag mailbox with lowered flag
        NVC_UI_CHAR_RANGE(0x1F4EE, 1),   // (📮)       postbox
        NVC_UI_CHAR_RANGE(0x1F4EF, 1),   // (📯)       postal horn
        NVC_UI_CHAR_RANGE(0x1F4F0, 5),   // (📰..📴)   newspaper phone off
        NVC_UI_CHAR_RANGE(0x1F4F5, 1),   // (📵)       no mobile phones
        NVC_UI_CHAR_RANGE(0x1F4F6, 2),   // (📶..📷)   antenna bars
        NVC_UI_CHAR_RANGE(0x1F4F8, 1),   // (📸)       camera with flash
        NVC_UI_CHAR_RANGE(0x1F4F9, 4),   // (📹..📼)   video camera
        NVC_UI_CHAR_RANGE(0x1F4FD, 1),   // (📽️)       film projector
        NVC_UI_CHAR_RANGE(0x1F4FF, 4),   // (📿..🔂)   prayer beads single button
        NVC_UI_CHAR_RANGE(0x1F503, 1),   // (🔃)       clockwise vertical arrows
        NVC_UI_CHAR_RANGE(0x1F504, 4),   // (🔄..🔇)   counterclockwise arrows button speaker
        NVC_UI_CHAR_RANGE(0x1F508, 1),   // (🔈)       speaker low volume
        NVC_UI_CHAR_RANGE(0x1F509, 1),   // (🔉)       speaker medium volume
        NVC_UI_CHAR_RANGE(0x1F50A, 11),  // (🔊..🔔)   speaker high volume
        NVC_UI_CHAR_RANGE(0x1F515, 1),   // (🔕)       bell with slash
        NVC_UI_CHAR_RANGE(0x1F516, 22),  // (🔖..🔫)   bookmark pistol
        NVC_UI_CHAR_RANGE(0x1F52C, 2),   // (🔬..🔭)   microscope
        NVC_UI_CHAR_RANGE(0x1F52E, 16),  // (🔮..🔽)   crystal ball button
        NVC_UI_CHAR_RANGE(0x1F549, 2),   // (🕉️..🕊️)   om
        NVC_UI_CHAR_RANGE(0x1F54B, 4),   // (🕋..🕎)   kaaba
        NVC_UI_CHAR_RANGE(0x1F550, 12),  // (🕐..🕛)   one o’clock o’clock
        NVC_UI_CHAR_RANGE(0x1F55C, 12),  // (🕜..🕧)   one-thirty-thirty
        NVC_UI_CHAR_RANGE(0x1F56F, 2),   // (🕯️..🕰️)   candle clock
        NVC_UI_CHAR_RANGE(0x1F573, 7),   // (🕳️..🕹️)   hole
        NVC_UI_CHAR_RANGE(0x1F57A, 1),   // (🕺)       man dancing
        NVC_UI_CHAR_RANGE(0x1F587, 1),   // (🖇️)       linked paperclips
        NVC_UI_CHAR_RANGE(0x1F58A, 4),   // (🖊️..🖍️)   pen
        NVC_UI_CHAR_RANGE(0x1F590, 1),   // (🖐️)       hand with fingers splayed
        NVC_UI_CHAR_RANGE(0x1F595, 2),   // (🖕..🖖)   middle finger salute
        NVC_UI_CHAR_RANGE(0x1F5A4, 1),   // (🖤)       black heart
        NVC_UI_CHAR_RANGE(0x1F5A5, 1),   // (🖥️)       desktop computer
        NVC_UI_CHAR_RANGE(0x1F5A8, 1),   // (🖨️)       printer
        NVC_UI_CHAR_RANGE(0x1F5B1, 2),   // (🖱️..🖲️)   computer mouse
        NVC_UI_CHAR_RANGE(0x1F5BC, 1),   // (🖼️)       framed picture
        NVC_UI_CHAR_RANGE(0x1F5C2, 3),   // (🗂️..🗄️)   card index dividers cabinet
        NVC_UI_CHAR_RANGE(0x1F5D1, 3),   // (🗑️..🗓️)   wastebasket calendar
        NVC_UI_CHAR_RANGE(0x1F5DC, 3),   // (🗜️..🗞️)   clamp-up newspaper
        NVC_UI_CHAR_RANGE(0x1F5E1, 1),   // (🗡️)       dagger
        NVC_UI_CHAR_RANGE(0x1F5E3, 1),   // (🗣️)       speaking head
        NVC_UI_CHAR_RANGE(0x1F5E8, 1),   // (🗨️)       left speech bubble
        NVC_UI_CHAR_RANGE(0x1F5EF, 1),   // (🗯️)       right anger bubble
        NVC_UI_CHAR_RANGE(0x1F5F3, 1),   // (🗳️)       ballot box with ballot
        NVC_UI_CHAR_RANGE(0x1F5FA, 1),   // (🗺️)       world map
        NVC_UI_CHAR_RANGE(0x1F5FB, 5),   // (🗻..🗿)   mount fuji
        NVC_UI_CHAR_RANGE(0x1F600, 1),   // (😀)       grinning face
        NVC_UI_CHAR_RANGE(0x1F601, 6),   // (😁..😆)   beaming face with smiling eyes squinting face
        NVC_UI_CHAR_RANGE(0x1F607, 2),   // (😇..😈)   smiling face with halo face with horns
        NVC_UI_CHAR_RANGE(0x1F609, 5),   // (😉..😍)   winking face face with heart-eyes
        NVC_UI_CHAR_RANGE(0x1F60E, 1),   // (😎)       smiling face with sunglasses
        NVC_UI_CHAR_RANGE(0x1F60F, 1),   // (😏)       smirking face
        NVC_UI_CHAR_RANGE(0x1F610, 1),   // (😐)       neutral face
        NVC_UI_CHAR_RANGE(0x1F611, 1),   // (😑)       expressionless face
        NVC_UI_CHAR_RANGE(0x1F612, 3),   // (😒..😔)   unamused face face
        NVC_UI_CHAR_RANGE(0x1F615, 1),   // (😕)       confused face
        NVC_UI_CHAR_RANGE(0x1F616, 1),   // (😖)       confounded face
        NVC_UI_CHAR_RANGE(0x1F617, 1),   // (😗)       kissing face
        NVC_UI_CHAR_RANGE(0x1F618, 1),   // (😘)       face blowing a kiss
        NVC_UI_CHAR_RANGE(0x1F619, 1),   // (😙)       kissing face with smiling eyes
        NVC_UI_CHAR_RANGE(0x1F61A, 1),   // (😚)       kissing face with closed eyes
        NVC_UI_CHAR_RANGE(0x1F61B, 1),   // (😛)       face with tongue
        NVC_UI_CHAR_RANGE(0x1F61C, 3),   // (😜..😞)   winking face with tongue face
        NVC_UI_CHAR_RANGE(0x1F61F, 1),   // (😟)       worried face
        NVC_UI_CHAR_RANGE(0x1F620, 6),   // (😠..😥)   angry face but relieved face
        NVC_UI_CHAR_RANGE(0x1F626, 2),   // (😦..😧)   frowning face with open mouth face
        NVC_UI_CHAR_RANGE(0x1F628, 4),   // (😨..😫)   fearful face face
        NVC_UI_CHAR_RANGE(0x1F62C, 1),   // (😬)       grimacing face
        NVC_UI_CHAR_RANGE(0x1F62D, 1),   // (😭)       loudly crying face
        NVC_UI_CHAR_RANGE(0x1F62E, 2),   // (😮..😯)   face with open mouth face
        NVC_UI_CHAR_RANGE(0x1F630, 4),   // (😰..😳)   anxious face with sweat face
        NVC_UI_CHAR_RANGE(0x1F634, 1),   // (😴)       sleeping face
        NVC_UI_CHAR_RANGE(0x1F635, 1),   // (😵)       face with crossed-out eyes
        NVC_UI_CHAR_RANGE(0x1F636, 1),   // (😶)       face without mouth
        NVC_UI_CHAR_RANGE(0x1F637, 10),  // (😷..🙀)   face with medical mask cat
        NVC_UI_CHAR_RANGE(0x1F641, 4),   // (🙁..🙄)   slightly frowning face with rolling eyes
        NVC_UI_CHAR_RANGE(0x1F645, 11),  // (🙅..🙏)   person gesturing NO hands
        NVC_UI_CHAR_RANGE(0x1F680, 1),   // (🚀)       rocket
        NVC_UI_CHAR_RANGE(0x1F681, 2),   // (🚁..🚂)   helicopter
        NVC_UI_CHAR_RANGE(0x1F683, 3),   // (🚃..🚅)   railway car train
        NVC_UI_CHAR_RANGE(0x1F686, 1),   // (🚆)       train
        NVC_UI_CHAR_RANGE(0x1F687, 1),   // (🚇)       metro
        NVC_UI_CHAR_RANGE(0x1F688, 1),   // (🚈)       light rail
        NVC_UI_CHAR_RANGE(0x1F689, 1),   // (🚉)       station
        NVC_UI_CHAR_RANGE(0x1F68A, 2),   // (🚊..🚋)   tram car
        NVC_UI_CHAR_RANGE(0x1F68C, 1),   // (🚌)       bus
        NVC_UI_CHAR_RANGE(0x1F68D, 1),   // (🚍)       oncoming bus
        NVC_UI_CHAR_RANGE(0x1F68E, 1),   // (🚎)       trolleybus
        NVC_UI_CHAR_RANGE(0x1F68F, 1),   // (🚏)       bus stop
        NVC_UI_CHAR_RANGE(0x1F690, 1),   // (🚐)       minibus
        NVC_UI_CHAR_RANGE(0x1F691, 3),   // (🚑..🚓)   ambulance car
        NVC_UI_CHAR_RANGE(0x1F694, 1),   // (🚔)       oncoming police car
        NVC_UI_CHAR_RANGE(0x1F695, 1),   // (🚕)       taxi
        NVC_UI_CHAR_RANGE(0x1F696, 1),   // (🚖)       oncoming taxi
        NVC_UI_CHAR_RANGE(0x1F697, 1),   // (🚗)       automobile
        NVC_UI_CHAR_RANGE(0x1F698, 1),   // (🚘)       oncoming automobile
        NVC_UI_CHAR_RANGE(0x1F699, 2),   // (🚙..🚚)   sport utility vehicle truck
        NVC_UI_CHAR_RANGE(0x1F69B, 7),   // (🚛..🚡)   articulated lorry tramway
        NVC_UI_CHAR_RANGE(0x1F6A2, 1),   // (🚢)       ship
        NVC_UI_CHAR_RANGE(0x1F6A3, 1),   // (🚣)       person rowing boat
        NVC_UI_CHAR_RANGE(0x1F6A4, 2),   // (🚤..🚥)   speedboat traffic light
        NVC_UI_CHAR_RANGE(0x1F6A6, 1),   // (🚦)       vertical traffic light
        NVC_UI_CHAR_RANGE(0x1F6A7, 7),   // (🚧..🚭)   construction smoking
        NVC_UI_CHAR_RANGE(0x1F6AE, 4),   // (🚮..🚱)   litter in bin sign-potable water
        NVC_UI_CHAR_RANGE(0x1F6B2, 1),   // (🚲)       bicycle
        NVC_UI_CHAR_RANGE(0x1F6B3, 3),   // (🚳..🚵)   no bicycles mountain biking
        NVC_UI_CHAR_RANGE(0x1F6B6, 1),   // (🚶)       person walking
        NVC_UI_CHAR_RANGE(0x1F6B7, 2),   // (🚷..🚸)   no pedestrians crossing
        NVC_UI_CHAR_RANGE(0x1F6B9, 6),   // (🚹..🚾)   men’s room closet
        NVC_UI_CHAR_RANGE(0x1F6BF, 1),   // (🚿)       shower
        NVC_UI_CHAR_RANGE(0x1F6C0, 1),   // (🛀)       person taking bath
        NVC_UI_CHAR_RANGE(0x1F6C1, 5),   // (🛁..🛅)   bathtub luggage
        NVC_UI_CHAR_RANGE(0x1F6CB, 1),   // (🛋️)       couch and lamp
        NVC_UI_CHAR_RANGE(0x1F6CC, 1),   // (🛌)       person in bed
        NVC_UI_CHAR_RANGE(0x1F6CD, 3),   // (🛍️..🛏️)   shopping bags
        NVC_UI_CHAR_RANGE(0x1F6D0, 1),   // (🛐)       place of worship
        NVC_UI_CHAR_RANGE(0x1F6D1, 2),   // (🛑..🛒)   stop sign cart
        NVC_UI_CHAR_RANGE(0x1F6D5, 1),   // (🛕)       hindu temple
        NVC_UI_CHAR_RANGE(0x1F6D6, 2),   // (🛖..🛗)   hut
        NVC_UI_CHAR_RANGE(0x1F6DC, 1),   // (🛜)       wireless
        NVC_UI_CHAR_RANGE(0x1F6DD, 3),   // (🛝..🛟)   playground slide buoy
        NVC_UI_CHAR_RANGE(0x1F6E0, 6),   // (🛠️..🛥️)   hammer and wrench boat
        NVC_UI_CHAR_RANGE(0x1F6E9, 1),   // (🛩️)       small airplane
        NVC_UI_CHAR_RANGE(0x1F6EB, 2),   // (🛫..🛬)   airplane departure arrival
        NVC_UI_CHAR_RANGE(0x1F6F0, 1),   // (🛰️)       satellite
        NVC_UI_CHAR_RANGE(0x1F6F3, 1),   // (🛳️)       passenger ship
        NVC_UI_CHAR_RANGE(0x1F6F4, 3),   // (🛴..🛶)   kick scooter
        NVC_UI_CHAR_RANGE(0x1F6F7, 2),   // (🛷..🛸)   sled saucer
        NVC_UI_CHAR_RANGE(0x1F6F9, 1),   // (🛹)       skateboard
        NVC_UI_CHAR_RANGE(0x1F6FA, 1),   // (🛺)       auto rickshaw
        NVC_UI_CHAR_RANGE(0x1F6FB, 2),   // (🛻..🛼)   pickup truck skate
        NVC_UI_CHAR_RANGE(0x1F7E0, 12),  // (🟠..🟫)   orange circle square
        NVC_UI_CHAR_RANGE(0x1F7F0, 1),   // (🟰)       heavy equals sign
        NVC_UI_CHAR_RANGE(0x1F90C, 1),   // (🤌)       pinched fingers
        NVC_UI_CHAR_RANGE(0x1F90D, 3),   // (🤍..🤏)   white heart hand
        NVC_UI_CHAR_RANGE(0x1F910, 9),   // (🤐..🤘)   zipper-mouth face of the horns
        NVC_UI_CHAR_RANGE(0x1F919, 6),   // (🤙..🤞)   call me hand fingers
        NVC_UI_CHAR_RANGE(0x1F91F, 1),   // (🤟)       love-you gesture
        NVC_UI_CHAR_RANGE(0x1F920, 8),   // (🤠..🤧)   cowboy hat face face
        NVC_UI_CHAR_RANGE(0x1F928, 8),   // (🤨..🤯)   face with raised eyebrow head
        NVC_UI_CHAR_RANGE(0x1F930, 1),   // (🤰)       pregnant woman
        NVC_UI_CHAR_RANGE(0x1F931, 2),   // (🤱..🤲)   breast-feeding up together
        NVC_UI_CHAR_RANGE(0x1F933, 8),   // (🤳..🤺)   selfie fencing
        NVC_UI_CHAR_RANGE(0x1F93C, 3),   // (🤼..🤾)   people wrestling playing handball
        NVC_UI_CHAR_RANGE(0x1F93F, 1),   // (🤿)       diving mask
        NVC_UI_CHAR_RANGE(0x1F940, 6),   // (🥀..🥅)   wilted flower net
        NVC_UI_CHAR_RANGE(0x1F947, 5),   // (🥇..🥋)   1st place medal arts uniform
        NVC_UI_CHAR_RANGE(0x1F94C, 1),   // (🥌)       curling stone
        NVC_UI_CHAR_RANGE(0x1F94D, 3),   // (🥍..🥏)   lacrosse disc
        NVC_UI_CHAR_RANGE(0x1F950, 15),  // (🥐..🥞)   croissant
        NVC_UI_CHAR_RANGE(0x1F95F, 13),  // (🥟..🥫)   dumpling food
        NVC_UI_CHAR_RANGE(0x1F96C, 5),   // (🥬..🥰)   leafy green face with hearts
        NVC_UI_CHAR_RANGE(0x1F971, 1),   // (🥱)       yawning face
        NVC_UI_CHAR_RANGE(0x1F972, 1),   // (🥲)       smiling face with tear
        NVC_UI_CHAR_RANGE(0x1F973, 4),   // (🥳..🥶)   partying face face
        NVC_UI_CHAR_RANGE(0x1F977, 2),   // (🥷..🥸)   ninja face
        NVC_UI_CHAR_RANGE(0x1F979, 1),   // (🥹)       face holding back tears
        NVC_UI_CHAR_RANGE(0x1F97A, 1),   // (🥺)       pleading face
        NVC_UI_CHAR_RANGE(0x1F97B, 1),   // (🥻)       sari
        NVC_UI_CHAR_RANGE(0x1F97C, 4),   // (🥼..🥿)   lab coat shoe
        NVC_UI_CHAR_RANGE(0x1F980, 5),   // (🦀..🦄)   crab
        NVC_UI_CHAR_RANGE(0x1F985, 13),  // (🦅..🦑)   eagle
        NVC_UI_CHAR_RANGE(0x1F992, 6),   // (🦒..🦗)   giraffe
        NVC_UI_CHAR_RANGE(0x1F998, 11),  // (🦘..🦢)   kangaroo
        NVC_UI_CHAR_RANGE(0x1F9A3, 2),   // (🦣..🦤)   mammoth
        NVC_UI_CHAR_RANGE(0x1F9A5, 6),   // (🦥..🦪)   sloth
        NVC_UI_CHAR_RANGE(0x1F9AB, 3),   // (🦫..🦭)   beaver
        NVC_UI_CHAR_RANGE(0x1F9AE, 2),   // (🦮..🦯)   guide dog cane
        NVC_UI_CHAR_RANGE(0x1F9B0, 10),  // (🦰..🦹)   red hair
        NVC_UI_CHAR_RANGE(0x1F9BA, 6),   // (🦺..🦿)   safety vest leg
        NVC_UI_CHAR_RANGE(0x1F9C0, 1),   // (🧀)       cheese wedge
        NVC_UI_CHAR_RANGE(0x1F9C1, 2),   // (🧁..🧂)   cupcake
        NVC_UI_CHAR_RANGE(0x1F9C3, 8),   // (🧃..🧊)   beverage box
        NVC_UI_CHAR_RANGE(0x1F9CB, 1),   // (🧋)       bubble tea
        NVC_UI_CHAR_RANGE(0x1F9CC, 1),   // (🧌)       troll
        NVC_UI_CHAR_RANGE(0x1F9CD, 3),   // (🧍..🧏)   person standing person
        NVC_UI_CHAR_RANGE(0x1F9D0, 23),  // (🧐..🧦)   face with monocle
        NVC_UI_CHAR_RANGE(0x1F9E7, 25),  // (🧧..🧿)   red envelope amulet
        NVC_UI_CHAR_RANGE(0x1FA70, 4),   // (🩰..🩳)   ballet shoes
        NVC_UI_CHAR_RANGE(0x1FA74, 1),   // (🩴)       thong sandal
        NVC_UI_CHAR_RANGE(0x1FA75, 3),   // (🩵..🩷)   light blue heart heart
        NVC_UI_CHAR_RANGE(0x1FA78, 3),   // (🩸..🩺)   drop of blood
        NVC_UI_CHAR_RANGE(0x1FA7B, 2),   // (🩻..🩼)   x-ray
        NVC_UI_CHAR_RANGE(0x1FA80, 3),   // (🪀..🪂)   yo-yo
        NVC_UI_CHAR_RANGE(0x1FA83, 4),   // (🪃..🪆)   boomerang dolls
        NVC_UI_CHAR_RANGE(0x1FA87, 2),   // (🪇..🪈)   maracas
        NVC_UI_CHAR_RANGE(0x1FA90, 6),   // (🪐..🪕)   ringed planet
        NVC_UI_CHAR_RANGE(0x1FA96, 19),  // (🪖..🪨)   military helmet
        NVC_UI_CHAR_RANGE(0x1FAA9, 4),   // (🪩..🪬)   mirror ball
        NVC_UI_CHAR_RANGE(0x1FAAD, 3),   // (🪭..🪯)   folding hand fan
        NVC_UI_CHAR_RANGE(0x1FAB0, 7),   // (🪰..🪶)   fly
        NVC_UI_CHAR_RANGE(0x1FAB7, 4),   // (🪷..🪺)   lotus with eggs
        NVC_UI_CHAR_RANGE(0x1FABB, 3),   // (🪻..🪽)   hyacinth
        NVC_UI_CHAR_RANGE(0x1FABF, 1),   // (🪿)       goose
        NVC_UI_CHAR_RANGE(0x1FAC0, 3),   // (🫀..🫂)   anatomical heart hugging
        NVC_UI_CHAR_RANGE(0x1FAC3, 3),   // (🫃..🫅)   pregnant man with crown
        NVC_UI_CHAR_RANGE(0x1FACE, 2),   // (🫎..🫏)   moose
        NVC_UI_CHAR_RANGE(0x1FAD0, 7),   // (🫐..🫖)   blueberries
        NVC_UI_CHAR_RANGE(0x1FAD7, 3),   // (🫗..🫙)   pouring liquid
        NVC_UI_CHAR_RANGE(0x1FADA, 2),   // (🫚..🫛)   ginger root pod
        NVC_UI_CHAR_RANGE(0x1FAE0, 8),   // (🫠..🫧)   melting face
        NVC_UI_CHAR_RANGE(0x1FAE8, 1),   // (🫨)       shaking face
        NVC_UI_CHAR_RANGE(0x1FAF0, 7),   // (🫰..🫶)   hand with index finger and thumb crossed hands
        NVC_UI_CHAR_RANGE(0x1FAF7, 2),   // (🫷..🫸)   leftwards pushing hand pushing hand
    };
    m_emojiSet = CFCharacterSetCreateMutable(kCFAllocatorDefault);
    if (likely(m_emojiSet != nullptr)) {
        for (int i = 0; i < countof(emojis); i++) {
            CFCharacterSetAddCharactersInRange(m_emojiSet, emojis[i]);
        }
        bool x = is_emoji(0x2194);
        x = false;
        
    }
}

}

