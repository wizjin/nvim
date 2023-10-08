//
//  nvc_ui.cpp
//  NVim
//
//  Created by wizjin on 2023/9/20.
//

#include "nvc_ui.h"
#include <mutex>
#include <string>
#include <vector>
#include <map>
#include "nvc_rpc.h"

#define kNvcUiKeysMax                       64
#define kNvcUiCacheGlyphMax                 127
#define kNvcUiCacheGlyphSize                512

#define nvc_ui_to_context(_ptr, _member)    nv_member_to_struct(nvc_ui_context_t, _ptr, _member)
#define nvc_ui_get_userdata(_ctx)           nvc_rpc_get_userdata(&(_ctx)->rpc)
#define nvc_lock_guard_t                    std::lock_guard<std::mutex>

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
typedef int (*nvc_ui_action_t)(nvc_ui_context_t *ctx, int narg);

#define NVC_UI_COLOR_CODE_LIST              \
    NVC_UI_COLOR_CODE(foreground),          \
    NVC_UI_COLOR_CODE(background),          \
    NVC_UI_COLOR_CODE(special),             \
    NVC_UI_COLOR_CODE(reverse),             \
    NVC_UI_COLOR_CODE(italic),              \
    NVC_UI_COLOR_CODE(bold),                \
    NVC_UI_COLOR_CODE(nocombine),           \
    NVC_UI_COLOR_CODE(strikethrough),       \
    NVC_UI_COLOR_CODE(underline),           \
    NVC_UI_COLOR_CODE(undercurl),           \
    NVC_UI_COLOR_CODE(underdouble),         \
    NVC_UI_COLOR_CODE(underdotted),         \
    NVC_UI_COLOR_CODE(underdashed),         \
    NVC_UI_COLOR_CODE(altfont),             \
    NVC_UI_COLOR_CODE(blend)

#define NVC_UI_COLOR_CODE(_code)            nvc_ui_color_code_##_code
typedef enum nvc_ui_color_code { NVC_UI_COLOR_CODE_LIST } nvc_ui_color_code_t;

#undef NVC_UI_COLOR_CODE
#define NVC_UI_COLOR_CODE(_code)            { #_code, nvc_ui_color_code_##_code }
static const std::map<const std::string, nvc_ui_color_code_t> nvc_ui_color_name_table = { NVC_UI_COLOR_CODE_LIST };

typedef struct nvc_ui_cell_pos {
    uint32_t x;
    uint32_t y;
} nvc_ui_cell_pos_t;

typedef struct nvc_ui_cell_size {
    uint32_t width;
    uint32_t height;
} nvc_ui_cell_size_t;

typedef struct nvc_ui_cell_rect {
    uint32_t x;
    uint32_t y;
    uint32_t width;
    uint32_t height;
    
    inline bool contains(const nvc_ui_cell_pos& pos) const {
        return pos.x >= x && pos.y >= y && pos.x <= x + width && pos.y <= y + height;
    }
} nvc_ui_cell_rect_t;

typedef struct mode_info {
    std::string name;
    std::string short_name;
    std::string cursor_shape;
    std::string mouse_shape;
    int         cell_percentage;
    int         blinkwait;
    int         blinkon;
    int         blinkoff;
    int         attr_id;
    int         attr_id_lm;
    
    inline CGFloat calc_cell_percentage(int value) const {
        return (CGFloat)(value * cell_percentage)/100.0;
    }
} mode_info_t;

typedef void (*nvc_ui_set_mode_info)(mode_info_t &info, nvc_rpc_context_t *ctx);

static inline void nvc_ui_set_mode_info_null(mode_info_t &info, nvc_rpc_context_t *ctx) {
    nvc_rpc_read_skip_items(ctx, 1);
}

#define NVC_UI_SET_MODE_INFO_STR(_action)   { #_action, [](mode_info_t &info, nvc_rpc_context_t *ctx) { info._action = nvc_rpc_read_str(ctx); } }
#define NVC_UI_SET_MODE_INFO_INT(_action)   { #_action, [](mode_info_t &info, nvc_rpc_context_t *ctx) { info._action = nvc_rpc_read_int(ctx); } }
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

typedef struct nvc_ui_cell {
    UniChar     ch;
    CGGlyph     glyph;
    int         hl_id;
} nvc_ui_cell_t;

class nvc_ui_grid {
private:
    int                 m_width;
    int                 m_height;
    nvc_ui_cell_pos_t   m_cursor;
    nvc_ui_cell_t       *m_cells;
public:
    inline nvc_ui_grid(uint32_t width, uint32_t height) : m_width(width), m_height(height) {
        m_cursor.x = -1;
        m_cursor.y = -1;
        m_cells = (nvc_ui_cell_t *)malloc(sizeof(nvc_ui_cell_t) * m_width * m_height);
    }
    
    inline ~nvc_ui_grid() {
        if (m_cells != nullptr) {
            free(m_cells);
            m_cells = nullptr;
        }
    }
    
    inline int width(void) const { return m_width; }
    inline int height(void) const { return m_height; }
    inline const nvc_ui_cell_pos_t& cursor(void) const { return m_cursor;   }
    inline bool has_cursor(void) const { return m_cursor.x >= 0 && m_cursor.y >= 0; }
    
    inline void resize(uint32_t width, uint32_t height) {
        void *ptr = realloc(m_cells, sizeof(nvc_ui_cell_t) * width * height);
        if (likely(ptr != nullptr)) {
            m_width = width;
            m_height = height;
            m_cells = (nvc_ui_cell_t *)ptr;
        }
    }
    
    inline void clear() {
        bzero(m_cells, sizeof(nvc_ui_cell_t) * m_width * m_height);
    }
    
    inline void update(int row, int col_start, int count, UniChar ch, CGGlyph glyph, uint32_t hl_id) {
        if (likely(count > 0 && row >= 0 && row < m_height && col_start >= 0 && col_start < m_width)) {
            nvc_ui_cell_t *cell = m_cells + row*m_width + col_start;
            for (int n = MIN(count, m_width - col_start); n > 0; n--) {
                cell->ch = ch;
                cell->glyph = glyph;
                cell->hl_id = hl_id;
                cell++;
            }
        }
    }
    
    inline void update_cursor(int x, int y) {
        m_cursor.x = x;
        m_cursor.y = y;
    }
    
    inline nvc_ui_cell_rect scroll(int top, int bot, int left, int right, int rows) {
        nvc_ui_cell_rect rect = {
            .x = (uint32_t)left,
            .y = (uint32_t)top,
            .width = (uint32_t)MAX(right - left, 0),
            .height = (uint32_t)MAX(bot - top, 0),
        };
        if (likely(rows != 0 && rect.width > 0 && rect.height > 0)) {
            nvc_ui_cell_t *cells = m_cells + left;
            size_t size = sizeof(nvc_ui_cell_t) * rect.width;
            if (rows < 0) {
                for (int i = bot + rows - 1; i >= top; i--) {
                    memcpy(cells + (i - rows) * m_width, cells + i * m_width, size);
                }
            } else {
                int dst = (bot - rows);
                for (int i = top; i < dst; i++) {
                    memcpy(cells + i * m_width, cells + (i + rows) * m_width, size);
                }
            }
        }
        return rect;
    }
    
    inline void draw(nvc_ui_context *ctx, CGContextRef context, const nvc_ui_cell_rect_t& dirty);
};

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

typedef std::map<nvc_ui_color_code_t, nvc_ui_color_t>   nvc_ui_colors_set_t;
typedef std::map<int, nvc_ui_colors_set_t>              nvc_ui_colors_set_map_t;
typedef std::map<std::string, int>                      nvc_ui_group_map_t;
typedef std::map<int, nvc_ui_grid *, std::less<int>>    nvc_ui_grid_map_t;
typedef std::map<UniChar, CGGlyph>                      mvc_ui_glyph_table_t;
typedef std::vector<nvc_ui_tab_t>                       nvc_ui_tab_list_t;
typedef std::vector<mode_info_t>                        mode_info_list_t;

struct nvc_ui_context {
    nvc_rpc_context_t           rpc;
    nvc_ui_callback_t           cb;
    bool                        attached;
    CTFontRef                   font;
    CGFloat                     font_size;
    std::string                 mode;
    int                         mode_idx;
    bool                        mode_enabled;
    CGSize                      cell_size;
    CGFloat                     font_offset;
    CGRect                      dirty_rect;
    std::mutex                  locker;
    nvc_ui_cell_size_t          window_size;
    nvc_ui_colors_set_t         default_colors;
    nvc_rpc_object_handler_t    current_tab;
    nvc_ui_tab_list_t           tabs;
    nvc_ui_colors_set_map_t     hl_attrs;
    nvc_ui_group_map_t          hl_groups;
    mode_info_list_t            mode_infos;
    nvc_ui_grid_map_t           grids;
    mvc_ui_glyph_table_t        glyph_cache;
    CGGlyph                     glyphs[kNvcUiCacheGlyphMax];
};

static int nvc_ui_response_handler(nvc_rpc_context_t *ctx, int items);
static int nvc_ui_notification_handler(nvc_rpc_context_t *ctx, int items);
static int nvc_ui_close_handler(nvc_rpc_context_t *ctx, int items);

static const nvc_ui_cell_size_t nvc_ui_cell_size_zero = { 0, 0 };

#pragma mark - NVC Util Helper
static inline void nvc_util_parse_token(const std::string& value, char delimiter, const std::function<bool(const std::string&)>& action) {
    size_t last_pos = 0;
    while (last_pos != std::string::npos) {
        size_t pos = value.find_first_of(delimiter, last_pos);
        auto token = value.substr(last_pos, (pos == std::string_view::npos ? std::string_view::npos : pos - last_pos));
        if (!action(token)) {
            break;
        }
        if (pos == std::string::npos) {
            last_pos = std::string::npos;
        } else {
            last_pos = pos + 1;
        }
    }
}

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

#pragma mark - NVC UI Helper
static inline int nvc_ui_set_font(nvc_ui_context_t *ctx, CTFontRef font) {
    int res = NVC_RC_ILLEGAL_CALL;
    if (font != nullptr) {
        ctx->font = (CTFontRef)CFRetain(font);
        CGFloat ascent = CTFontGetAscent(ctx->font);
        CGFloat descent = CTFontGetDescent(ctx->font);
        CGFloat leading = CTFontGetLeading(ctx->font);
        CGFloat height = ceil(ascent + descent + leading);
        CGGlyph glyph = (CGGlyph) 0;
        UniChar capitalM = (UniChar) 0x004D;
        CGSize advancement = CGSizeZero;
        CTFontGetGlyphsForCharacters(ctx->font, &capitalM, &glyph, 1);
        CTFontGetAdvancesForGlyphs(ctx->font, kCTFontOrientationHorizontal, &glyph, &advancement, 1);
        CGFloat width = ceil(advancement.width);
        ctx->cell_size = CGSizeMake(width, height);
        ctx->font_offset = descent;
        for (int i = 0; i < kNvcUiCacheGlyphMax; i++) {
            if (isspace(i)) {
                ctx->glyphs[i] = 0;
            } else {
                UniChar ch = i;
                CTFontGetGlyphsForCharacters(ctx->font, &ch, ctx->glyphs + i, 1);
            }
        }
        res = NVC_RC_OK;
    }
    return res;
}

static inline CTFontRef nvc_ui_load_font(const std::string& name, CGFloat font_size) {
    CTFontRef font = nullptr;
    if (!name.empty()) {
        CFStringRef font_name = CFStringCreateWithBytesNoCopy(nullptr, (const UInt8 *)name.c_str(), name.size(), kCFStringEncodingUTF8, false, kCFAllocatorNull);
        if (font_name != nullptr) {
            CTFontRef original_font = CTFontCreateWithName(font_name, font_size, nullptr);
            if (original_font != nullptr) {
                font = CTFontCreateCopyWithFamily(original_font, font_size, nullptr, font_name);
                CFRelease(original_font);
            }
            CFRelease(font_name);
        }
    }
    return font;
}

static inline CGFloat nvc_ui_parse_font_size(const std::string& str, CGFloat font_size) {
    nvc_util_parse_token(str, ':', [&font_size](const std::string& value) -> bool {
        if (!value.empty()) {
            size_t idx = 0;
            if (value.at(idx) == 'h') idx++;
            if (idx != std::string::npos && isdigit(value.at(idx))) {
                double size = atof(value.substr(idx).c_str());
                if (size > 0) {
                    font_size = size;
                    return false;
                }
            }
        }
        return true;
    });
    return font_size;
}

static inline nvc_ui_cell_size_t nvc_ui_size2cell(nvc_ui_context_t *ctx, const CGSize& size) {
    nvc_ui_cell_size_t cell;
    cell.width = floor(size.width/ctx->cell_size.width);
    cell.height = floor(size.height/ctx->cell_size.height);
    return cell;
}

static inline CGPoint nvc_ui_cell2point(nvc_ui_context_t *ctx, const nvc_ui_cell_pos_t& pos) {
    return CGPointMake(ctx->cell_size.width * pos.x, ctx->cell_size.height * pos.y);
}

static inline CGSize nvc_ui_cell2size(nvc_ui_context_t *ctx, const nvc_ui_cell_size_t& size) {
    return CGSizeMake(ctx->cell_size.width * size.width, ctx->cell_size.height * size.height);
}

static inline nvc_ui_cell_rect_t nvc_ui_rect2cell(nvc_ui_context_t *ctx, const CGRect& rect) {
    nvc_ui_cell_rect_t rc;
    rc.x = MAX(floor(rect.origin.x/ctx->cell_size.width), 0);
    rc.y = MAX(floor(rect.origin.y/ctx->cell_size.height), 0);
    rc.width = MIN(ceil(rect.size.width/ctx->cell_size.width), ctx->window_size.width - rc.x);
    rc.height = MIN(ceil(rect.size.height/ctx->cell_size.height), ctx->window_size.height - rc.y);
    return rc;
}

static inline bool nvc_ui_size_equals(const nvc_ui_cell_size_t& s1, const nvc_ui_cell_size_t& s2) {
    return s1.width == s2.width && s1.height == s2.height;
}

static inline void nvc_ui_update_dirty(nvc_ui_context_t *ctx, int x, int y, int width, int height) {
    CGRect rc = CGRectMake(x, y, width, height);
    if (CGRectIsEmpty(ctx->dirty_rect)) {
        ctx->dirty_rect = rc;
    } else {
        ctx->dirty_rect = CGRectUnion(ctx->dirty_rect, rc);
    }
}

static inline bool nvc_ui_update_size(nvc_ui_context_t *ctx, CGSize size) {
    bool res = false;
    nvc_ui_cell_size_t wnd_size = nvc_ui_size2cell(ctx, size);
    if (!nvc_ui_size_equals(ctx->window_size, wnd_size)) {
        ctx->window_size = wnd_size;
        res = true;
    }
    return res;
}

static inline CGGlyph nvc_ui_ch2glyph(nvc_ui_context_t *ctx, UniChar ch) {
    CGGlyph glyph = 0;
    if (ch < kNvcUiCacheGlyphMax) {
        glyph = ctx->glyphs[ch];
    } else {
        const auto& p = ctx->glyph_cache.find(ch);
        if (p != ctx->glyph_cache.end()) {
            glyph = p->second;
        } else {
            CTFontGetGlyphsForCharacters(ctx->font, &ch, &glyph, 1);
            if (ctx->glyph_cache.size() > kNvcUiCacheGlyphSize) {
                ctx->glyph_cache.clear();
            }
            ctx->glyph_cache[ch] = glyph;
        }
    }
    return glyph;
}

static inline void nvc_ui_set_fill_color(CGContextRef context, uint32_t rgb) {
    const uint8_t *c = (const uint8_t *)&rgb;
    CGContextSetRGBFillColor(context, c[2]/255.0, c[1]/255.0, c[0]/255.0, 1.0);
}

static inline uint32_t nvc_ui_get_default_color(nvc_ui_context_t *ctx, nvc_ui_color_code_t code) {
    uint32_t c = 0;
    const auto& p = ctx->default_colors.find(code);
    if (likely(p != ctx->default_colors.end())) {
        c = p->second;
    }
    return c;
}

static inline uint32_t nvc_ui_find_hl_color(nvc_ui_context_t *ctx, int hl, nvc_ui_color_code_t code) {
    const auto& p = ctx->hl_attrs.find(hl);
    if (likely(p != ctx->hl_attrs.end())) {
        const auto& q = p->second.find(code);
        if (q != p->second.end()) {
            return q->second;
        }
    }
    return nvc_ui_get_default_color(ctx, code);
}

inline void nvc_ui_grid::draw(nvc_ui_context *ctx, CGContextRef context, const nvc_ui_cell_rect_t& dirty) {
    CGPoint pt;
    CGSize size = ctx->cell_size;
    CGFloat font_offset = ctx->cell_size.height - ctx->font_offset;
    int last_hl_id = 0;
    int width = MIN(m_width, dirty.x + dirty.width);
    int height = MIN(m_height, dirty.y + dirty.height);
    bool need_cursor = ctx->mode_enabled && dirty.contains(m_cursor);
    uint32_t fg = nvc_ui_get_default_color(ctx, nvc_ui_color_code_foreground);
    uint32_t bg = nvc_ui_get_default_color(ctx, nvc_ui_color_code_background);
    for (int j = dirty.y; j < height; j++) {
        pt.y = size.height * j;
        nvc_ui_cell_t *cell = m_cells + j * ctx->window_size.width + dirty.x;
        for (int i = dirty.x; i < width; i++) {
            pt.x = size.width * i;
            if (cell->hl_id != last_hl_id) {
                last_hl_id = cell->hl_id;
                fg = nvc_ui_find_hl_color(ctx, last_hl_id, nvc_ui_color_code_foreground);
                bg = nvc_ui_find_hl_color(ctx, last_hl_id, nvc_ui_color_code_background);
            }
            if (last_hl_id != 0) {
                nvc_ui_set_fill_color(context, bg);
                CGContextFillRect(context, CGRectMake(pt.x, pt.y, size.width, size.height));
            }
            uint32_t tc = fg;
            if (need_cursor && m_cursor.x == i && m_cursor.y == j) {
                if (ctx->mode_idx < ctx->mode_infos.size()) {
                    const auto& info = ctx->mode_infos[ctx->mode_idx];
                    nvc_ui_set_fill_color(context, nvc_ui_find_hl_color(ctx, info.attr_id, nvc_ui_color_code_foreground));
                    if (info.cursor_shape == "block") {
                        CGContextFillRect(context, CGRectMake(pt.x, pt.y, size.width, size.height));
                        tc = nvc_ui_find_hl_color(ctx, info.attr_id, nvc_ui_color_code_background);
                    } else if (info.cursor_shape == "horizontal") {
                        CGContextFillRect(context, CGRectMake(pt.x, pt.y, size.width, info.calc_cell_percentage(size.height)));
                    } else if (info.cursor_shape == "vertical") {
                        CGContextFillRect(context, CGRectMake(pt.x, pt.y, info.calc_cell_percentage(size.width), size.height));
                    }
                }
            }
            if (cell->glyph != 0) {
                nvc_ui_set_fill_color(context, tc);
                CGContextSetTextPosition(context, pt.x, pt.y + font_offset);
                CTFontDrawGlyphs(ctx->font, &cell->glyph, &CGPointZero, 1, context);
            }
            cell++;
        }
    }
}

static inline UniChar nvc_ui_utf82unicode(const uint8_t *str, uint8_t len) {
    UniChar unicode = 0;
    switch (len) {
        case 0:
            break;
        case 1:
            unicode = str[0] & 0x7F;
            break;
        case 2:
            if (str[1] == 0x80) {
                unicode = ((str[0] & 0x1F) << 6)
                        | (str[1] & 0x3F);
            }
            break;
        case 3:
            if (((str[1]&0xC0) == 0x80) && ((str[2]&0xC0) == 0x80)) {
                unicode = ((str[0] & 0x0F) << 12)
                        | ((str[1] & 0x3F) << 6)
                        | (str[2] & 0x3F);
            }
            break;
        case 4:
            if (((str[1]&0xC0) == 0x80) && ((str[2]&0xC0) == 0x80) && ((str[3]&0xC0) == 0x80)) {
                uint32_t cp = ((str[0] & 0x07) << 18)
                        | ((str[1] & 0x3F) << 12)
                        | ((str[2] & 0x3F) << 6)
                        | (str[3] & 0x3F);
                cp -= 0x10000;
                unicode = ((0xD800 + ((cp >> 10)&0x03FF)) << 8) | (0xDC00 + (cp & 0x03FF));
            }
            break;
        default:
            NVLogW("nvc ui invalid utf8 %d", len);
            break;
    }
    return unicode;
}

#pragma mark - NVC UI API
nvc_ui_context_t *nvc_ui_create(int inskt, int outskt, const nvc_ui_config_t *config, const nvc_ui_callback_t *callback, void *userdata) {
    nvc_ui_context_t *ctx = nullptr;
    if (inskt != INVALID_SOCKET && outskt != INVALID_SOCKET && config != NULL && callback != NULL) {
        ctx = new nvc_ui_context_t();
        if (unlikely(ctx == nullptr)) {
            NVLogE("nvc ui create context failed");
        } else {
            ctx->attached = false;
            ctx->cb = *callback;
            ctx->font = nullptr;
            ctx->mode_idx = 0;
            ctx->mode_enabled = true;
            ctx->cell_size = CGSizeZero;
            ctx->dirty_rect = CGRectZero;
            ctx->window_size = nvc_ui_cell_size_zero;
            int res = nvc_rpc_init(&ctx->rpc, inskt, outskt, userdata, nvc_ui_response_handler, nvc_ui_notification_handler, nvc_ui_close_handler);
            if (res == NVC_RC_OK) {
                ctx->font_size = config->font_size;
                res = nvc_ui_set_font(ctx, config->font);
            }
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

void nvc_ui_destroy(nvc_ui_context_t *ctx) {
    if (ctx != nullptr) {
        nvc_rpc_final(&ctx->rpc);
        for (const auto& p : ctx->grids) {
            delete p.second;
        }
        ctx->grids.clear();
        if (ctx->font != nullptr) {
            CFRelease(ctx->font);
            ctx->font = nullptr;
        }
        delete ctx;
    }
}

bool nvc_ui_is_attached(nvc_ui_context_t *ctx) {
    bool res = false;
    if (likely(ctx != nullptr)) {
        res = ctx->attached;
    }
    return res;
}

CGSize nvc_ui_attach(nvc_ui_context_t *ctx, CGSize size) {
    if (likely(ctx != nullptr && !ctx->attached)) {
        ctx->attached = true;
        nvc_ui_update_size(ctx, size);
        nvc_rpc_call_const_begin(&ctx->rpc, "nvim_ui_attach", 3);
        nvc_rpc_write_unsigned(&ctx->rpc, ctx->window_size.width);
        nvc_rpc_write_unsigned(&ctx->rpc, ctx->window_size.height);
        nvc_rpc_write_map_size(&ctx->rpc, 3);
        nvc_rpc_write_const_str(&ctx->rpc, "override");
        nvc_rpc_write_false(&ctx->rpc);
        nvc_rpc_write_const_str(&ctx->rpc, "ext_linegrid");
        nvc_rpc_write_true(&ctx->rpc);
        nvc_rpc_write_const_str(&ctx->rpc, "ext_tabline");
        nvc_rpc_write_false(&ctx->rpc);
        nvc_rpc_call_end(&ctx->rpc);
        size = nvc_ui_cell2size(ctx, ctx->window_size);
    }
    return size;
}

void nvc_ui_detach(nvc_ui_context_t *ctx) {
    if (likely(ctx != nullptr && ctx->attached)) {
        ctx->attached = false;
        nvc_rpc_call_const_begin(&ctx->rpc, "nvim_ui_detach", 0);
        nvc_rpc_call_end(&ctx->rpc);
    }
}

void nvc_ui_redraw(nvc_ui_context_t *ctx, CGContextRef context) {
    if (likely(ctx != nullptr && ctx->attached && context != nullptr)) {
        nvc_ui_cell_rect_t rc = nvc_ui_rect2cell(ctx, CGContextGetClipBoundingBox(context));
        nvc_lock_guard_t guard(ctx->locker);
        for (const auto& [key, grid] : ctx->grids) {
            grid->draw(ctx, context, rc);
        }
    }
}

CGSize nvc_ui_resize(nvc_ui_context_t *ctx, CGSize size) {
    if (likely(ctx != nullptr && ctx->attached)) {
        if (nvc_ui_update_size(ctx, size)) {
            nvc_rpc_call_const_begin(&ctx->rpc, "nvim_ui_try_resize", 2);
            nvc_rpc_write_unsigned(&ctx->rpc, ctx->window_size.width);
            nvc_rpc_write_unsigned(&ctx->rpc, ctx->window_size.height);
            nvc_rpc_call_end(&ctx->rpc);
            size = nvc_ui_cell2size(ctx, ctx->window_size);
        }
    }
    return size;
}

CGFloat nvc_ui_get_line_height(nvc_ui_context_t *ctx) {
    CGFloat height = 0;
    if (likely(ctx != nullptr && ctx->attached)) {
        height = ctx->cell_size.height;
    }
    return height;
}

CGPoint nvc_ui_get_cursor_position(nvc_ui_context_t *ctx) {
    CGPoint pt = CGPointZero;
    if (likely(ctx != nullptr && ctx->attached)) {
        nvc_lock_guard_t guard(ctx->locker);
        for (const auto& [key, grid] : ctx->grids) {
            if (grid->has_cursor()) {
                pt = nvc_ui_cell2point(ctx, grid->cursor());
                break;
            }
        }
        
    }
    return pt;
}

void nvc_ui_open_file(nvc_ui_context_t *ctx, const char *file, uint32_t len, bool new_tab) {
    if (likely(ctx != nullptr && ctx->attached && len > 0)) {
        nvc_rpc_call_command_const_begin(&ctx->rpc, (new_tab ? "tabnew" : "edit"), 1);
        nvc_rpc_write_str(&ctx->rpc, file, len);
        nvc_rpc_call_command_end(&ctx->rpc);
    }
}

void nvc_ui_tab_next(nvc_ui_context_t *ctx, int count) {
    if (likely(ctx != nullptr && ctx->attached)) {
        nvc_rpc_call_command_const_begin(&ctx->rpc, "tabnext", 1);
        nvc_rpc_write_signed(&ctx->rpc, count);
        nvc_rpc_call_command_end(&ctx->rpc);
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

void nvc_ui_input_rawkey(nvc_ui_context_t *ctx, const char* keys, uint32_t len) {
    if (likely(ctx != nullptr && ctx->attached)) {
        nvc_rpc_call_const_begin(&ctx->rpc, "nvim_input", 1);
        nvc_rpc_write_str(&ctx->rpc, keys, len);
        nvc_rpc_call_end(&ctx->rpc);
    }
}

#undef NVC_UI_MOUSE_KEY
#define NVC_UI_MOUSE_KEY(_key)      #_key,
static const std::string nvc_ui_mouse_key_name[]  = { NVC_UI_MOUSE_KEY_LIST };

#undef NVC_UI_MOUSE_ACTION
#define NVC_UI_MOUSE_ACTION(_key)   #_key,
static const std::string nvc_ui_mouse_action_name[]  = { NVC_UI_MOUSE_ACTION_LIST };

void nvc_ui_input_mouse(nvc_ui_context_t *ctx, nvc_ui_mouse_info_t mouse) {
    if (likely(ctx != nullptr && ctx->attached)) {
        nvc_rpc_call_const_begin(&ctx->rpc, "nvim_input_mouse", 6);
        nvc_rpc_write_string(&ctx->rpc, nvc_ui_mouse_key_name[mouse.key]);
        nvc_rpc_write_string(&ctx->rpc, nvc_ui_mouse_action_name[mouse.action]);
        uint32_t len = 0;
        char modifier[kNvcUiKeysMax];
        len = nvc_ui_key_flags_encode(mouse.flags, modifier, len, true);
        nvc_rpc_write_str(&ctx->rpc, modifier, len);
        nvc_rpc_write_unsigned(&ctx->rpc, 0);
        nvc_rpc_write_signed(&ctx->rpc, mouse.point.y/ctx->cell_size.height);
        nvc_rpc_write_signed(&ctx->rpc, mouse.point.x/ctx->cell_size.width);
        nvc_rpc_call_end(&ctx->rpc);
    }
}

#pragma mark - NVC UI Option Actions
static inline int nvc_ui_option_set_action_guifont(nvc_ui_context_t *ctx, int items) {
    if (likely(items-- > 0)) {
        const auto& value = nvc_rpc_read_str(&ctx->rpc);
        if (!value.empty() && value != "*") {
            nvc_util_parse_token(value, ',', [ctx](const std::string& name) -> bool {
                bool result = true;
                CGFloat font_size = ctx->font_size;
                size_t p = name.find_first_of(':');
                auto family = name.substr(0, p);
                if (p != std::string::npos) {
                    auto args = name.substr(p + 1);
                    font_size = nvc_ui_parse_font_size(args, ctx->font_size);
                }
                CTFontRef font = nvc_ui_load_font(family, font_size);
                if (font != nullptr) {
                    if (nvc_ui_set_font(ctx, font) == NVC_RC_OK) {
                        ctx->cb.font_updated(nvc_ui_get_userdata(ctx));
                        result = false;
                    }
                    CFRelease(font);
                }
                return result;
            });
        }
    }
    return items;
}

static inline int nvc_ui_option_set_action_ext_tabline(nvc_ui_context_t *ctx, int items) {
    if (likely(items-- > 0)) {
        ctx->cb.enable_ext_tabline(nvc_ui_get_userdata(ctx), nvc_rpc_read_bool(&ctx->rpc));
    }
    return items;
}

#pragma mark - NVC UI Redraw Actions
static inline int nvc_ui_redraw_action_set_title(nvc_ui_context_t *ctx, int count) {
    if (likely(count-- > 0)) {
        int items = nvc_rpc_read_array_size(&ctx->rpc);
        if (likely(items-- > 0)) {
            uint32_t len = 0;
            ctx->cb.update_title(nvc_ui_get_userdata(ctx), nvc_rpc_read_str(&ctx->rpc, &len), len);
        }
        nvc_rpc_read_skip_items(&ctx->rpc, items);
    }
    return count;
}

static inline int nvc_ui_redraw_action_mode_info_set(nvc_ui_context_t *ctx, int items) {
    if (likely(items-- > 0)) {
        int narg = nvc_rpc_read_array_size(&ctx->rpc);
        if (likely(narg >= 2)) {
            narg -= 2;
            bool enabled = nvc_rpc_read_bool(&ctx->rpc);
            mode_info_list_t mode_infos;
            for (int n = nvc_rpc_read_array_size(&ctx->rpc); n > 0; n--) {
                mode_info_t info;
                for (int mn = nvc_rpc_read_map_size(&ctx->rpc); mn > 0; mn--) {
                    const auto& name = nvc_rpc_read_str(&ctx->rpc);
                    const auto& p = nvc_ui_set_mode_info_actions.find(name);
                    if (likely(p != nvc_ui_set_mode_info_actions.end())) {
                        p->second(info, &ctx->rpc);
                    } else {
                        nvc_rpc_read_skip_items(&ctx->rpc, 1);
                        NVLogW("nvc ui unknown mode info: %s", name.c_str());
                    }
                }
                mode_infos.push_back(info);
            }
            ctx->mode_infos = mode_infos;
            ctx->mode_enabled = enabled;
        }
        nvc_rpc_read_skip_items(&ctx->rpc, narg);
    }
    return items;
}

typedef int (*nvc_ui_option_set_action)(nvc_ui_context_t *ctx, int items);
#define NVC_UI_OPTION_SET_ACTION(_action)       { #_action, nvc_ui_option_set_action_##_action }
#define NVC_UI_OPTION_SET_ACTION_NULL(_action)  { #_action, nullptr }
static const std::map<const std::string, nvc_ui_option_set_action> nvc_ui_option_set_actions = {
    NVC_UI_OPTION_SET_ACTION_NULL(arabicshape),
    NVC_UI_OPTION_SET_ACTION_NULL(ambiwidth),
    NVC_UI_OPTION_SET_ACTION_NULL(emoji),
    NVC_UI_OPTION_SET_ACTION(guifont),
    NVC_UI_OPTION_SET_ACTION_NULL(guifontwide),
    NVC_UI_OPTION_SET_ACTION_NULL(linespace),
    NVC_UI_OPTION_SET_ACTION_NULL(mousefocus),
    NVC_UI_OPTION_SET_ACTION_NULL(mousemoveevent),
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

static inline int nvc_ui_redraw_action_option_set(nvc_ui_context_t *ctx, int items) {
    while (items-- > 0) {
        int narg = nvc_rpc_read_array_size(&ctx->rpc);
        if (likely(narg-- > 0)) {
            const auto& key = nvc_rpc_read_str(&ctx->rpc);
            if (likely(narg > 0)) {
                const auto& p = nvc_ui_option_set_actions.find(key);
                if (unlikely(p == nvc_ui_option_set_actions.end())) {
                    NVLogW("nvc ui find unknown option value type: %s", key.c_str());
                } else if (p->second != nullptr) {
                    narg = p->second(ctx, narg);
                }
            }
            nvc_rpc_read_skip_items(&ctx->rpc, narg);
        }
    }
    return 0;
}

static inline int nvc_ui_redraw_action_mode_change(nvc_ui_context_t *ctx, int items) {
    if (likely(items-- > 0)) {
        int narg = nvc_rpc_read_array_size(&ctx->rpc);
        if (likely(narg >= 2)) {
            narg -= 2;
            ctx->mode = nvc_rpc_read_str(&ctx->rpc);
            ctx->mode_idx = nvc_rpc_read_int(&ctx->rpc);
            nvc_lock_guard_t guard(ctx->locker);
            for (const auto& [key, grid] : ctx->grids) {
                if (grid->has_cursor()) {
                    nvc_ui_update_dirty(ctx, grid->cursor().x, grid->cursor().y, 1, 1);
                }
            }
        }
        nvc_rpc_read_skip_items(&ctx->rpc, narg);
    }
    return items;
}


static inline int nvc_ui_redraw_action_mouse_on(nvc_ui_context_t *ctx, int items) {
    if (likely(items > 0)) {
        ctx->cb.mouse_on(nvc_ui_get_userdata(ctx));
    }
    return items;
}

static inline int nvc_ui_redraw_action_mouse_off(nvc_ui_context_t *ctx, int items) {
    if (likely(items > 0)) {
        ctx->cb.mouse_off(nvc_ui_get_userdata(ctx));
    }
    return items;
}

static inline int nvc_ui_redraw_action_flush(nvc_ui_context_t *ctx, int items) {
    if (!CGRectIsEmpty(ctx->dirty_rect)) {
        CGRect dirty = ctx->dirty_rect;
        ctx->dirty_rect = CGRectZero;
        dirty.origin.x *= ctx->cell_size.width;
        dirty.origin.y *= ctx->cell_size.height;
        dirty.size.width *= ctx->cell_size.width;
        dirty.size.height *= ctx->cell_size.height;
        ctx->cb.flush(nvc_ui_get_userdata(ctx), dirty);
    }
    return items;
}

static inline int nvc_ui_redraw_action_grid_resize(nvc_ui_context_t *ctx, int items) {
    if (likely(items-- > 0)) {
        int narg = nvc_rpc_read_array_size(&ctx->rpc);
        if (likely(narg >= 3)) {
            narg -= 3;
            int grid_id = nvc_rpc_read_int(&ctx->rpc);
            int width = nvc_rpc_read_int(&ctx->rpc);
            int height = nvc_rpc_read_int(&ctx->rpc);
            NVLogD("nvc ui grid resize %d - %dx%d", grid_id, width, height);
            nvc_lock_guard_t guard(ctx->locker);
            nvc_ui_grid *grid;
            const auto& p = ctx->grids.find(grid_id);
            if (p == ctx->grids.end()) {
                grid = new nvc_ui_grid(width, height);
                ctx->grids[grid_id] = grid;
            } else {
                grid = p->second;
                grid->resize(width, height);
            }
        }
        nvc_rpc_read_skip_items(&ctx->rpc, narg);
    }
    return items;
}

static inline int nvc_ui_redraw_action_default_colors_set(nvc_ui_context_t *ctx, int items) {
    if (likely(items-- > 0)) {
        int narg = nvc_rpc_read_array_size(&ctx->rpc);
        if (likely(narg >= 3)) {
            narg -= 3;
            ctx->default_colors[nvc_ui_color_code_foreground] = nvc_rpc_read_uint32(&ctx->rpc);
            ctx->default_colors[nvc_ui_color_code_background] = nvc_rpc_read_uint32(&ctx->rpc);
            ctx->default_colors[nvc_ui_color_code_special] = nvc_rpc_read_uint32(&ctx->rpc);
            ctx->cb.update_background(nvc_ui_get_userdata(ctx), ctx->default_colors[nvc_ui_color_code_background]);
        }
        nvc_rpc_read_skip_items(&ctx->rpc, narg);
    }
    return items;
}

static inline int nvc_ui_redraw_action_hl_attr_define(nvc_ui_context_t *ctx, int items) {
    while (items-- > 0) {
        int narg = nvc_rpc_read_array_size(&ctx->rpc);
        if (likely(narg >= 2)) {
            narg -= 2;
            int hl_id = nvc_rpc_read_int(&ctx->rpc);
            nvc_ui_colors_set_t colors;
            for (int mn = nvc_rpc_read_map_size(&ctx->rpc); mn > 0; mn--) {
                const auto& key = nvc_rpc_read_str(&ctx->rpc);
                const auto& p = nvc_ui_color_name_table.find(key);
                if (unlikely(p != nvc_ui_color_name_table.end())) {
                    colors[p->second] = nvc_rpc_read_uint32(&ctx->rpc);
                } else {
                    NVLogW("nvc ui invalid hl attr %s", key.c_str());
                    nvc_rpc_read_skip_items(&ctx->rpc, 1);
                }
            }
            ctx->hl_attrs[hl_id] = colors;
        }
        nvc_rpc_read_skip_items(&ctx->rpc, narg);
    }
    return 0;
}

typedef void (*nvc_ui_hl_attrs_notification)(nvc_ui_context_t *ctx, const nvc_ui_colors_set_t &attrs);

static inline void nvc_ui_hl_attrs_notification_TabLineFill(nvc_ui_context_t *ctx, const nvc_ui_colors_set_t &attrs) {
    const auto& p = attrs.find(nvc_ui_color_code_background);
    if (p != attrs.end()) {
        ctx->cb.update_tab_background(nvc_ui_get_userdata(ctx), p->second);
    }
}

#define NVC_UI_HL_ATTRS_NOTIFICATION(_action)    { #_action, nvc_ui_hl_attrs_notification_##_action }
static const std::map<const std::string, nvc_ui_hl_attrs_notification> nvc_ui_hl_groups_notification = {
    NVC_UI_HL_ATTRS_NOTIFICATION(TabLineFill),
};

static inline void nvc_ui_notify_hl_attrs(nvc_ui_context_t *ctx, const std::string& name) {
    const auto& notification = nvc_ui_hl_groups_notification.find(name);
    if (notification != nvc_ui_hl_groups_notification.end()) {
        const auto& p = ctx->hl_groups.find(name);
        if (likely(p != ctx->hl_groups.end())) {
            const auto& q = ctx->hl_attrs.find(p->second);
            if (likely(q != ctx->hl_attrs.end())) {
                notification->second(ctx, q->second);
            }
        }
    }
}

static inline int nvc_ui_redraw_action_hl_group_set(nvc_ui_context_t *ctx, int items) {
    while (items-- > 0) {
        int narg = nvc_rpc_read_array_size(&ctx->rpc);
        if (likely(narg >= 2)) {
            narg -= 2;
            const auto& name = nvc_rpc_read_str(&ctx->rpc);
            ctx->hl_groups[name] = nvc_rpc_read_int(&ctx->rpc);
            nvc_ui_notify_hl_attrs(ctx, name);
        }
        nvc_rpc_read_skip_items(&ctx->rpc, narg);
    }
    return 0;
}

static inline int nvc_ui_redraw_action_grid_line(nvc_ui_context_t *ctx, int items) {
    int last_grid_id = -1;
    nvc_ui_grid *grid = nullptr;
    nvc_lock_guard_t guard(ctx->locker);
    while (items-- > 0) {
        int narg = nvc_rpc_read_array_size(&ctx->rpc);
        if (likely(narg > 4)) {
            narg -= 4;
            int grid_id = nvc_rpc_read_int(&ctx->rpc);
            int row = nvc_rpc_read_int(&ctx->rpc);
            int col_start = nvc_rpc_read_int(&ctx->rpc);
            int cells = nvc_rpc_read_array_size(&ctx->rpc);
            if (last_grid_id != grid_id || grid == nullptr) {
                last_grid_id = grid_id;
                const auto& p = ctx->grids.find(grid_id);
                if (likely(p != ctx->grids.end())) {
                    grid = p->second;
                }
            }
            if (likely(grid != nullptr)) {
                int last_hl_id = 0;
                int offset = col_start;
                while (cells-- > 0) {
                    int cnum = nvc_rpc_read_array_size(&ctx->rpc);
                    if (likely(cnum-- > 0)) {
                        uint32_t len = 0;
                        const uint8_t *str = (const uint8_t *)nvc_rpc_read_str(&ctx->rpc, &len);
                        UniChar ch = nvc_ui_utf82unicode(str, len);
                        int repeat = 1;
                        if (cnum-- > 0) {
                            int hl_id = nvc_rpc_read_int(&ctx->rpc);
                            if (hl_id != last_hl_id) {
                                last_hl_id = hl_id;
                            }
                        }
                        if (cnum-- > 0) {
                            repeat = nvc_rpc_read_int(&ctx->rpc);
                        }
                        grid->update(row, offset, repeat, ch, nvc_ui_ch2glyph(ctx, ch), last_hl_id);
                        offset += repeat;
                    }
                    nvc_rpc_read_skip_items(&ctx->rpc, cnum);
                }
                nvc_ui_update_dirty(ctx, col_start, row, offset - col_start, 1);
            }
            nvc_rpc_read_skip_items(&ctx->rpc, cells);
        }
        nvc_rpc_read_skip_items(&ctx->rpc, narg);
    }
    return 0;
}

static inline int nvc_ui_redraw_action_grid_clear(nvc_ui_context_t *ctx, int items) {
    if (likely(items-- > 0)) {
        int narg = nvc_rpc_read_array_size(&ctx->rpc);
        if (likely(narg-- > 0)) {
            int grid_id = nvc_rpc_read_int(&ctx->rpc);
            nvc_lock_guard_t guard(ctx->locker);
            const auto& p = ctx->grids.find(grid_id);
            if (likely(p != ctx->grids.end())) {
                const auto& grid = p->second;
                grid->clear();
                nvc_ui_update_dirty(ctx, 0, 0, grid->width(), grid->height());
            }
        }
        nvc_rpc_read_skip_items(&ctx->rpc, narg);
    }
    return items;
}

static inline int nvc_ui_redraw_action_grid_destroy(nvc_ui_context_t *ctx, int items) {
    if (likely(items-- > 0)) {
        int narg = nvc_rpc_read_array_size(&ctx->rpc);
        if (likely(narg-- > 0)) {
            int grid_id = nvc_rpc_read_int(&ctx->rpc);
            NVLogD("nvc ui grid %d destroy", grid_id);
            nvc_lock_guard_t guard(ctx->locker);
            const auto& p = ctx->grids.find(grid_id);
            if (likely(p != ctx->grids.end())) {
                const auto& grid = p->second;
                ctx->grids.erase(p);
                nvc_ui_update_dirty(ctx, 0, 0, grid->width(), grid->height());
                delete grid;
            }
        }
        nvc_rpc_read_skip_items(&ctx->rpc, narg);
    }
    return items;
}

static inline int nvc_ui_redraw_action_grid_cursor_goto(nvc_ui_context_t *ctx, int items) {
    if (likely(items-- > 0)) {
        int narg = nvc_rpc_read_array_size(&ctx->rpc);
        if (likely(narg >= 3)) {
            narg -= 3;
            int grid_id = nvc_rpc_read_int(&ctx->rpc);
            int row = nvc_rpc_read_int(&ctx->rpc);
            int column = nvc_rpc_read_int(&ctx->rpc);
            nvc_lock_guard_t guard(ctx->locker);
            const auto& p = ctx->grids.find(grid_id);
            if (likely(p != ctx->grids.end())) {
                const auto& grid = p->second;
                if (grid->has_cursor()) {
                    nvc_ui_update_dirty(ctx, grid->cursor().x, grid->cursor().y, 1, 1);
                }
                grid->update_cursor(column, row);
                nvc_ui_update_dirty(ctx, column, row, 1, 1);
            }
        }
        nvc_rpc_read_skip_items(&ctx->rpc, narg);
    }
    return items;
}

static inline int nvc_ui_redraw_action_grid_scroll(nvc_ui_context_t *ctx, int items) {
    if (likely(items-- > 0)) {
        int narg = nvc_rpc_read_array_size(&ctx->rpc);
        if (likely(narg >= 6)) {
            narg -= 6;
            int grid_id = nvc_rpc_read_int(&ctx->rpc);
            int top = nvc_rpc_read_int(&ctx->rpc);
            int bot = nvc_rpc_read_int(&ctx->rpc);
            int left = nvc_rpc_read_int(&ctx->rpc);
            int right = nvc_rpc_read_int(&ctx->rpc);
            int rows = nvc_rpc_read_int(&ctx->rpc);
            nvc_lock_guard_t guard(ctx->locker);
            const auto& p = ctx->grids.find(grid_id);
            if (likely(p != ctx->grids.end())) {
                const auto& dirty = p->second->scroll(top, bot, left, right, rows);
                nvc_ui_update_dirty(ctx, dirty.x, dirty.y, dirty.width, dirty.height);
            }
        }
        nvc_rpc_read_skip_items(&ctx->rpc, narg);
    }
    return items;
}

static inline int nvc_ui_redraw_action_tabline_update(nvc_ui_context_t *ctx, int items) {
    if (likely(items-- > 0)) {
        int narg = nvc_rpc_read_array_size(&ctx->rpc);
        if (likely(narg >= 2)) {
            narg -= 2;
            nvc_rpc_object_handler_t handler = nvc_rpc_read_ext_handle(&ctx->rpc);
            nvc_ui_tab_list_t tabs;
            int tn = nvc_rpc_read_array_size(&ctx->rpc);
            while (tn-- > 0) {
                nvc_ui_tab_t tab;
                int mn = nvc_rpc_read_map_size(&ctx->rpc);
                while (mn-- > 0) {
                    const auto& name = nvc_rpc_read_str(&ctx->rpc);
                    const auto& p = nvc_ui_set_ui_tab_actions.find(name);
                    if (likely(p != nvc_ui_set_ui_tab_actions.end())) {
                        p->second(tab, &ctx->rpc);
                    } else {
                        nvc_rpc_read_skip_items(&ctx->rpc, 1);
                        NVLogW("nvc ui tab info: %s", name.c_str());
                    }
                }
                tabs.push_back(tab);
            }
            ctx->current_tab = handler;
            bool list_updated = ctx->tabs != tabs;
            if (list_updated) {
                ctx->tabs = tabs;
            }
            ctx->cb.update_tab_list(nvc_ui_get_userdata(ctx), list_updated);
        }
        nvc_rpc_read_skip_items(&ctx->rpc, narg);
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
    NVC_REDRAW_ACTION_IGNORE(busy_start),
    NVC_REDRAW_ACTION_IGNORE(busy_stop),
    NVC_REDRAW_ACTION_IGNORE(suspend),
    NVC_REDRAW_ACTION_IGNORE(update_menu),
    NVC_REDRAW_ACTION_IGNORE(bell),
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
static inline int nvc_ui_notification_action_redraw(nvc_ui_context_t *ctx, int items) {
    while (items-- > 0) {
        int narg = nvc_rpc_read_array_size(&ctx->rpc);
        if (likely(narg-- > 0)) {
            const auto& action = nvc_rpc_read_str(&ctx->rpc);
            const auto& p = nvc_ui_redraw_actions.find(action);
            if (unlikely(p == nvc_ui_redraw_actions.end())) {
                NVLogW("nvc ui unknown redraw action: %s", action.c_str());
            } else if (p->second != nullptr) {
                //NVLogW("nvc ui redraw action: %s", action.c_str());
                narg = p->second(ctx, narg);
            }
            nvc_rpc_read_skip_items(&ctx->rpc, narg);
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
            nvc_rpc_read_skip_items(ctx, p->second(nvc_ui_to_context(ctx, rpc), nvc_rpc_read_array_size(ctx)));
        }
    }
    return items;
}

static int nvc_ui_close_handler(nvc_rpc_context_t *ctx, int items) {
    if (ctx != nullptr) {
        nvc_ui_context_t *ui_ctx = nvc_ui_to_context(ctx, rpc);
        ui_ctx->cb.close(nvc_ui_get_userdata(ui_ctx));
    }
    return items;
}
