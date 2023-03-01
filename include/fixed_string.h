/*	Copyright(C)
	Author: 479764650@qq.com
	Description: 地址固定的字面量,解决C++20非类型模板参数对字符串的支持
	History: 2023/2/26
*/

#ifndef FIXED_STRING_H
#define FIXED_STRING_H

#include <algorithm>

template<size_t N>
struct FixedString
{
	char str[N];
	// FixString对象将拥有静态存储期,同一个字面对象在程序中仅有一个实例
	constexpr FixedString(const char(&s)[N]) { std::copy_n(s, N, str); }
};

template<FixedString STR>
constexpr decltype(STR) operator""_fs() { return STR; }

#endif // !FIXED_STRING_H

