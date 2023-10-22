//
//  nvc_ui_render.h
//  NVim
//
//  Created by wizjin on 2023/10/19.
//

#ifndef __NVC_UI_RENDER_H__
#define __NVC_UI_RENDER_H__

#include "nvc_ui_context.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
namespace nvc {

class UIRender {
private:
    CGContextRef    m_context;
    UIContext       &m_ctx;
    UIFont          &m_font;
    UIMode          &m_mode;
    UIHLAttrGroups  &m_hl_attrs;
    const UIHLAttr  *m_last_hl;
    
    CGFloat         m_font_offset;
    CGFloat         m_one_pixel;
    
    int32_t         m_last_hl_id;

    ui_color_t      m_foreground;
    ui_color_t      m_background;
    ui_color_t      m_special;

    ui_color_t      m_text_color;    
    ui_color_t      m_fill_color;
    ui_color_t      m_stroke_color;

    UIRender& operator=(const UIRender&);
public:
    explicit UIRender(UIContext& ctx, CGContextRef context);
    
    inline UIContext& ctx(void) const { return m_ctx; }
    inline UIFont& font(void) const { return m_font; }
    inline CGFloat font_offset(void) const { return m_font_offset; }
    inline UIHLAttrGroups &hl_attrs(void) const { return m_hl_attrs; }
    inline const UIModeInfo *mode_info(void) const { return m_mode.info(); }
    inline bool need_cursor(void) const { return m_ctx.mode_enabled() && m_ctx.show_cursor(); }

    const UIHLAttr *update_hl_id(int32_t hl);
    
    inline ui_color_t stroke_color(void) const { return m_special; }
    inline ui_color_t text_color(void) const { return m_text_color; }
    inline void text_color(ui_color_t rgb) {
        m_text_color = rgb;
    }
    
    inline void set_fill_color(ui_color_t rgb) {
        if (m_fill_color != rgb) {
            m_fill_color = rgb;
            const uint8_t *c = (const uint8_t *)&rgb;
            CGContextSetRGBFillColor(m_context, c[2]/255.0, c[1]/255.0, c[0]/255.0, 1.0);
        }
    }

    inline void set_stroke_color(ui_color_t rgb) {
        if (m_stroke_color != rgb) {
            m_stroke_color = rgb;
            const uint8_t *c = (const uint8_t *)&rgb;
            CGContextSetRGBStrokeColor(m_context, c[2]/255.0, c[1]/255.0, c[0]/255.0, 1.0);
        }
    }
    
    inline CGFloat one_pixel(void) const { return m_one_pixel; }
    
    inline void line_width(CGFloat width) const {
        CGContextSetLineWidth(m_context, width);
    }
    
    inline void line_dash(CGFloat components[], uint8_t count) const {
        CGContextSetLineDash(m_context, 0, components, count);
    }
    
    inline void draw_background(const CGRect& rc) {
        if (m_last_hl_id != 0) {
            set_fill_color(m_background);
            CGContextFillRect(m_context, rc);
        }
    }
    
    inline void draw_rect(const CGRect& rc) const {
        CGContextFillRect(m_context, rc);
    }
    
    inline void draw_line(CGFloat x1, CGFloat y1, CGFloat x2, CGFloat y2) const {
        CGContextBeginPath(m_context);
        CGContextMoveToPoint(m_context, x1, y1);
        CGContextAddLineToPoint(m_context, x2, y2);
        CGContextStrokePath(m_context);
    }
    
    inline void draw_wavy_line(CGFloat x, CGFloat y, CGFloat w, CGFloat h, bool is_wide) const {
        CGContextBeginPath(m_context);
        CGContextMoveToPoint(m_context, x, y);
        CGContextAddCurveToPoint(m_context, x + w*0.25, y, x + w*0.25, y + h, x + w*0.5, y + h);
        CGContextAddCurveToPoint(m_context, x + w*0.75, y + h, x + w*0.75, y, x + w, y);
        if (is_wide) {
            x += w;
            CGContextAddCurveToPoint(m_context, x + w*0.25, y, x + w*0.25, y + h, x + w*0.5, y + h);
            CGContextAddCurveToPoint(m_context, x + w*0.75, y + h, x + w*0.75, y, x + w, y);
        }
        CGContextStrokePath(m_context);
    }
    
    inline void draw_glyph(CTFontRef font, CGGlyph glyph, const UIPoint& pt) {
        if (likely(font != nullptr)) {
            set_fill_color(m_text_color);
            CGContextSetTextPosition(m_context, pt.x, pt.y);
            CTFontDrawGlyphs(font, &glyph, &CGPointZero, 1, m_context);
        }
    }

};

}
#endif

#ifdef __cplusplus
}
#endif

#endif /* __NVC_UI_RENDER_H__ */
