#include "TypeList.h"
#include "Graph.h"
#include <tuple>
#include <variant>
#include <type_traits>
#include <iostream>

template<typename T>
using SizeLess4 = std::bool_constant<(sizeof(T) < 4)>;

template<typename Acc, typename E>
using TypeSizeAcc = std::integral_constant<size_t, Acc::value + sizeof(E)>;

template<typename L, typename R>
using sizeCmp = std::bool_constant<(sizeof(L) < sizeof(R))>;


// 点实例
using A = Node<'A'>;
using B = Node<'B'>;
using C = Node<'C'>;
using D = Node<'D'>;
using E = Node<'E'>;

//图定义
using g = Graph<
	LINK(NODE(A)->NODE(B)->NODE(C)->NODE(D)),
	LINK(NODE(A)->NODE(C)),  // test shortest path: A -> C -> D
	LINK(NODE(B)->NODE(A)),  // test cycle
	LINK(NODE(A)->NODE(E))>; // test D -> E unreachable

int main() {
	using Alist = TypeList<int, char>;
	static_assert(TL<Alist>);
	static_assert(Alist::size == 2);
	static_assert(std::is_same_v<Alist::append<double>, TypeList<int, char, double>>);
	static_assert(std::is_same_v<Alist::prepend<double>, TypeList<double, int, char>>);
	static_assert(std::is_same_v<Alist::exportTo<std::tuple>, std::tuple<int, char>>);
	static_assert(std::is_same_v<Alist::exportTo<std::variant>, std::variant<int, char>>);
	using LongList = TypeList<char, float, double, int, char>;
	static_assert(std::is_same_v<Map_t<LongList, std::add_pointer>,
		TypeList<char*, float*, double*, int*, char*>>);

	static_assert(std::is_same_v<Filter_t<LongList, SizeLess4>, TypeList<char, char>>);
	static_assert(Fold_t<LongList, std::integral_constant<size_t, 0>, TypeSizeAcc>::value == 18);
	static_assert(std::is_same_v<Concat_t<TypeList<int, char>, TypeList<float>>, TypeList<int, char, float>>);
	static_assert(Elem_v<LongList, char>);
	static_assert(std::is_same_v<Unique_t<LongList>, TypeList<char, float, double, int>>);
	static_assert(std::is_same_v<Partition_t<LongList, SizeLess4>::Satisfied, TypeList<char, char>>);
	static_assert(std::is_same_v<Partition_t<LongList, SizeLess4>::Rest, TypeList<float, double, int>>);
	static_assert(std::is_same_v<Sort_t<LongList, sizeCmp>, TypeList<char, char, float, int, double>>);

	static_assert(g::GetShortestPath('A', 'D').sz == 3);
	std::cout << g::PathFinder<A, E>::type::size << std::endl;
	using a = g::AllPairs;

	static_assert(std::is_same_v<g::PathFinder<A, D>::type, TypeList<A, C, D>>);
	static_assert(std::is_same_v<g::AllPairs,
		TypeList< std::pair<A, B>, std::pair<A, C>, std::pair<A, D>, std::pair<A, A>, std::pair<A, E>,
		std::pair<B, B>, std::pair<B, C>, std::pair<B, D>, std::pair<B, A>, std::pair<B, E>,
		std::pair<C, B>, std::pair<C, C>, std::pair<C, D>, std::pair<C, A>, std::pair<C, E>>>);

	static_assert(std::is_same_v<g::ReachableNodePairs,
		TypeList< std::pair<A, B>, std::pair<A, C>, std::pair<A, D>, std::pair<A, A>, std::pair<A, E>,
		std::pair<B, B>, std::pair<B, C>, std::pair<B, D>, std::pair<B, A>, std::pair<B, E>,
		std::pair<C, C>, std::pair<C, D>>>);
		
	static_assert(std::is_same_v<Chain_t<LINK(NODE(A)->NODE(B)->NODE(C)->NODE(D))>, TypeList<Edge<A, B>, Edge<B, C>, Edge<C, D>>>);
	//static_assert(std::is_same_v<g::Edges, TypeList<Edge<A, B>, Edge<B, C>, Edge<C, D>, Edge<A, C>, Edge<B, A>, Edge<A, E>>>);

}