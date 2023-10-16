//
//  nvc_ui.cpp
//  NVim
//
//  Created by wizjin on 2023/9/20.
//

#include "nvc_ui.h"
#include <mutex>
#include <map>
#include "nvc_ui_context.h"

#define kNvcUiKeysMax                       64

extern "C" void NSBeep(void);

#pragma mark - NVC RPC Helper
#define nvc_rpc_call_command_const_begin(_ctx, _cmd, _args)   \
    nvc_rpc_call_command_begin(_ctx, _cmd, sizeof(_cmd) - 1, _args)

static const std::string nvc_rpc_empty_str = "";
static inline const std::string nvc_rpc_read_str(nvc_rpc_context_t *ctx) {
    uint32_t len = 0;
    const auto& str = nvc_rpc_read_str(ctx, &len);
    return likely(str != nullptr) ? std::string(str, len) : nvc_rpc_empty_str;
}

static inline void nvc_rpc_write_string(nvc_rpc_context_t *ctx, const std::string& str) {
    nvc_rpc_write_str(ctx, str.c_str(), (uint32_t)str.size());
}

static inline void nvc_rpc_call_command_begin(nvc_rpc_context_t *ctx, const char *cmd, uint32_t cmdlen, uint32_t args) {
    nvc_rpc_call_const_begin(ctx, "nvim_cmd", 2);
    nvc_rpc_write_map_size(ctx, 2);
    nvc_rpc_write_const_str(ctx, "cmd");
    nvc_rpc_write_str(ctx, cmd, cmdlen);
    nvc_rpc_write_const_str(ctx, "args");
    nvc_rpc_write_array_size(ctx, args);
}

static inline void nvc_rpc_call_command_end(nvc_rpc_context_t *ctx) {
    nvc_rpc_write_map_size(ctx, 0);
    nvc_rpc_call_end(ctx);
}

#pragma mark - NVC UI Struct
typedef int (*nvc_ui_action_t)(nvc::UIContext *ctx, int narg);

#undef NVC_UI_COLOR_CODE
#define NVC_UI_COLOR_CODE(_code)            { #_code, nvc::ui_color_code_##_code }
static const std::map<const std::string, nvc::UIColorCode> nvc_ui_color_name_table = { NVC_UI_COLOR_CODE_LIST };

typedef void (*nvc_ui_set_mode_info)(nvc::UIModeInfo &info, nvc_rpc_context_t *ctx);

static inline void nvc_ui_set_mode_info_null(nvc::UIModeInfo &info, nvc_rpc_context_t *ctx) {
    nvc_rpc_read_skip_items(ctx, 1);
}

#define NVC_UI_SET_MODE_INFO_STR(_action)   { #_action, [](nvc::UIModeInfo &info, nvc_rpc_context_t *ctx) { info._action = nvc_rpc_read_str(ctx); } }
#define NVC_UI_SET_MODE_INFO_INT(_action)   { #_action, [](nvc::UIModeInfo &info, nvc_rpc_context_t *ctx) { info._action = nvc_rpc_read_int(ctx); } }
#define NVC_UI_SET_MODE_INFO_NULL(_action)  { #_action, nvc_ui_set_mode_info_null }
static const std::map<const std::string, nvc_ui_set_mode_info> nvc_ui_set_mode_info_actions = {
    NVC_UI_SET_MODE_INFO_STR(name),
    NVC_UI_SET_MODE_INFO_STR(short_name),
    NVC_UI_SET_MODE_INFO_STR(cursor_shape),
    NVC_UI_SET_MODE_INFO_STR(mouse_shape),
    NVC_UI_SET_MODE_INFO_INT(cell_percentage),
    NVC_UI_SET_MODE_INFO_INT(blinkwait),
    NVC_UI_SET_MODE_INFO_INT(blinkon),
    NVC_UI_SET_MODE_INFO_INT(blinkoff),
    NVC_UI_SET_MODE_INFO_NULL(hl_id),
    NVC_UI_SET_MODE_INFO_NULL(id_lm),
    NVC_UI_SET_MODE_INFO_INT(attr_id),
    NVC_UI_SET_MODE_INFO_INT(attr_id_lm),
};

typedef uint8_t nvc_ui_font_index_t;

typedef struct nvc_ui_glyph_info {
    CGGlyph             glyph;
    nvc_ui_font_index_t font_index;
} nvc_ui_glyph_info_t;

typedef struct nvc_ui_tab {
    nvc_rpc_object_handler_t    tab;
    std::string                 name;
    
    inline bool operator==(const struct nvc_ui_tab& rhl) const {
        return this->tab == rhl.tab && this->name == rhl.name;
    }
} nvc_ui_tab_t;

typedef void (*nvc_ui_set_ui_tab)(nvc_ui_tab_t &tab, nvc_rpc_context_t *ctx);

#define NVC_UI_SET_UI_TAB_EXT(_action)      { #_action, [](nvc_ui_tab_t &tab, nvc_rpc_context_t *ctx) { tab._action = nvc_rpc_read_ext_handle(ctx); } }
#define NVC_UI_SET_UI_TAB_STR(_action)      { #_action, [](nvc_ui_tab_t &tab, nvc_rpc_context_t *ctx) { tab._action = nvc_rpc_read_str(ctx); } }
static const std::map<const std::string, nvc_ui_set_ui_tab> nvc_ui_set_ui_tab_actions = {
    NVC_UI_SET_UI_TAB_EXT(tab),
    NVC_UI_SET_UI_TAB_STR(name),
};

static int nvc_ui_response_handler(nvc_rpc_context_t *ctx, int items);
static int nvc_ui_notification_handler(nvc_rpc_context_t *ctx, int items);
static int nvc_ui_close_handler(nvc_rpc_context_t *ctx, int items);

#pragma mark - NVC Util Helper
static inline uint32_t nvc_ui_key_flags_encode(nvc_ui_key_flags_t flags, char output[], uint32_t len, bool short_code) {
    if (flags.shift) {
        output[len++] = 'S';
        if (!short_code) output[len++] = '-';
    }
    if (flags.control) {
        output[len++] = 'C';
        if (!short_code) output[len++] = '-';
    }
    if (flags.option) {
        output[len++] = 'M';
        if (!short_code) output[len++] = '-';
    }
    if (flags.command) {
        output[len++] = 'D';
        if (!short_code) output[len++] = '-';
    }
    return len;
}

#pragma mark - NVC UI API
nvc_ui_context_t *nvc_ui_create(int inskt, int outskt, const nvc_ui_config_t *config, const nvc_ui_callback_t *callback, void *userdata) {
    nvc::UIContext *ctx = nullptr;
    if (inskt != INVALID_SOCKET && outskt != INVALID_SOCKET && config != NULL && callback != NULL) {
        ctx = new nvc::UIContext(*callback, config->font_size, userdata);
        if (unlikely(ctx == nullptr)) {
            NVLogE("nvc ui create context failed");
        } else {
            int res = nvc_rpc_init(ctx->rpc(), inskt, outskt, ctx, nvc_ui_response_handler, nvc_ui_notification_handler, nvc_ui_close_handler);
            if (res != NVC_RC_OK) {
                nvc_ui_destroy(ctx);
                NVLogE("nvc ui init context failed");
                free(ctx);
                ctx = nullptr;
            }
        }
    }
    return ctx;
}

void nvc_ui_destroy(nvc_ui_context_t *ptr) {
    nvc::UIContext *ctx = static_cast<nvc::UIContext *>(ptr);
    if (ctx != nullptr) {
        nvc_rpc_final(ctx->rpc());
        for (const auto& p : ctx->grids()) {
            delete p.second;
        }
        ctx->grids().clear();
        delete ctx;
    }
}

bool nvc_ui_is_attached(nvc_ui_context_t *ptr) {
    bool res = false;
    nvc::UIContext *ctx = static_cast<nvc::UIContext *>(ptr);
    if (likely(ctx != nullptr)) {
        res = ctx->attached();
    }
    return res;
}

CGSize nvc_ui_attach(nvc_ui_context_t *ptr, CGSize size) {
    nvc::UIContext *ctx = static_cast<nvc::UIContext *>(ptr);
    if (likely(ctx != nullptr) && ctx->attach(size)) {
        const auto& wndSize = ctx->window_size();
        nvc_rpc_call_const_begin(ctx->rpc(), "nvim_ui_attach", 3);
        nvc_rpc_write_unsigned(ctx->rpc(), wndSize.width);
        nvc_rpc_write_unsigned(ctx->rpc(), wndSize.height);
        nvc_rpc_write_map_size(ctx->rpc(), 3);
        nvc_rpc_write_const_str(ctx->rpc(), "override");
        nvc_rpc_write_false(ctx->rpc());
        nvc_rpc_write_const_str(ctx->rpc(), "ext_linegrid");
        nvc_rpc_write_true(ctx->rpc());
        nvc_rpc_write_const_str(ctx->rpc(), "ext_tabline");
        nvc_rpc_write_false(ctx->rpc());
        nvc_rpc_call_end(ctx->rpc());
        size = ctx->cell2size(wndSize);
    }
    return size;
}

void nvc_ui_detach(nvc_ui_context_t *ptr) {
    nvc::UIContext *ctx = static_cast<nvc::UIContext *>(ptr);
    if (likely(ctx != nullptr) && ctx->detach()) {
        nvc_rpc_call_const_begin(ctx->rpc(), "nvim_ui_detach", 0);
        nvc_rpc_call_end(ctx->rpc());
    }
}

void nvc_ui_redraw(nvc_ui_context_t *ptr, CGContextRef context) {
    nvc::UIContext *ctx = static_cast<nvc::UIContext *>(ptr);
    if (likely(ctx != nullptr && context != nullptr)) {
        ctx->draw(context);
    }
}

CGSize nvc_ui_resize(nvc_ui_context_t *ptr, CGSize size) {
    nvc::UIContext *ctx = static_cast<nvc::UIContext *>(ptr);
    if (likely(ctx != nullptr) && ctx->update_size(size)) {
        const auto& wndSize = ctx->window_size();
        nvc_rpc_call_const_begin(ctx->rpc(), "nvim_ui_try_resize", 2);
        nvc_rpc_write_unsigned(ctx->rpc(), wndSize.width);
        nvc_rpc_write_unsigned(ctx->rpc(), wndSize.height);
        nvc_rpc_call_end(ctx->rpc());
        size = ctx->cell2size(wndSize);
    }
    return size;
}

CGFloat nvc_ui_get_line_height(nvc_ui_context_t *ptr) {
    nvc::UIContext *ctx = static_cast<nvc::UIContext *>(ptr);
    CGFloat height = 0;
    if (likely(ctx != nullptr && ctx->attached())) {
        height = ctx->cell_size().height;
    }
    return height;
}

CGPoint nvc_ui_get_cursor_position(nvc_ui_context_t *ptr) {
    nvc::UIContext *ctx = static_cast<nvc::UIContext *>(ptr);
    CGPoint pt = CGPointZero;
    if (likely(ctx != nullptr)) {
        pt = ctx->find_cursor();
    }
    return pt;
}

void nvc_ui_open_file(nvc_ui_context_t *ptr, const char *file, uint32_t len, bool new_tab) {
    nvc::UIContext *ctx = static_cast<nvc::UIContext *>(ptr);
    if (likely(ctx != nullptr && ctx->attached() && len > 0)) {
        nvc_rpc_call_command_const_begin(ctx->rpc(), (new_tab ? "tabnew" : "edit"), 1);
        nvc_rpc_write_str(ctx->rpc(), file, len);
        nvc_rpc_call_command_end(ctx->rpc());
    }
}

void nvc_ui_tab_next(nvc_ui_context_t *ptr, int count) {
    nvc::UIContext *ctx = static_cast<nvc::UIContext *>(ptr);
    if (likely(ctx != nullptr && ctx->attached())) {
        nvc_rpc_call_command_const_begin(ctx->rpc(), "tabnext", 1);
        nvc_rpc_write_signed(ctx->rpc(), count);
        nvc_rpc_call_command_end(ctx->rpc());
    }
}

// Note: https://neovim.io/doc/user/intro.html#keycodes
// <HIToolbox/Events.h>
#define NVC_UI_KEYCODE(_code, _notation)    { _code, _notation }
static const std::map<uint16_t, const std::string> nvc_ui_keycode_table = {
    NVC_UI_KEYCODE(0x24, "CR"),         // kVK_Return
    NVC_UI_KEYCODE(0x30, "Tab"),        // kVK_Tab
    NVC_UI_KEYCODE(0x31, "Space"),      // kVK_Space
    NVC_UI_KEYCODE(0x33, "BS"),         // kVK_Delete
    NVC_UI_KEYCODE(0x35, "Esc"),        // kVK_Escape
    NVC_UI_KEYCODE(0x40, "F17"),        // kVK_F17
    NVC_UI_KEYCODE(0x41, "kPoint"),     // kVK_ANSI_KeypadDecimal
    NVC_UI_KEYCODE(0x43, "kMultiply"),  // kVK_ANSI_KeypadMultiply
    NVC_UI_KEYCODE(0x45, "kPlus"),      // kVK_ANSI_KeypadPlus
    NVC_UI_KEYCODE(0x47, "kDel"),       // kVK_ANSI_KeypadClear
    NVC_UI_KEYCODE(0x4B, "kDivide"),    // kVK_ANSI_KeypadDivide
    NVC_UI_KEYCODE(0x4C, "kEnter"),     // kVK_ANSI_KeypadEnter
    NVC_UI_KEYCODE(0x4E, "kMinus"),     // kVK_ANSI_KeypadMinus
    NVC_UI_KEYCODE(0x4F, "F18"),        // kVK_F18
    NVC_UI_KEYCODE(0x50, "F19"),        // kVK_F19
    NVC_UI_KEYCODE(0x51, "kEqual"),     // kVK_ANSI_KeypadEquals
    NVC_UI_KEYCODE(0x52, "k0"),         // kVK_ANSI_Keypad0
    NVC_UI_KEYCODE(0x53, "k1"),         // kVK_ANSI_Keypad1
    NVC_UI_KEYCODE(0x54, "k2"),         // kVK_ANSI_Keypad2
    NVC_UI_KEYCODE(0x55, "k3"),         // kVK_ANSI_Keypad3
    NVC_UI_KEYCODE(0x56, "k4"),         // kVK_ANSI_Keypad4
    NVC_UI_KEYCODE(0x57, "k5"),         // kVK_ANSI_Keypad5
    NVC_UI_KEYCODE(0x58, "k6"),         // kVK_ANSI_Keypad6
    NVC_UI_KEYCODE(0x59, "k7"),         // kVK_ANSI_Keypad7
    NVC_UI_KEYCODE(0x5A, "F20"),        // kVK_F20
    NVC_UI_KEYCODE(0x5B, "k8"),         // kVK_ANSI_Keypad8
    NVC_UI_KEYCODE(0x5C, "k9"),         // kVK_ANSI_Keypad9
    NVC_UI_KEYCODE(0x60, "F5"),         // kVK_F5
    NVC_UI_KEYCODE(0x61, "F6"),         // kVK_F6
    NVC_UI_KEYCODE(0x62, "F7"),         // kVK_F7
    NVC_UI_KEYCODE(0x63, "F3"),         // kVK_F3
    NVC_UI_KEYCODE(0x64, "F8"),         // kVK_F8
    NVC_UI_KEYCODE(0x65, "F9"),         // kVK_F9
    NVC_UI_KEYCODE(0x67, "F11"),        // kVK_F11
    NVC_UI_KEYCODE(0x69, "F13"),        // kVK_F13
    NVC_UI_KEYCODE(0x6A, "F16"),        // kVK_F16
    NVC_UI_KEYCODE(0x6B, "F14"),        // kVK_F14
    NVC_UI_KEYCODE(0x6D, "F10"),        // kVK_F10
    NVC_UI_KEYCODE(0x6F, "F12"),        // kVK_F12
    NVC_UI_KEYCODE(0x71, "F15"),        // kVK_F15
    NVC_UI_KEYCODE(0x73, "Home"),       // kVK_Home
    NVC_UI_KEYCODE(0x74, "PageUp"),     // kVK_PageUp
    NVC_UI_KEYCODE(0x75, "Del"),        // kVK_ForwardDelete
    NVC_UI_KEYCODE(0x76, "F4"),         // kVK_F4
    NVC_UI_KEYCODE(0x77, "End"),        // kVK_End
    NVC_UI_KEYCODE(0x78, "F2"),         // kVK_F2
    NVC_UI_KEYCODE(0x79, "PageDown"),   // kVK_PageDown
    NVC_UI_KEYCODE(0x7A, "F1"),         // kVK_F1
    NVC_UI_KEYCODE(0x7B, "Left"),       // kVK_LeftArrow
    NVC_UI_KEYCODE(0x7C, "Right"),      // kVK_RightArrow
    NVC_UI_KEYCODE(0x7D, "Down"),       // kVK_DownArrow
    NVC_UI_KEYCODE(0x7E, "Up"),         // kVK_UpArrow
};

bool nvc_ui_input_key(nvc_ui_context_t *ctx, uint16_t key, nvc_ui_key_flags_t flags) {
    bool res = false;
    const auto& p = nvc_ui_keycode_table.find(key);
    if (p != nvc_ui_keycode_table.end()) {
        const auto& notation = p->second;
        char keys[kNvcUiKeysMax];
        uint32_t len = 0;
        keys[len++] = '<';
        len = nvc_ui_key_flags_encode(flags, keys, len, false);
        memcpy(keys + len, notation.c_str(), notation.size());
        len += notation.size();
        keys[len++] = '>';
        nvc_ui_input_rawkey(ctx, keys, len);
        res = true;
    }
    return res;
}

// Note: https://developer.apple.com/documentation/appkit/1535851-function-key_unicode_values?language=objc
// <AppKit/NSEvent.h>
#define NVC_UI_FUNCKEY(_code, _notation)    { _code, _notation }
static const std::map<UniChar, const std::string> nvc_ui_funckey_table = {
    NVC_UI_FUNCKEY(0xF700, "Up"),       // NSUpArrowFunctionKey
    NVC_UI_FUNCKEY(0xF701, "Down"),     // NSDownArrowFunctionKey
    NVC_UI_FUNCKEY(0xF702, "Left"),     // NSLeftArrowFunctionKey
    NVC_UI_FUNCKEY(0xF703, "Right"),    // NSRightArrowFunctionKey
    NVC_UI_FUNCKEY(0xF704, "F1"),       // NSF1FunctionKey
    NVC_UI_FUNCKEY(0xF705, "F2"),       // NSF2FunctionKey
    NVC_UI_FUNCKEY(0xF706, "F3"),       // NSF3FunctionKey
    NVC_UI_FUNCKEY(0xF707, "F4"),       // NSF4FunctionKey
    NVC_UI_FUNCKEY(0xF708, "F5"),       // NSF5FunctionKey
    NVC_UI_FUNCKEY(0xF709, "F6"),       // NSF6FunctionKey
    NVC_UI_FUNCKEY(0xF70A, "F7"),       // NSF7FunctionKey
    NVC_UI_FUNCKEY(0xF70B, "F8"),       // NSF8FunctionKey
    NVC_UI_FUNCKEY(0xF70C, "F9"),       // NSF9FunctionKey
    NVC_UI_FUNCKEY(0xF70D, "F10"),      // NSF10FunctionKey
    NVC_UI_FUNCKEY(0xF70E, "F11"),      // NSF11FunctionKey
    NVC_UI_FUNCKEY(0xF70F, "F12"),      // NSF12FunctionKey
    NVC_UI_FUNCKEY(0xF710, "F13"),      // NSF13FunctionKey
    NVC_UI_FUNCKEY(0xF711, "F14"),      // NSF14FunctionKey
    NVC_UI_FUNCKEY(0xF712, "F15"),      // NSF15FunctionKey
    NVC_UI_FUNCKEY(0xF713, "F16"),      // NSF16FunctionKey
    NVC_UI_FUNCKEY(0xF714, "F17"),      // NSF17FunctionKey
    NVC_UI_FUNCKEY(0xF715, "F18"),      // NSF18FunctionKey
    NVC_UI_FUNCKEY(0xF716, "F19"),      // NSF19FunctionKey
    NVC_UI_FUNCKEY(0xF717, "F20"),      // NSF20FunctionKey
    NVC_UI_FUNCKEY(0xF718, "F21"),      // NSF21FunctionKey
    NVC_UI_FUNCKEY(0xF719, "F22"),      // NSF22FunctionKey
    NVC_UI_FUNCKEY(0xF71A, "F23"),      // NSF23FunctionKey
    NVC_UI_FUNCKEY(0xF71B, "F24"),      // NSF24FunctionKey
    NVC_UI_FUNCKEY(0xF71C, "F25"),      // NSF25FunctionKey
    NVC_UI_FUNCKEY(0xF71D, "F26"),      // NSF26FunctionKey
    NVC_UI_FUNCKEY(0xF71E, "F27"),      // NSF27FunctionKey
    NVC_UI_FUNCKEY(0xF71F, "F28"),      // NSF28FunctionKey
    NVC_UI_FUNCKEY(0xF720, "F29"),      // NSF29FunctionKey
    NVC_UI_FUNCKEY(0xF721, "F30"),      // NSF30FunctionKey
    NVC_UI_FUNCKEY(0xF722, "F31"),      // NSF31FunctionKey
    NVC_UI_FUNCKEY(0xF723, "F32"),      // NSF32FunctionKey
    NVC_UI_FUNCKEY(0xF724, "F33"),      // NSF33FunctionKey
    NVC_UI_FUNCKEY(0xF725, "F34"),      // NSF34FunctionKey
    NVC_UI_FUNCKEY(0xF726, "F35"),      // NSF35FunctionKey
    NVC_UI_FUNCKEY(0xF727, "Insert"),   // NSInsertFunctionKey
    NVC_UI_FUNCKEY(0xF728, "Del"),      // NSDeleteFunctionKey
    NVC_UI_FUNCKEY(0xF729, "Home"),     // NSHomeFunctionKey
    NVC_UI_FUNCKEY(0xF72A, "Begin"),    // NSBeginFunctionKey
    NVC_UI_FUNCKEY(0xF72B, "End"),      // NSEndFunctionKey
    NVC_UI_FUNCKEY(0xF72C, "PageUp"),   // NSPageUpFunctionKey
    NVC_UI_FUNCKEY(0xF72D, "PageDown"), // NSPageDownFunctionKey
    // 0xF72E NSPrintScreenFunctionKey
    // 0xF72F NSScrollLockFunctionKey
    // 0xF730 NSPauseFunctionKey
    // 0xF731 NSSysReqFunctionKey
    // 0xF732 NSBreakFunctionKey
    // 0xF733 NSResetFunctionKey
    // 0xF734 NSStopFunctionKey
    // 0xF735 NSMenuFunctionKey
    // 0xF736 NSUserFunctionKey
    // 0xF737 NSSystemFunctionKey
    // 0xF738 NSPrintFunctionKey
    // 0xF739 NSClearLineFunctionKey
    // 0xF73A NSClearDisplayFunctionKey
    // 0xF73B NSInsertLineFunctionKey
    // 0xF73C NSDeleteLineFunctionKey
    // 0xF73D NSInsertCharFunctionKey
    // 0xF73E NSDeleteCharFunctionKey
    // 0xF73F NSPrevFunctionKey
    // 0xF740 NSNextFunctionKey
    // 0xF741 NSSelectFunctionKey
    // 0xF742 NSExecuteFunctionKey
    // 0xF743 NSUndoFunctionKey
    // 0xF744 NSRedoFunctionKey
    // 0xF745 NSFindFunctionKey
    NVC_UI_FUNCKEY(0xF746, "Help"),     // NSPageDownFunctionKey
    // 0xF747 NSModeSwitchFunctionKey
};

bool nvc_ui_input_function_key(nvc_ui_context_t *ctx, UniChar ch, nvc_ui_key_flags_t flags) {
    bool res = false;
    const auto& p = nvc_ui_funckey_table.find(ch);
    if (p != nvc_ui_funckey_table.end()) {
        const auto& notation = p->second;
        char keys[kNvcUiKeysMax];
        uint32_t len = 0;
        keys[len++] = '<';
        len = nvc_ui_key_flags_encode(flags, keys, len, false);
        memcpy(keys + len, notation.c_str(), notation.size());
        len += notation.size();
        keys[len++] = '>';
        nvc_ui_input_rawkey(ctx, keys, len);
        res = true;
    }
    return res;
}

void nvc_ui_input_keystr(nvc_ui_context_t *ctx, nvc_ui_key_flags_t flags, const char* str, uint32_t slen) {
    std::string value;
    for (int i = 0; i < slen; i++) {
        if (str[i] == '<') {
            value += "<lt>";
        } else {
            value.push_back(str[i]);
        }
    }
    if (!value.empty()) {
        nvc_ui_input_rawkey(ctx, value.c_str(), (uint32_t)value.size());
    }
}

void nvc_ui_input_rawkey(nvc_ui_context_t *ptr, const char* keys, uint32_t len) {
    nvc::UIContext *ctx = static_cast<nvc::UIContext *>(ptr);
    if (likely(ctx != nullptr && ctx->attached())) {
        nvc_rpc_call_const_begin(ctx->rpc(), "nvim_input", 1);
        nvc_rpc_write_str(ctx->rpc(), keys, len);
        nvc_rpc_call_end(ctx->rpc());
    }
}

#undef NVC_UI_MOUSE_KEY
#define NVC_UI_MOUSE_KEY(_key)      #_key,
static const std::string nvc_ui_mouse_key_name[]  = { NVC_UI_MOUSE_KEY_LIST };

#undef NVC_UI_MOUSE_ACTION
#define NVC_UI_MOUSE_ACTION(_key)   #_key,
static const std::string nvc_ui_mouse_action_name[]  = { NVC_UI_MOUSE_ACTION_LIST };

void nvc_ui_input_mouse(nvc_ui_context_t *ptr, nvc_ui_mouse_info_t mouse) {
    nvc::UIContext *ctx = static_cast<nvc::UIContext *>(ptr);
    if (likely(ctx != nullptr && ctx->attached())) {
        nvc_rpc_call_const_begin(ctx->rpc(), "nvim_input_mouse", 6);
        nvc_rpc_write_string(ctx->rpc(), nvc_ui_mouse_key_name[mouse.key]);
        nvc_rpc_write_string(ctx->rpc(), nvc_ui_mouse_action_name[mouse.action]);
        uint32_t len = 0;
        char modifier[kNvcUiKeysMax];
        len = nvc_ui_key_flags_encode(mouse.flags, modifier, len, true);
        nvc_rpc_write_str(ctx->rpc(), modifier, len);
        nvc_rpc_write_unsigned(ctx->rpc(), 0);
        nvc_rpc_write_signed(ctx->rpc(), mouse.point.y/ctx->cell_size().height);
        nvc_rpc_write_signed(ctx->rpc(), mouse.point.x/ctx->cell_size().width);
        nvc_rpc_call_end(ctx->rpc());
    }
}

#pragma mark - NVC UI Option Actions
static inline int nvc_ui_option_set_action_emoji(nvc::UIContext *ctx, int items) {
    if (likely(items-- > 0)) {
        ctx->font().emoji(nvc_rpc_read_bool(ctx->rpc()));
    }
    return items;
}

static inline int nvc_ui_option_set_action_guifont(nvc::UIContext *ctx, int items) {
    if (likely(items-- > 0)) {
        if (ctx->font().load(nvc_rpc_read_str(ctx->rpc()))) {
            ctx->cb().font_updated(ctx->userdata());
        }
    }
    return items;
}

static inline int nvc_ui_option_set_action_guifontwide(nvc::UIContext *ctx, int items) {
    if (likely(items-- > 0)) {
        if (ctx->font().load_wide(nvc_rpc_read_str(ctx->rpc()))) {
            ctx->cb().font_updated(ctx->userdata());
        }
    }
    return items;
}

static inline int nvc_ui_option_set_action_mousehide(nvc::UIContext *ctx, int items) {
    if (likely(items-- > 0)) {
        ctx->cb().enable_mouse_autohide(ctx->userdata(), nvc_rpc_read_bool(ctx->rpc()));
    }
    return items;
}

static inline int nvc_ui_option_set_action_mousemoveevent(nvc::UIContext *ctx, int items) {
    if (likely(items-- > 0)) {
        ctx->cb().enable_mouse_move(ctx->userdata(), nvc_rpc_read_bool(ctx->rpc()));
    }
    return items;
}

static inline int nvc_ui_option_set_action_ext_tabline(nvc::UIContext *ctx, int items) {
    if (likely(items-- > 0)) {
        ctx->cb().enable_ext_tabline(ctx->userdata(), nvc_rpc_read_bool(ctx->rpc()));
    }
    return items;
}

#pragma mark - NVC UI Redraw Actions
static inline int nvc_ui_redraw_action_set_title(nvc::UIContext *ctx, int count) {
    if (likely(count-- > 0)) {
        int items = nvc_rpc_read_array_size(ctx->rpc());
        if (likely(items-- > 0)) {
            uint32_t len = 0;
            ctx->cb().update_title(ctx->userdata(), nvc_rpc_read_str(ctx->rpc(), &len), len);
        }
        nvc_rpc_read_skip_items(ctx->rpc(), items);
    }
    return count;
}

static inline int nvc_ui_redraw_action_mode_info_set(nvc::UIContext *ctx, int items) {
    if (likely(items-- > 0)) {
        int narg = nvc_rpc_read_array_size(ctx->rpc());
        if (likely(narg >= 2)) {
            narg -= 2;
            auto& mode = ctx->mode();
            bool enabled = nvc_rpc_read_bool(ctx->rpc());
            nvc::UIModeInfoList infos;
            for (int n = nvc_rpc_read_array_size(ctx->rpc()); n > 0; n--) {
                nvc::UIModeInfo info;
                for (int mn = nvc_rpc_read_map_size(ctx->rpc()); mn > 0; mn--) {
                    const auto& name = nvc_rpc_read_str(ctx->rpc());
                    const auto& p = nvc_ui_set_mode_info_actions.find(name);
                    if (likely(p != nvc_ui_set_mode_info_actions.end())) {
                        p->second(info, ctx->rpc());
                    } else {
                        nvc_rpc_read_skip_items(ctx->rpc(), 1);
                        NVLogW("nvc ui unknown mode info: %s", name.c_str());
                    }
                }
                infos.push_back(info);
            }
            mode.infos(infos);
            mode.enabled(enabled);
        }
        nvc_rpc_read_skip_items(ctx->rpc(), narg);
    }
    return items;
}

typedef int (*nvc_ui_option_set_action)(nvc::UIContext *ctx, int items);
#define NVC_UI_OPTION_SET_ACTION(_action)       { #_action, nvc_ui_option_set_action_##_action }
#define NVC_UI_OPTION_SET_ACTION_NULL(_action)  { #_action, nullptr }
static const std::map<const std::string, nvc_ui_option_set_action> nvc_ui_option_set_actions = {
    NVC_UI_OPTION_SET_ACTION_NULL(arabicshape),
    NVC_UI_OPTION_SET_ACTION_NULL(ambiwidth),
    NVC_UI_OPTION_SET_ACTION(emoji),
    NVC_UI_OPTION_SET_ACTION(guifont),
    NVC_UI_OPTION_SET_ACTION(guifontwide),
    NVC_UI_OPTION_SET_ACTION_NULL(linespace),
    NVC_UI_OPTION_SET_ACTION_NULL(mousefocus),
    NVC_UI_OPTION_SET_ACTION(mousehide),
    NVC_UI_OPTION_SET_ACTION(mousemoveevent),
    NVC_UI_OPTION_SET_ACTION_NULL(pumblend),
    NVC_UI_OPTION_SET_ACTION_NULL(showtabline),
    NVC_UI_OPTION_SET_ACTION_NULL(termguicolors),
    NVC_UI_OPTION_SET_ACTION_NULL(ttimeout),
    NVC_UI_OPTION_SET_ACTION_NULL(ttimeoutlen),
    NVC_UI_OPTION_SET_ACTION_NULL(verbose),
    NVC_UI_OPTION_SET_ACTION_NULL(ext_linegrid),
    NVC_UI_OPTION_SET_ACTION_NULL(ext_multigrid),
    NVC_UI_OPTION_SET_ACTION_NULL(ext_hlstate),
    NVC_UI_OPTION_SET_ACTION_NULL(ext_termcolors),
    NVC_UI_OPTION_SET_ACTION_NULL(ext_cmdline),
    NVC_UI_OPTION_SET_ACTION_NULL(ext_popupmenu),
    NVC_UI_OPTION_SET_ACTION(ext_tabline),
    NVC_UI_OPTION_SET_ACTION_NULL(ext_wildmenu),
    NVC_UI_OPTION_SET_ACTION_NULL(ext_messages),
};

static inline int nvc_ui_redraw_action_option_set(nvc::UIContext *ctx, int items) {
    while (items-- > 0) {
        int narg = nvc_rpc_read_array_size(ctx->rpc());
        if (likely(narg-- > 0)) {
            const auto& key = nvc_rpc_read_str(ctx->rpc());
            if (likely(narg > 0)) {
                const auto& p = nvc_ui_option_set_actions.find(key);
                if (unlikely(p == nvc_ui_option_set_actions.end())) {
                    NVLogW("nvc ui find unknown option value type: %s", key.c_str());
                } else if (p->second != nullptr) {
                    narg = p->second(ctx, narg);
                }
            }
            nvc_rpc_read_skip_items(ctx->rpc(), narg);
        }
    }
    return 0;
}

static inline int nvc_ui_redraw_action_mode_change(nvc::UIContext *ctx, int items) {
    if (likely(items-- > 0)) {
        int narg = nvc_rpc_read_array_size(ctx->rpc());
        if (likely(narg >= 2)) {
            narg -= 2;
            ctx->change_mode(nvc_rpc_read_str(ctx->rpc()), nvc_rpc_read_int(ctx->rpc()));
        }
        nvc_rpc_read_skip_items(ctx->rpc(), narg);
    }
    return items;
}


static inline int nvc_ui_redraw_action_mouse_on(nvc::UIContext *ctx, int items) {
    if (likely(items > 0)) {
        ctx->cb().mouse_on(ctx->userdata());
    }
    return items;
}

static inline int nvc_ui_redraw_action_mouse_off(nvc::UIContext *ctx, int items) {
    if (likely(items > 0)) {
        ctx->cb().mouse_off(ctx->userdata());
    }
    return items;
}

static inline int nvc_ui_redraw_action_busy_start(nvc::UIContext *ctx, int items) {
    ctx->show_cursor(false);
    return items;
}

static inline int nvc_ui_redraw_action_busy_stop(nvc::UIContext *ctx, int items) {
    ctx->show_cursor(true);
    return items;
}

static inline int nvc_ui_redraw_action_bell(nvc::UIContext *ctx, int items) {
    NSBeep();
    return items;
}

static inline int nvc_ui_redraw_action_flush(nvc::UIContext *ctx, int items) {
    if (!CGRectIsEmpty(ctx->dirty())) {
        CGRect dirty = ctx->dirty();
        ctx->clear_dirty();
        dirty.origin.x *= ctx->cell_size().width;
        dirty.origin.y *= ctx->cell_size().height;
        dirty.size.width *= ctx->cell_size().width;
        dirty.size.height *= ctx->cell_size().height;
        ctx->cb().flush(ctx->userdata(), dirty);
    }
    return items;
}

static inline int nvc_ui_redraw_action_grid_resize(nvc::UIContext *ctx, int items) {
    if (likely(items-- > 0)) {
        int narg = nvc_rpc_read_array_size(ctx->rpc());
        if (likely(narg >= 3)) {
            narg -= 3;
            int grid_id = nvc_rpc_read_int(ctx->rpc());
            int width = nvc_rpc_read_int(ctx->rpc());
            int height = nvc_rpc_read_int(ctx->rpc());
            NVLogD("nvc ui grid resize %d - %dx%d", grid_id, width, height);
            ctx->resize_grid(grid_id, width, height);
        }
        nvc_rpc_read_skip_items(ctx->rpc(), narg);
    }
    return items;
}

static inline int nvc_ui_redraw_action_default_colors_set(nvc::UIContext *ctx, int items) {
    if (likely(items-- > 0)) {
        int narg = nvc_rpc_read_array_size(ctx->rpc());
        if (likely(narg >= 3)) {
            narg -= 3;
            auto& color = ctx->color();
            color.default_color(nvc::ui_color_code_foreground, nvc_rpc_read_uint32(ctx->rpc()));
            color.default_color(nvc::ui_color_code_background, nvc_rpc_read_uint32(ctx->rpc()));
            color.default_color(nvc::ui_color_code_special, nvc_rpc_read_uint32(ctx->rpc()));
            ctx->cb().update_background(ctx->userdata(), color.default_color(nvc::ui_color_code_background));
        }
        nvc_rpc_read_skip_items(ctx->rpc(), narg);
    }
    return items;
}

static inline int nvc_ui_redraw_action_hl_attr_define(nvc::UIContext *ctx, int items) {
    while (items-- > 0) {
        int narg = nvc_rpc_read_array_size(ctx->rpc());
        if (likely(narg >= 2)) {
            narg -= 2;
            int hl_id = nvc_rpc_read_int(ctx->rpc());
            nvc::UIColorSet colors;
            for (int mn = nvc_rpc_read_map_size(ctx->rpc()); mn > 0; mn--) {
                const auto& key = nvc_rpc_read_str(ctx->rpc());
                const auto& p = nvc_ui_color_name_table.find(key);
                if (unlikely(p != nvc_ui_color_name_table.end())) {
                    colors[p->second] = nvc_rpc_read_uint32(ctx->rpc());
                } else {
                    NVLogW("nvc ui invalid hl attr %s", key.c_str());
                    nvc_rpc_read_skip_items(ctx->rpc(), 1);
                }
            }
            ctx->color().update_hl_attrs(hl_id, colors);
        }
        nvc_rpc_read_skip_items(ctx->rpc(), narg);
    }
    return 0;
}

typedef void (*nvc_ui_hl_attrs_notification)(nvc::UIContext *ctx, const nvc::UIColorSet &attrs);

static inline void nvc_ui_hl_attrs_notification_TabLineFill(nvc::UIContext *ctx, const nvc::UIColorSet &attrs) {
    const auto& p = attrs.find(nvc::ui_color_code_background);
    if (p != attrs.end()) {
        ctx->cb().update_tab_background(ctx->userdata(), p->second);
    }
}

#define NVC_UI_HL_ATTRS_NOTIFICATION(_action)    { #_action, nvc_ui_hl_attrs_notification_##_action }
static const std::map<const std::string, nvc_ui_hl_attrs_notification> nvc_ui_hl_groups_notification = {
    NVC_UI_HL_ATTRS_NOTIFICATION(TabLineFill),
};

static inline void nvc_ui_notify_hl_attrs(nvc::UIContext *ctx, const std::string& name) {
    const auto& notification = nvc_ui_hl_groups_notification.find(name);
    if (notification != nvc_ui_hl_groups_notification.end()) {
        auto colors = ctx->color().find_hl_colors(name);
        if (colors != nullptr) {
            notification->second(ctx, *colors);
        }
    }
}

static inline int nvc_ui_redraw_action_hl_group_set(nvc::UIContext *ctx, int items) {
    while (items-- > 0) {
        int narg = nvc_rpc_read_array_size(ctx->rpc());
        if (likely(narg >= 2)) {
            narg -= 2;
            const auto& name = nvc_rpc_read_str(ctx->rpc());
            ctx->color().update_hl_groups(name, nvc_rpc_read_int(ctx->rpc()));
            nvc_ui_notify_hl_attrs(ctx, name);
        }
        nvc_rpc_read_skip_items(ctx->rpc(), narg);
    }
    return 0;
}

static inline int nvc_ui_redraw_action_grid_line(nvc::UIContext *ctx, int items) {
    ctx->update_grid_line(items);
    return 0;
}

static inline int nvc_ui_redraw_action_grid_clear(nvc::UIContext *ctx, int items) {
    if (likely(items-- > 0)) {
        int narg = nvc_rpc_read_array_size(ctx->rpc());
        if (likely(narg-- > 0)) {
            ctx->clear_grid(nvc_rpc_read_int(ctx->rpc()));
        }
        nvc_rpc_read_skip_items(ctx->rpc(), narg);
    }
    return items;
}

static inline int nvc_ui_redraw_action_grid_destroy(nvc::UIContext *ctx, int items) {
    if (likely(items-- > 0)) {
        int narg = nvc_rpc_read_array_size(ctx->rpc());
        if (likely(narg-- > 0)) {
            int grid_id = nvc_rpc_read_int(ctx->rpc());
            ctx->destroy_grid(grid_id);
            NVLogD("nvc ui grid %d destroy", grid_id);
        }
        nvc_rpc_read_skip_items(ctx->rpc(), narg);
    }
    return items;
}

static inline int nvc_ui_redraw_action_grid_cursor_goto(nvc::UIContext *ctx, int items) {
    if (likely(items-- > 0)) {
        int narg = nvc_rpc_read_array_size(ctx->rpc());
        if (likely(narg >= 3)) {
            narg -= 3;
            int grid_id = nvc_rpc_read_int(ctx->rpc());
            int row = nvc_rpc_read_int(ctx->rpc());
            int column = nvc_rpc_read_int(ctx->rpc());
            ctx->update_grid_cursor(grid_id, nvc::UIPoint(column, row));
        }
        nvc_rpc_read_skip_items(ctx->rpc(), narg);
    }
    return items;
}

static inline int nvc_ui_redraw_action_grid_scroll(nvc::UIContext *ctx, int items) {
    if (likely(items-- > 0)) {
        int narg = nvc_rpc_read_array_size(ctx->rpc());
        if (likely(narg >= 6)) {
            narg -= 6;
            int grid_id = nvc_rpc_read_int(ctx->rpc());
            int top = nvc_rpc_read_int(ctx->rpc());
            int bot = nvc_rpc_read_int(ctx->rpc());
            int left = nvc_rpc_read_int(ctx->rpc());
            int right = nvc_rpc_read_int(ctx->rpc());
            int rows = nvc_rpc_read_int(ctx->rpc());
            ctx->scroll_grid(grid_id, nvc::UIRect(left, top, right - left, bot - top), rows);
        }
        nvc_rpc_read_skip_items(ctx->rpc(), narg);
    }
    return items;
}

static inline int nvc_ui_redraw_action_tabline_update(nvc::UIContext *ctx, int items) {
    if (likely(items-- > 0)) {
        int narg = nvc_rpc_read_array_size(ctx->rpc());
        if (likely(narg >= 2)) {
            narg -= 2;
            nvc_rpc_object_handler_t handler = nvc_rpc_read_ext_handle(ctx->rpc());
            // TODO: fix tab list
            //nvc_ui_tab_list_t tabs;
            int tn = nvc_rpc_read_array_size(ctx->rpc());
            while (tn-- > 0) {
                nvc_ui_tab_t tab;
                int mn = nvc_rpc_read_map_size(ctx->rpc());
                while (mn-- > 0) {
                    const auto& name = nvc_rpc_read_str(ctx->rpc());
                    const auto& p = nvc_ui_set_ui_tab_actions.find(name);
                    if (likely(p != nvc_ui_set_ui_tab_actions.end())) {
                        p->second(tab, ctx->rpc());
                    } else {
                        nvc_rpc_read_skip_items(ctx->rpc(), 1);
                        NVLogW("nvc ui tab info: %s", name.c_str());
                    }
                }
                //tabs.push_back(tab);
            }
            //ctx->current_tab = handler;
            //bool list_updated = ctx->tabs != tabs;
            //if (list_updated) {
            //    ctx->tabs = tabs;
            //}
            //ctx->cb.update_tab_list(nvc_ui_get_userdata(ctx), list_updated);
        }
        nvc_rpc_read_skip_items(ctx->rpc(), narg);
    }
    return items;
}

#define NVC_REDRAW_ACTION(_action)          { #_action, nvc_ui_redraw_action_##_action }
#define NVC_REDRAW_ACTION_IGNORE(_action)   { #_action, nullptr }
// NOTE: https://neovim.io/doc/user/ui.html
static const std::map<const std::string, nvc_ui_action_t> nvc_ui_redraw_actions = {
    // Global Events
    NVC_REDRAW_ACTION_IGNORE(set_icon),
    NVC_REDRAW_ACTION(set_title),
    NVC_REDRAW_ACTION(mode_info_set),
    NVC_REDRAW_ACTION(option_set),
    NVC_REDRAW_ACTION(mode_change),
    NVC_REDRAW_ACTION(mouse_on),
    NVC_REDRAW_ACTION(mouse_off),
    NVC_REDRAW_ACTION(busy_start),
    NVC_REDRAW_ACTION(busy_stop),
    NVC_REDRAW_ACTION_IGNORE(suspend),
    NVC_REDRAW_ACTION_IGNORE(update_menu),
    NVC_REDRAW_ACTION(bell),
    NVC_REDRAW_ACTION_IGNORE(visual_bell),
    NVC_REDRAW_ACTION(flush),
    // Grid Events (line-based)
    NVC_REDRAW_ACTION(grid_resize),
    NVC_REDRAW_ACTION(default_colors_set),
    NVC_REDRAW_ACTION(hl_attr_define),
    NVC_REDRAW_ACTION(hl_group_set),
    NVC_REDRAW_ACTION(grid_line),
    NVC_REDRAW_ACTION(grid_clear),
    NVC_REDRAW_ACTION(grid_destroy),
    NVC_REDRAW_ACTION(grid_cursor_goto),
    NVC_REDRAW_ACTION(grid_scroll),
    // Multigrid Events
    NVC_REDRAW_ACTION_IGNORE(win_pos),
    NVC_REDRAW_ACTION_IGNORE(win_float_pos),
    NVC_REDRAW_ACTION_IGNORE(win_external_pos),
    NVC_REDRAW_ACTION_IGNORE(win_hide),
    NVC_REDRAW_ACTION_IGNORE(win_close),
    NVC_REDRAW_ACTION_IGNORE(msg_set_pos),
    NVC_REDRAW_ACTION_IGNORE(win_viewport),
    NVC_REDRAW_ACTION_IGNORE(win_extmark),
    // Popupmenu Events
    NVC_REDRAW_ACTION_IGNORE(popupmenu_show),
    NVC_REDRAW_ACTION_IGNORE(popupmenu_select),
    NVC_REDRAW_ACTION_IGNORE(popupmenu_hide),
    // Tabline Events
    NVC_REDRAW_ACTION(tabline_update),
    // Cmdline Events
    NVC_REDRAW_ACTION_IGNORE(cmdline_show),
    NVC_REDRAW_ACTION_IGNORE(cmdline_pos),
    NVC_REDRAW_ACTION_IGNORE(cmdline_special_char),
    NVC_REDRAW_ACTION_IGNORE(cmdline_hide),
    NVC_REDRAW_ACTION_IGNORE(cmdline_block_show),
    NVC_REDRAW_ACTION_IGNORE(cmdline_block_append),
    NVC_REDRAW_ACTION_IGNORE(cmdline_block_hide),
    // Message/Dialog Events
    NVC_REDRAW_ACTION_IGNORE(msg_show),
    NVC_REDRAW_ACTION_IGNORE(msg_clear),
    NVC_REDRAW_ACTION_IGNORE(msg_showmode),
    NVC_REDRAW_ACTION_IGNORE(msg_showcmd),
    NVC_REDRAW_ACTION_IGNORE(msg_ruler),
    NVC_REDRAW_ACTION_IGNORE(msg_history_show),
    NVC_REDRAW_ACTION_IGNORE(msg_history_clear),
};

#pragma mark - NVC UI Notification Actions
static inline int nvc_ui_notification_action_redraw(nvc::UIContext *ctx, int items) {
    while (items-- > 0) {
        int narg = nvc_rpc_read_array_size(ctx->rpc());
        if (likely(narg-- > 0)) {
            const auto& action = nvc_rpc_read_str(ctx->rpc());
            const auto& p = nvc_ui_redraw_actions.find(action);
            if (unlikely(p == nvc_ui_redraw_actions.end())) {
                NVLogW("nvc ui unknown redraw action: %s", action.c_str());
            } else if (p->second != nullptr) {
                //NVLogW("nvc ui redraw action: %s", action.c_str());
                narg = p->second(ctx, narg);
            }
            nvc_rpc_read_skip_items(ctx->rpc(), narg);
        }
    }
    return 0;
}

#define NVC_NOTIFICATION_ACTION(_action)    { #_action, nvc_ui_notification_action_##_action}
static const std::map<const std::string, nvc_ui_action_t> nvc_ui_notification_actions = {
    NVC_NOTIFICATION_ACTION(redraw),
};

#pragma mark - NVC UI Helper
static int nvc_ui_response_handler(nvc_rpc_context_t *ctx, int items) {
    if (likely(items-- > 0)) {
        uint64_t msgid = nvc_rpc_read_uint64(ctx);
        NVLogD("nvc ui recive response msgid: %lu", msgid);
        if (likely(items-- > 0)) {
            int n = nvc_rpc_read_array_size(ctx);
            if (n >= 2) {
                n -= 2;
                int64_t code = nvc_rpc_read_int64(ctx);
                const auto& msg = nvc_rpc_read_str(ctx);
                NVLogE("nvc ui match request %ld failed(%lu): %s", msgid, code, msg.c_str());
            }
            nvc_rpc_read_skip_items(ctx, n);
        }
        if (likely(items-- > 0)) {
            int n = nvc_rpc_read_map_size(ctx);
            if (n > 0) {
                nvc_rpc_read_skip_items(ctx, n * 2);
            }
        }
    }
    return items;
}

static int nvc_ui_notification_handler(nvc_rpc_context_t *ctx, int items) {
    if (likely(items-- > 0)) {
        const auto& action = nvc_rpc_read_str(ctx);
        const auto& p = nvc_ui_notification_actions.find(action);
        if (unlikely(p == nvc_ui_notification_actions.end())) {
            NVLogW("nvc ui unknown notification action: %s", action.c_str());
        } else if (likely(items-- > 0)) {
            nvc_rpc_read_skip_items(ctx, p->second((nvc::UIContext *)nvc_rpc_get_userdata(ctx), nvc_rpc_read_array_size(ctx)));
        }
    }
    return items;
}

static int nvc_ui_close_handler(nvc_rpc_context_t *ctx, int items) {
    if (ctx != nullptr) {
        nvc::UIContext *ui_ctx = (nvc::UIContext *)nvc_rpc_get_userdata(ctx);
        ui_ctx->cb().close(ui_ctx->userdata());
    }
    return items;
}
