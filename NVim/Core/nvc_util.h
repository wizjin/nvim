//
//  nvc_util.h
//  NVim
//
//  Created by wizjin on 2023/10/14.
//

#ifndef __NVC_UTIL_H__
#define __NVC_UTIL_H__

#include <CoreText/CoreText.h>
#include <string>
#include <iterator>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus

#define nvc_lock_guard_t        std::lock_guard<std::mutex>

namespace nvc {

#pragma mark - UI Helper
struct UIPoint {
    int32_t x;
    int32_t y;
    
    inline UIPoint() : x(0), y(0) {}
    inline UIPoint(int32_t _x, int32_t _y) : x(_x), y(_y) {}
    inline bool operator==(const UIPoint& rhs) const { return x == rhs.x && y == rhs.y; }
    inline bool operator!=(const UIPoint& rhs) const { return x != rhs.x || y != rhs.y; }
};

struct UISize {
    int32_t width;
    int32_t height;
    
    inline UISize() : width(0), height(0) {}
    inline UISize(int32_t _w, int32_t _h) : width(_w), height(_h) {}
    inline int32_t area(void) const { return width * height; }
    inline bool empty(void) const { return width <= 0 && height <= 0; }
    inline bool contains(const UIPoint& pt) const { return pt.x >= 0 && pt.y >= 0 && pt.x < width && pt.y < height; }
    inline bool operator==(const UISize& rhs) const { return width == rhs.width && height == rhs.height; }
    inline bool operator!=(const UISize& rhs) const { return width != rhs.width || height != rhs.height; }
};

struct UIRect {
    UIPoint origin;
    UISize size;
    
    inline UIRect() {}
    inline UIRect(const UIPoint& _origin, const UISize& _size) : origin(_origin), size(_size) {}
    inline UIRect(int32_t x, int32_t y, int32_t width, int32_t height) : origin(x, y), size(width, height) {}
    inline int32_t x(void) const { return origin.x; }
    inline int32_t y(void) const { return origin.y; }
    inline int32_t width(void) const { return size.width; }
    inline int32_t height(void) const { return size.height; }
    inline int32_t left(void) const { return origin.x; }
    inline int32_t right(void) const { return origin.x + size.width; }
    inline int32_t top(void) const { return origin.y; }
    inline int32_t bottom(void) const { return origin.y + size.height; }
    inline bool empty(void) const { return size.empty(); }
    inline bool contains(const UIPoint& pt) const {
        return pt.x >= origin.x && pt.y >= origin.y && pt.x <= origin.x + size.width && pt.y <= origin.y + size.height;
    }
    inline bool operator==(const UIRect& rhs) const { return origin == rhs.origin && size == rhs.size; }
    inline bool operator!=(const UIRect& rhs) const { return origin != rhs.origin || size != rhs.size; }
};

#pragma mark - Util Helper

typedef uint32_t UnicodeChar;

class token_spliter {
public:
    using value_type    = std::string_view;
    
    class iterator {
    public:
        using iterator_category = std::input_iterator_tag;

        inline iterator(const token_spliter& spliter, ssize_t pos) : m_spliter(spliter), m_pos(pos) {
            m_end = m_spliter.m_data.find_first_of(m_spliter.m_delimiter, m_pos);
            check_value();
        }
        
        inline bool operator== (const iterator& rhs) const { return m_pos == rhs.m_pos && &m_spliter == &rhs.m_spliter; }
        inline bool operator!= (const iterator& rhs) const { return m_pos != rhs.m_pos || &m_spliter != &rhs.m_spliter; }
        
        inline iterator& operator++() {
            if (m_pos != value_type::npos) {
                m_pos = m_end;
                check_value();
            }
            return *this;
        }

        inline const value_type& operator*() const { return m_value; }
    private:
        inline void check_value(void) {
            while(m_pos != value_type::npos) {
                const auto& data = m_spliter.m_data;
                m_value = data.substr(m_pos, m_end != value_type::npos ? m_end - m_pos : value_type::npos);
                if (!m_value.empty()) {
                    while (m_value.starts_with(' ')) m_value.remove_prefix(1);
                    while (m_value.ends_with(' ')) m_value.remove_suffix(1);
                    if (!m_value.empty()) break;
                }
                m_pos = m_end;
                if (m_pos != value_type::npos) {
                    m_end = m_spliter.m_data.find_first_of(m_spliter.m_delimiter, ++m_pos);
                }
            }
        }

        const token_spliter&    m_spliter;
        value_type              m_value;
        ssize_t                 m_pos;
        ssize_t                 m_end;
    };

private:
    const value_type&       m_data;
    char                    m_delimiter;
public:
    inline token_spliter(const value_type& data, char delimiter) : m_data(data), m_delimiter(delimiter) {}
    
    inline iterator begin() const { return iterator(*this, 0); }
    inline iterator end() const { return iterator(*this, std::string_view::npos); }

};

}
#endif

#ifdef __cplusplus
}
#endif

#endif /* __NVC_UTIL_H__ */
