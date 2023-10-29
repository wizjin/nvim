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
#include <ranges>

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
    
    inline constexpr explicit UIPoint() : x(0), y(0) {}
    inline constexpr explicit UIPoint(int32_t _x, int32_t _y) : x(_x), y(_y) {}
    inline constexpr bool operator==(const UIPoint& rhs) const { return x == rhs.x && y == rhs.y; }
    inline constexpr bool operator!=(const UIPoint& rhs) const { return x != rhs.x || y != rhs.y; }
    inline constexpr UIPoint& operator-=(const UIPoint& rhs) {
        this->x -= rhs.x;
        this->y -= rhs.y;
        return *this;
    }
    
    static const UIPoint zero;
};

struct UISize {
    int32_t width;
    int32_t height;
    
    inline constexpr explicit UISize() : width(0), height(0) {}
    inline constexpr explicit UISize(int32_t _w, int32_t _h) : width(_w), height(_h) {}
    inline constexpr int32_t area(void) const { return width * height; }
    inline constexpr bool empty(void) const { return width <= 0 || height <= 0; }
    inline constexpr bool contains(const UIPoint& pt) const { return pt.x >= 0 && pt.y >= 0 && pt.x < width && pt.y < height; }
    inline constexpr bool operator==(const UISize& rhs) const { return width == rhs.width && height == rhs.height; }
    inline constexpr bool operator!=(const UISize& rhs) const { return width != rhs.width || height != rhs.height; }
    
    static const UISize zero;
};

struct UIRect {
    UIPoint origin;
    UISize size;

    inline constexpr explicit UIRect() {};
    inline constexpr explicit UIRect(const UIPoint& _origin, const UISize& _size) : origin(_origin), size(_size) {}
    inline constexpr explicit UIRect(int32_t x, int32_t y, int32_t width, int32_t height) : origin(x, y), size(width, height) {}
    inline constexpr int32_t x(void) const { return origin.x; }
    inline constexpr int32_t y(void) const { return origin.y; }
    inline constexpr int32_t width(void) const { return size.width; }
    inline constexpr int32_t height(void) const { return size.height; }
    inline constexpr int32_t left(void) const { return origin.x; }
    inline constexpr int32_t right(void) const { return origin.x + size.width; }
    inline constexpr int32_t top(void) const { return origin.y; }
    inline constexpr int32_t bottom(void) const { return origin.y + size.height; }
    inline constexpr bool empty(void) const { return size.empty(); }
    inline constexpr bool contains(const UIPoint& pt) const {
        return pt.x >= origin.x && pt.y >= origin.y && pt.x <= origin.x + size.width && pt.y <= origin.y + size.height;
    }

    inline constexpr bool operator==(const UIRect& rhs) const { return origin == rhs.origin && size == rhs.size; }
    inline constexpr bool operator!=(const UIRect& rhs) const { return origin != rhs.origin || size != rhs.size; }

    inline constexpr const UIRect& operator+=(const UIRect& rhs) {
        if (!rhs.empty()) {
            if (empty()) {
                *this = rhs;
            } else {
                int32_t left = std::min(this->left(), rhs.left());
                int32_t right = std::max(this->right(), rhs.right());
                int32_t top = std::min(this->top(), rhs.top());
                int32_t bottom = std::max(this->bottom(), rhs.bottom());
                this->origin.x = left;
                this->origin.y = top;
                this->size.width = right - left;
                this->size.height = bottom - top;
            }
        }
        return *this;
    }

    inline constexpr UIRect intersection(const UIRect& rhs) const {
        UIRect rc;
        if (!empty()) {
            int32_t left = std::max(this->left(), rhs.left());
            int32_t right = std::min(this->right(), rhs.right());
            int32_t top = std::max(this->top(), rhs.top());
            int32_t bottom = std::min(this->bottom(), rhs.bottom());
            int32_t width = right - left;
            int32_t height = bottom - top;
            if (width > 0 && height > 0) {
                rc.origin.x = left;
                rc.origin.y = top;
                rc.size.width = width;
                rc.size.height = height;
            }
        }
        return rc;
    }
    
    static const UIRect zero;
};

#pragma mark - Util Helper
inline constexpr const auto subrange_to_string_view = [] (const std::ranges::subrange<const char *> &sr) {
    std::string_view    view(sr.begin(), sr.size());
    ssize_t begin = view.find_first_not_of(' ');
    if (begin != std::string_view::npos) {
        view.remove_prefix(begin);
    } else {
        view.remove_prefix(view.size());
    }
    ssize_t end = view.find_last_not_of(' ');
    if (end != std::string_view::npos) {
        view.remove_suffix(view.size() - end - 1);
    }
    return view;
};

inline constexpr const auto split_token = [] (const std::string_view &input, char delimiter) {
    return std::ranges::split_view(input, delimiter)
            | std::views::transform(subrange_to_string_view)
            | std::views::filter(std::not_fn(std::mem_fn(&std::string_view::empty)));
};

}
#endif

#ifdef __cplusplus
}
#endif

#endif /* __NVC_UTIL_H__ */
