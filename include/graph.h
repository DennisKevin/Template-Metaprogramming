/*	Copyright(C)
	Author: 479764650@qq.com
	Description: 静态图获取最短路径,提供运行期接口
	History: 2023/2/8
*/

#ifndef GRAPH_H
#define GRAPH_H

#include <type_traits>
#include <utility>
#include "type_list.h"

// 顶点
template<char Id>
struct Node { constexpr static char id = Id; };

template<typename Node>
concept Vertex = requires {
	Node::id;
};

// 边及萃取
template<Vertex F, Vertex T>
struct Edge
{
	using From = F;
	using To = T;
};

template<typename Node = void>
	requires (Vertex<Node> || std::is_void_v<Node>)
struct EdgeTrait {
	template<typename Edge>
	using IsFrom = std::is_same<typename Edge::From, Node>;
	template<typename Edge>
	using IsTo = std::is_same<typename Edge::To, Node>;
	template<typename Edge>
	using GetFrom = Return<typename Edge::From>;
	template<typename Edge>
	using GetTo = Return<typename Edge::To>;
};

// 链及模板解析
#define NODE(node_name) auto(*)(node_name)
#define LINK(link) link->void

template<typename Link, TL Out = TypeList<>>
struct Chain;

template<Vertex F, TL Out>
struct Chain<auto(*)(F)->void, Out>
{
	using From = F;
	using type = Out;
};

template<Vertex F, typename T, TL Out>
class Chain<auto(*)(F)->T, Out>
{
	using To = typename Chain<T, Out>::From;
public:
	using From = F;
	using type = typename Chain<T, typename Out::template append<Edge<From, To>>>::type;
};

template<typename Link>
using Chain_t = typename Chain<Link>::type;

// 存储路径结构体
template<typename NodeType>
struct PathRef {
	const NodeType* path;
	size_t sz;
};

// 静态图
template<typename ...Chains>
class Graph {
public:
	template<typename NodeType>
	constexpr static PathRef<NodeType> GetShortestPath(NodeType from, NodeType to)
	{
		PathRef<NodeType> result{};
		matchPath(from, to, result, AllSavedPaths{});
		return result;
	}

public:
	using Edges = Unique_t<Concat_t<Chain_t<Chains>...>>;

	// 最短路径搜索
	template<Vertex From, Vertex Target, TL Path = TypeList<>>
	// 无required时需用分支技巧判断是否有环，参数名不重要时，可以写完typename = void
	// template<Vertex From, Vertex Target, TL Path = TypeList<>, typename = void> 
	struct PathFinder;

	template<Vertex Target, TL Path>
	struct PathFinder<Target, Target, Path> :
		Path::template append<Target> {};

	template<Vertex CurNode, Vertex Target, TL Path>
		requires(Elem_v<Path, CurNode>)
	struct PathFinder<CurNode, Target, Path> : TypeList<> {};

	template<Vertex CurNode, Vertex Target, TL Path>
	struct PathFinder
	{
	private:
		using EdgesFrom = Filter_t<Edges, EdgeTrait<CurNode>::template IsFrom>;
		using NextNodes = Map_t<EdgesFrom, EdgeTrait<>::GetTo>;

		template<Vertex AdjacentNode>
		using GetPath = PathFinder<AdjacentNode, Target, typename Path::template append<CurNode>>;

		using AllPaths = Map_t<NextNodes, GetPath>;
		template<TL MinPath, TL Path_>
		using GetMinPath = std::conditional_t<MinPath::size == 0 ||
			(MinPath::size > Path_::size && Path_::size > 0), Path_, MinPath>;
	public:
		using type = Fold_t<AllPaths, TypeList<>, GetMinPath>;
	};

	template<Vertex From, Vertex Target>
	using PathFinder_t = typename PathFinder<From, Target>::type;

	// 笛卡尔积获取所有点对
	template<TL A, TL B, template<typename, typename> class Pair>
	struct CrossProduct
	{
		template<TL OuterResult, typename ElemA>
		struct OuterAppend
		{
			template<TL InnerResult, typename ElemB>
			using InnerAppend = typename InnerResult::template append<Pair<ElemA, ElemB>>;
			using type = Fold_t<B, OuterResult, InnerAppend>;
		};
	public:
		using type = Fold_t<A, TypeList<>, OuterAppend>;
	};

	template<TL A, TL B, template<typename, typename> class Pair>
	using CrossProduct_t = typename CrossProduct<A, B, Pair>::type;

	using AllPairs = CrossProduct_t<
		Unique_t<Map_t<Edges, EdgeTrait<>::GetFrom>>,
		Unique_t<Map_t<Edges, EdgeTrait<>::GetTo>>,
		std::pair>;
	template<typename NodePair>
	using IsNonEmptyPath = std::bool_constant<(
		PathFinder_t<typename NodePair::first_type, typename NodePair::second_type>::size > 0)>;
	// 可达节点
	using ReachableNodePairs = Filter_t<AllPairs, IsNonEmptyPath>;
	template<Vertex Node, Vertex ...Nodes>
	class PathStorage {
		using NodeType = std::decay_t<decltype(Node::id)>;
		constexpr static NodeType pathStorage[]{ Node::id, Nodes::id... };
	public:
		constexpr static PathRef<NodeType> path{
			.path = pathStorage,
			.sz = sizeof...(Nodes) + 1
		};
	};

	template<typename NodePair>
	using PathData = Return<std::pair<NodePair,
		typename PathFinder_t<typename NodePair::first_type, typename NodePair::second_type>::template exportTo<PathStorage>>>;
	using AllSavedPaths = Map_t<ReachableNodePairs, PathData>;

	template<typename NodeType, typename From, typename Target, typename PathStorage_>
	constexpr static bool matchPath(NodeType from, NodeType to,
		PathRef<NodeType>& result, std::pair<std::pair<From, Target>, PathStorage_>)
	{
		if (From::id == from && Target::id == to) {
			result = PathStorage_::path;
			return true;
		}
		return false;
	}

	template<typename NodeType, typename ...PathPairs>
	constexpr static void matchPath(NodeType from, NodeType to,
		PathRef<NodeType>& result, TypeList<PathPairs...>)
	{
		(matchPath(from, to, result, PathPairs{}) || ...);
	}
};

#endif // !GRAPH_H