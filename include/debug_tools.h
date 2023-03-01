/*	Copyright(C)
	Author: 479764650@qq.com
	Description: 元编程调试工具
	History: 2023/2/14
*/

#ifndef DEBUG_TOOLS_H
#define DEBUG_TOOLS_H

#include <iostream>

// 供编译期查看类型信息
template<typename, typename ...> struct Dump;

// 运行时打印.可以通过编译时的值计算，对字符串进行后续处理，产生一些有价值的应用
#if __GNUC__
template<typename ...Ts>
void PrintType()
{
	std::cout << _PRETTY_FUNCTION_ << std::endl;
}
#elif _MSC_VER
template<typename ...Ts>
void PrintType()
{
	std::cout << __FUNCSIG__ << std::endl;
}
#else
template<typename ...Ts>
void PrintType() {}
#endif

#endif // !DEBUG_TOOLS_H


