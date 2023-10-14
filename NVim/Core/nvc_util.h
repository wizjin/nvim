//
//  nvc_util.h
//  NVim
//
//  Created by wizjin on 2023/10/14.
//

#ifndef __NVC_UTIL_H__
#define __NVC_UTIL_H__

#include <sstream>
#include <string>
#include <vector>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
namespace nvc {

static inline std::vector<std::string> split(const std::string &s, char delim) {
    std::vector<std::string> result;
    if (!s.empty()) {
        std::string item;
        std::stringstream ss(s);
        while (getline(ss, item, delim)) {
            result.push_back(item);
        }
    }
    return result;
}

}
#endif

#ifdef __cplusplus
}
#endif

#endif /* __NVC_UTIL_H__ */
