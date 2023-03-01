/*	Copyright(C)
	Author: 479764650@qq.com
	Description: TypeList及对应高阶函数操作
	History: 2023/2/8
*/


#ifndef TYPE_LIST_H
#define TYPE_LIST_H

#include <type_traits>

template <typename ...Ts>
struct TypeList {
	// 用于定义concept
	struct IsTypeList {};
	using type = TypeList;

	constexpr static size_t size = sizeof...(Ts);    

	//using代替模板类的方式，可以减少编译器需要实例化的类型数量
	template <typename ...T> using append = TypeList<Ts..., T...>;
	template <typename ...T> using prepend = TypeList<T..., Ts...>;
	template <template<typename ...> typename T> using exportTo = T<Ts...>;
};

template<typename TypeList>
concept TL = requires {
	typename TypeList::IsTypeList;
	typename TypeList::type;
};


// Map
template<TL In, template<typename> class Func>
struct Map {};
template<template<typename> class Func, typename ...Ts>
struct Map<TypeList<Ts...>, Func> : TypeList<typename Func<Ts>::type...> {};

template<TL In, template<typename> class Func>
using Map_t = typename Map<In, Func>::type;

// Filter
template<TL In, template <typename> class P, TL Out = TypeList<>>
struct Filter : Out {};

template<template <typename> class P, TL OUT, typename H, typename ...Ts>
struct Filter<TypeList<H, Ts...>, P, OUT> :
	std::conditional_t<P<H>::value,
	Filter<TypeList<Ts...>, P, typename OUT::template append<H>>,
	Filter<TypeList<Ts...>, P, OUT>> { };

template<TL IN, template <typename> class P>
using Filter_t = typename Filter<IN, P>::type;

// Fold
template<typename T>
struct Return {
	using type = T;
};

template<TL In, typename Init, template<typename, typename> class Op>
struct Fold : Return<Init> {};

template<TL In, typename Init, template<typename, typename> class Op>
using Fold_t = typename Fold<In, Init, Op>::type;

template<typename Acc, template<typename, typename> class Op, typename H, typename ...Ts>
struct Fold<TypeList<H, Ts...>, Acc, Op> :
	Fold<TypeList<Ts...>, typename Op<Acc, H>::type, Op> {};

// 连接TL
/*实现1
template<TL In, TL In2>
class Concat {
	template<typename ACC, typename E>
	struct Append : ACC::template append<E> {};
public:
	using type = Fold_t<In2, In, Append>;
};
*/

/*实现2
template<TL In, TL In2>
struct Concat : In2::template exportTo<IN::template append> { };
*/

template<TL ...In>
struct Concat;

template<TL ...In>
using Concat_t = typename Concat<In...>::type;

template<> struct Concat<> : TypeList<> {};
template<TL In, TL In2, TL ...Rest>
struct Concat<In, In2, Rest...> : Concat<Concat_t<In, In2>, Rest...> {};

template<typename ...Ts, typename ...Ts1>
struct Concat<TypeList<Ts...>, TypeList<Ts1...>> : TypeList<Ts..., Ts1...> {};



// 判断类型是否存在
/*
template<TL In, typename E>
class Elem {
	template<typename Acc, typename T>
	using FindE = std::conditional<Acc::value, Acc, std::is_same<T, E>>;
	using Found = Fold_t<In, std::false_type, FindE>;
public:
	constexpr static bool value = Found::value;
};
*/

template<typename In, typename E>
struct Elem : std::false_type {};

template<typename E, typename ...Ts>
struct Elem<TypeList<Ts...>, E> :
	std::bool_constant<(false || ... || std::is_same_v<E, Ts>) > {};;

template<TL In, typename E>
constexpr bool Elem_v = Elem<In, E>::value;

// 去重
template<TL In>
class Unique
{
	template<typename Acc, typename E>
	using Append = std::conditional < Elem_v<Acc, E>, Acc, typename Acc::template append<E>>;
public:
	using type = Fold_t<In, TypeList<>, Append>;
};

template<TL In>
using Unique_t = Unique<In>::type;

// 分区
template<TL In, template<typename> typename P>
class Partition {
	template<typename E>
	using NotP = std::bool_constant<!P<E>::value>;
public:
	struct type
	{
		using Satisfied = Filter_t<In, P>;
		using Rest = Filter_t<In, NotP>;
	};
};

template<TL In, template<typename> typename P>
using Partition_t = Partition<In, P>::type;

// 快速排序
template<TL In, template<typename, typename> class Cmp>
struct Sort : TypeList<> {};

template<template<typename, typename> class Cmp,
	typename Pivot, typename ...Ts>
	class Sort<TypeList<Pivot, Ts...>, Cmp> {
	template<typename E> using LT = Cmp<E, Pivot>;
	using P = Partition_t<TypeList<Ts...>, LT>;
	using SmallerSorted = typename Sort<typename P::Satisfied, Cmp>::type;
	using BiggerSorted = typename Sort<typename P::Rest, Cmp>::type;
	public:
		using type = Concat_t<typename SmallerSorted::template append<Pivot>, BiggerSorted>;
};

template<TL In, template<typename, typename> class Cmp>
using Sort_t = typename Sort<In, Cmp>::type;

#endif // !TYPE_LIST_H