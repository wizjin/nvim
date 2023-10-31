//
//  nvc_ui_render.cpp
//  NVim
//
//  Created by wizjin on 2023/10/19.
//

#include "nvc_ui_render.h"

namespace nvc {

UIRender::UIRender(UIContext& ctx, CGContextRef context)
: m_ctx(ctx), m_context(context), m_font(ctx.font()), m_mode(ctx.mode()), m_hl_attrs(ctx.hl_attrs()),
  m_font_offset(ctx.font().glyph_size().height - ctx.font().font_offset()),
  m_last_hl(nullptr),
  m_last_hl_id(0),
  m_one_pixel(1.0/ctx.font().scale_factor()),
  m_foreground(m_hl_attrs.default_foreground()),
  m_background(m_hl_attrs.default_background()),
  m_special(m_hl_attrs.default_special()),
  m_text_color(m_foreground),
  m_fill_color(-1),
  m_stroke_color(-1) {

}

const UIHLAttr *UIRender::update_hl_id(int32_t hl) {
    if (m_last_hl_id != hl) {
        m_last_hl_id = hl;
        m_last_hl = m_hl_attrs.find_hl_attr(m_last_hl_id);
        if (m_last_hl == nullptr) {
            m_foreground = m_hl_attrs.default_foreground();
            m_background = m_hl_attrs.default_background();
            m_special = m_hl_attrs.default_special();
        } else {
            m_foreground = m_last_hl->has_foreground ? m_last_hl->foreground : m_hl_attrs.default_foreground();
            m_background = m_last_hl->has_background ? m_last_hl->background : m_hl_attrs.default_background();
            m_special = m_last_hl->has_special ? m_last_hl->special : m_hl_attrs.default_special();
            if (m_last_hl->reverse) std::swap(m_foreground, m_background);
        }
    }
    m_text_color = m_foreground;
    return m_last_hl;
}

}
