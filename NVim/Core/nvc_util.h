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
    
    inline explicit UIPoint() : x(0), y(0) {}
    inline explicit UIPoint(int32_t _x, int32_t _y) : x(_x), y(_y) {}
    inline bool operator==(const UIPoint& rhs) const { return x == rhs.x && y == rhs.y; }
    inline bool operator!=(const UIPoint& rhs) const { return x != rhs.x || y != rhs.y; }
};

struct UISize {
    int32_t width;
    int32_t height;
    
    inline explicit UISize() : width(0), height(0) {}
    inline explicit UISize(int32_t _w, int32_t _h) : width(_w), height(_h) {}
    inline int32_t area(void) const { return width * height; }
    inline bool empty(void) const { return width <= 0 && height <= 0; }
    inline bool contains(const UIPoint& pt) const { return pt.x >= 0 && pt.y >= 0 && pt.x < width && pt.y < height; }
    inline bool operator==(const UISize& rhs) const { return width == rhs.width && height == rhs.height; }
    inline bool operator!=(const UISize& rhs) const { return width != rhs.width || height != rhs.height; }
};

struct UIRect {
    UIPoint origin;
    UISize size;

    inline explicit UIRect() = default;
    inline explicit UIRect(const UIPoint& _origin, const UISize& _size) : origin(_origin), size(_size) {}
    inline explicit UIRect(int32_t x, int32_t y, int32_t width, int32_t height) : origin(x, y), size(width, height) {}
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
inline constexpr const auto ranges_to_view = [] (const std::ranges::subrange<const char *> &sr) {
    std::string_view    view(sr.begin(), sr.size());
    ssize_t begin = view.find_first_not_of(' ');
    if (begin == std::string_view::npos) {
        view.remove_prefix(view.size());
    } else {
        view.remove_prefix(begin);
    }
    ssize_t end = view.find_last_not_of(' ');
    if (end != std::string_view::npos) {
        view.remove_suffix(view.size() - end - 1);
    }
    return view;
};

inline constexpr const auto split_token = [] (const std::string_view &input, char delimiter) {
    return std::ranges::split_view(input, delimiter) | std::views::transform(ranges_to_view) | std::views::filter(std::not_fn(std::mem_fn(&std::string_view::empty)));
};

}
#endif

#ifdef __cplusplus
}
#endif

#endif /* __NVC_UTIL_H__ */
