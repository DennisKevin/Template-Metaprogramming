/*	Copyright(C)
	Author: 479764650@qq.com
	Description: KV数据表
	History: 2023/2/13
*/

#ifndef DATA_TABLE_H
#define DATA_TABLE_H

#include <algorithm>
#include <bitset>
#include <type_traits>
#include <tuple>
#include "type_list.h"

// 记录
template<auto Key, typename ValueType, size_t Dim = 1>
struct Entry {
	constexpr static auto key = Key;
	constexpr static size_t dim = Dim;
	constexpr static bool isArray = Dim > 1;
	using type = ValueType;
};

template<auto Key, typename ValueType, size_t Dim>
struct Entry<Key, ValueType[Dim]> : Entry<Key, ValueType, Dim> {};

template<typename E>
concept KVEntry = requires {
	typename E::type;
	requires std::is_standard_layout_v<typename E::type>;
	requires std::is_trivial_v<typename E::type>;
	{E::key} -> std::convertible_to<size_t>;
	{E::dim} -> std::convertible_to<size_t>;
};

// 记录分组器
template<TL Entries = TypeList<>, TL GroupedEntries = TypeList<>>
struct GroupEntriesTrait : GroupedEntries {};

template<KVEntry HeadEntry, KVEntry ...RestEntries, TL GroupedEntries>
class GroupEntriesTrait<TypeList<HeadEntry, RestEntries...>, GroupedEntries>
{
private:
	template<KVEntry _Entry>
	using GroupPred = std::bool_constant<(HeadEntry::dim == _Entry::dim &&
		sizeof(typename HeadEntry::type) == sizeof(typename _Entry::type) &&
		alignof(typename HeadEntry::type) == alignof(typename _Entry::type))>;
	using Group = Partition_t<TypeList<HeadEntry, RestEntries...>, GroupPred>;
	using Satisfied = typename Group::Satisfied;
	using Rest = typename Group::Rest;
public:
	using type = typename GroupEntriesTrait<Rest,
		typename GroupedEntries::template append<Satisfied>>::type;
};

template<TL Entries, TL GroupedEntries = TypeList<>>
using GroupEntriesTrait_t = GroupEntriesTrait<Entries, GroupedEntries>::type;

// 存储区域:储存GroupEntriesTrait分好组的每一组KV表
template<KVEntry HeadEntry, KVEntry ...TailEntries>
class GenericRegion {
private:
	constexpr static size_t entriesNum = sizeof...(TailEntries) + 1;
	constexpr static size_t maxSize = std::max(sizeof(typename HeadEntry::type),
		alignof(typename HeadEntry::type)) * HeadEntry::dim;
	char _data[entriesNum][maxSize];

public:
	bool GetData(size_t nthEntry, void* out, size_t len)
	{
		if (nthEntry >= entriesNum) [[unlikely]]{
			return false;
		}

		std::copy_n(_data[nthEntry], std::min(len, maxSize), reinterpret_cast<char*>(out));
		return true;
	}

	bool SetData(size_t nthEntry, const void* value, size_t len)
	{
		if (nthEntry >= entriesNum) [[unlikely]] {
			return false;
		}

		std::copy_n(reinterpret_cast<char*>(value), std::min(len, maxSize), _data[nthEntry]);
		return true;
	}
};

/*
struct Region
{
	virtual bool GetData(size_t index, void* out, size_t len) = 0;
	virtual bool SetData(size_t index, const void* out, size_t len) = 0;
	virtual ~Region() = default;
};

template<typename ...R>
class Regions : private R...
{
private:
	Region* regions[sizeof...(R)];

public:
	constexpr Regions() {
		// 使用虚表进行类型擦除，运行时派发到具体的GenericRegion
		size_t i = 0;
		((regions[i++] = static_cast<R*>(this)), ...);
	}
};
*/

template<typename ...R>
class Regions
{
public:
	bool GetData(size_t index, void* out, size_t len)
	{
		auto op = [&](auto& region, size_t nthEntry) {
			return region.GetData(nthEntry, out, len);
		};
		return ProcData(std::make_index_sequence<sizeof...(R)>{}, op, index);
	}
	
	bool SetData(size_t index, const void* value, size_t len)
	{
		auto op = [&](auto& region, size_t nthEntry) {
			return region.SetData(nthEntry, value, len);
		};
		return ProcData(std::make_index_sequence<sizeof...(R)>{}, op, index);
	}

private:
	template<size_t _Index, typename _Op>
	bool ProcData(size_t index, _Op&& op)
	{
		size_t regionIdx = index >> 16;
		size_t nthEntry = index & 0xFFFF;
		if (_Index == regionIdx) {
			return op(std::get<_Index>(regions_), nthEntry);
		}
		return false;
	}

	template<typename _Op, size_t ..._Indexes>
	bool ProcData(std::index_sequence<_Indexes...>, _Op&& op, size_t index)
	{
		// 此处短路操作等价与if/else效果
		return (ProcData<_Indexes>(std::forward<_Op>(op), index) || ...);
	}

private:
	std::tuple<R...> regions_;
};

template<TL GroupedEntries>
class GenericRegionTrait
{
private:
	template<TL G>
	using ToRegion = Return<typename G::template exportTo<GenericRegion>>;
public:
	using type = Map_t<GroupedEntries, ToRegion>;
};

template<TL GroupedEntries>
using GenericRegionTrait_t = typename GenericRegionTrait<GroupedEntries>::type;

template<TL GroupedEntries>
using RegionsInst = typename GenericRegionTrait_t<GroupedEntries>::template exportTo<Regions>;

template<typename ...Indexes>
struct Indexer
{
	size_t keyToId[sizeof...(Indexes)];
	std::bitset<sizeof...(Indexes)> mask;
	constexpr Indexer()
	{
		constexpr size_t indexSize = sizeof...(Indexes);
		static_assert(((Indexes::key < indexSize)&&...), "key is out of size");
		(void(keyToId[Indexes::key] == Indexes::id), ...);
	}
};

template<TL GroupedEntries>
class GroupIndexTrait
{
private:
	template<size_t GroupIdx = 0, size_t InnerIdx = 0, TL Res = TypeList<>>
	struct Index
	{
		constexpr static size_t groupIdx = GroupIdx;
		constexpr static size_t innerIdx = InnerIdx;
		using result = Res;
	};

	template<typename Acc, TL G>
	class AddGroup
	{
	private:
		constexpr static size_t groupIdx = Acc::groupIdx;
		template<typename Acc_, KVEntry E>
		class AddKey 
		{
			constexpr static size_t innerIdx = Acc_::innerIdx;
			struct KeyWithIndex
			{
				constexpr static auto key = E::key;
				constexpr static auto id = groupIdx << 16 | innerIdx;
			};
			using result = typename Acc_::result::template append<KeyWithIndex>;
		
		public:
			using type = Index<groupIdx + 1, innerIdx + 1, result>;
		};
		using result = typename Acc::result;
	
	public: // 内层进行迭代
		using type = Fold_t<G, Index<groupIdx + 1, 0, result>, AddKey>;
	};

public:
	using type = typename Fold_t<GroupedEntries, Index<>, AddGroup>::result;
};

template<TL GroupedEntries>
using IndexerInst = typename GroupIndexTrait<GroupedEntries>::type::template exportTo<Indexer>;

template<TL Es>
class DataTable {
private:
	RegionsInst<Es> regions_;
	IndexerInst<Es> indexer_;

public:
	bool GetData(size_t key, void* out, size_t len = -1)
	{
		if (key >= Es::size || !indexer_.mask[key]) {
			return false;
		}
		return regions_.GetData(indexer_.keyToId[key], out, len);
	}
	bool SetData(size_t key, const void* value, size_t len = -1)
	{
		if (key >= Es::size) {
			return false;
		}
		indexer_.mask[key] = regions_.SetData(indexer_.keyToId[key], value, len);
		return indexer_.mask[key];
	}
};
#endif // !DATA_TABLE_H