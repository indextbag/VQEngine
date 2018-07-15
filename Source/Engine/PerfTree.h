//	vilbeyli/VQEngine
//	Copyright(C) 2018  - Volkan Ilbeyli
//
//	This program is free software : you can redistribute it and / or modify
//	it under the terms of the GNU General Public License as published by
//	the Free Software Foundation, either version 3 of the License, or
//	(at your option) any later version.
//
//	This program is distributed in the hope that it will be useful,
//	but WITHOUT ANY WARRANTY; without even the implied warranty of
//	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
//	GNU General Public License for more details.
//
//	You should have received a copy of the GNU General Public License
//	along with this program.If not, see <http://www.gnu.org/licenses/>.
//
//	Contact: volkanilbeyli@gmail.com
#pragma once

template<class T> struct TreeNode
{
	const T*				pData;
	TreeNode*				pParent;
	std::vector<TreeNode>	children;

	bool operator<(const TreeNode<T>& other) { return pData->operator<(*other.pData); }
	bool operator==(const TreeNode<T>& other) { return pData == other.pData; }
};

constexpr int PERF_TREE_ENTRY_DRAW_Y_OFFSET_PER_LINE = 22;	// pixels
template<class T> struct Tree
{
	template<class T>
	using pFnNodeComparisonPredicate = bool(*)(const TreeNode<T>&, const TreeNode<T>&);
	using pFnNodePredicate = bool(*)(const TreeNode<T>&);
	

public:	
	//----------------------------------------------------------------------------------------------------------------
	// INTERFACE
	//----------------------------------------------------------------------------------------------------------------

	// Adds a child node with pData to the given parent
	//
	TreeNode<T>* AddChild(TreeNode<T>& parent, const T* pData);

	// Sorts the tree on each level based on the comparison function
	//
	void Sort(pFnNodeComparisonPredicate<T> fnLessThanComparison);
	void Sort();

	// Renders the tree, returns the number of lines rendered.
	//
	size_t RenderTree(TextRenderer* pTextRenderer, const vec2& screenPosition, TextDrawDescription& drawDesc);

	// Returns a const pointer to TreeNode with the given tag
	//
	const TreeNode<T>* FindNode(const std::string& tag) const;

	// Removes all the nodes from the tree
	//
	void Clear();

	// Prunes the tree with the given predicate on nodes.
	// returns the number of nodes pruned.
	//
	size_t Prune(pFnNodePredicate prunePredicate);

public:
	//----------------------------------------------------------------------------------------------------------------
	// DATA
	//----------------------------------------------------------------------------------------------------------------
	TreeNode<T> root = { nullptr, nullptr, std::vector<TreeNode<T>>() };


private:
	// Finds the TreeNode given address of the TreeNode's data.
	//
	TreeNode<T>* FindNode(const T* pData) const;

	// searches among children | depth first, returns nullptr if not found
	//
	TreeNode<T>* SearchSubTree(const TreeNode<T>& node, const T* pSearchEntry) const;
	const TreeNode<T>* SearchSubTree(const TreeNode<T>& node, const std::string& tag) const;

	size_t RenderSubTree(
		const TreeNode<T>&		node,
		TextRenderer *			pTextRenderer,
		const vec2 &			screenPosition,
		TextDrawDescription&	drawDesc,
		std::ostringstream&		stats
	);

	void SortSubTree(TreeNode<T>& node, pFnNodeComparisonPredicate<T> fnLessThanComparison);
	void SortSubTree(TreeNode<T>& node);

	size_t PruneRecurse(TreeNode<T>& node, pFnNodePredicate prunePredicate);
};

template<class T>
void Tree<T>::Clear()
{
	root.children.clear();
	root.pData = nullptr;
	root.pParent = nullptr;
}

// TEMPLATE TREENODE AND TREE IMPLEMENTATION
// ===============================================================================================================================================
template<class T>
inline TreeNode<T> * Tree<T>::AddChild(TreeNode<T>& parent, const T * pData)
{
	TreeNode<T> newNode = { pData, &parent, std::vector<TreeNode<T>>() };
	parent.children.push_back(newNode);
	return &parent.children.back();	// is this safe? this might not be safe...
}

template<class T>
inline void Tree<T>::Sort(pFnNodeComparisonPredicate<T> fnLessThanComparison)
{
	SortSubTree(root, fnLessThanComparison);
}

template<class T>
inline void Tree<T>::Sort()
{
	SortSubTree(root);
}

template<class T>
inline size_t Tree<T>::RenderTree(TextRenderer * pTextRenderer, const vec2 & screenPosition, TextDrawDescription & drawDesc)
{
	std::ostringstream stats;	// set stream properties once, 
	stats.precision(2);			// and pass it to the recursive 
	stats << std::fixed;		// function.
	if(root.pData)
		return RenderSubTree(root, pTextRenderer, screenPosition, drawDesc, stats);

	return 0;
}

template<class T>
inline TreeNode<T>* Tree<T>::FindNode(const T * pData) const
{
	if (root.pData == pData) return &root;
	return SearchSubTree(root, pData);
}
template<class T>
inline const TreeNode<T>* Tree<T>::FindNode(const std::string& tag) const
{
	if (root.pData == nullptr)
		return nullptr;

	if (root.pData->tag == tag) 
		return &root;

	return SearchSubTree(root, tag);
}


template<class T>
inline TreeNode<T>* Tree<T>::SearchSubTree(const TreeNode<T>& node, const T * pSearchEntry) const
{
	TreeNode<T>* pSearchResult = nullptr;

	// visit children | depth first
	for (size_t i = 0; i < node.children.size(); i++)
	{
		TreeNode<T>* pEntryNode = &root.children[i];
		if (pSearchEntry == pEntryNode->pData)
			return pEntryNode;

		pSearchResult = SearchSubTree(*pEntryNode, pSearchEntry);
		if (pSearchResult && pSearchResult->pData == pSearchEntry)
			return pSearchResult;
	}

	return pSearchResult;
}
template<class T>
inline const TreeNode<T>* Tree<T>::SearchSubTree(const TreeNode<T>& node, const std::string& tag) const
{
	const TreeNode<T>* pSearchResult = nullptr;

	// visit children | depth first
	for (size_t i = 0; i < node.children.size(); i++)
	{
		const TreeNode<T>* pEntryNode = &root.children[i];
		if (tag == pEntryNode->pData->tag)
			return pEntryNode;

		pSearchResult = SearchSubTree(*pEntryNode, tag);
		if (pSearchResult && pSearchResult->pData->tag == tag)
			return pSearchResult;
	}

	return pSearchResult;
}

template<class T>
inline size_t Tree<T>::RenderSubTree(
	const TreeNode<T>&      node
	, TextRenderer*         pTextRenderer
	, const vec2&           screenPosition
	, TextDrawDescription&  drawDesc
	, std::ostringstream&   stats
)
{
	const int X_OFFSET = 20;	// todo: DrawSettings struct for the tree?

	if (node.pData->IsStale()) return 0;
	stats.clear(); stats.str("");	// clear & populate text
	stats << node.pData->tag << "  " << node.pData->GetAvg() * 1000 << " ms";

	drawDesc.screenPosition = screenPosition;
	drawDesc.text = stats.str();
	pTextRenderer->RenderText(drawDesc);

	size_t row_count = 0;
	size_t last_row = 1;	// for children?
	size_t children_rows = 0;
	for (size_t i = 0; i < node.children.size(); i++)
	{
		if (i != 0)
		{
			const TreeNode<T>& child = node.children[i - 1];
			row_count += (child.pData->IsStale())
				? 0
				: child.children.size();
		}
		
		const size_t child_rows = RenderSubTree(node.children[i], pTextRenderer,
		{
			screenPosition.x() + X_OFFSET,
			screenPosition.y() + PERF_TREE_ENTRY_DRAW_Y_OFFSET_PER_LINE * (last_row + row_count)
		} , drawDesc, stats);

		children_rows += child_rows;
		if (child_rows > 0)
		{
			last_row += 1;
		}
	}
	return children_rows + 1;
}

template<class T>
inline void Tree<T>::SortSubTree(TreeNode<T>& node, pFnNodeComparisonPredicate<T> fnLessThanComparison)
{
	std::sort(node.children.begin(), node.children.end(), fnLessThanComparison);
	std::for_each(node.children.begin(), node.children.end(), [this, fnLessThanComparison](TreeNode<T>& n) { SortSubTree(n, fnLessThanComparison); });
}


template<class T>
inline void Tree<T>::SortSubTree(TreeNode<T>& node)
{
	std::sort(node.children.begin(), node.children.end());
	std::for_each(node.children.begin(), node.children.end(), [this](TreeNode<T>& n) { SortSubTree(n); });
}


template<class T>
inline size_t Tree<T>::PruneRecurse(TreeNode<T>& node, pFnNodePredicate prunePredicate)
{
	if (!node.pData)
		return 0;

	std::vector<TreeNode<T>> pStaleChildren;

	std::vector<TreeNode<T>>::iterator it = node.children.begin();
	size_t szRemovedGrandChildren = 0;
	while (it != node.children.end())
	{
		if (it->pData == nullptr || it->pData->IsStale())
		{
			pStaleChildren.push_back(*it);
		}
		else
		{	// recursively prune grand children
			szRemovedGrandChildren += PruneRecurse(*it, prunePredicate);
		}
		++it;
	}

	std::for_each(pStaleChildren.begin(), pStaleChildren.end(), [&](const TreeNode<T>& n) 
	{
		node.children.erase(
			std::remove(node.children.begin(), node.children.end(), n),
			node.children.end()
		);
	});

	// TODO: test correctness
	// is the pruned children size accurate? 
	// what if stale children had stale grandchildren?
	return szRemovedGrandChildren + pStaleChildren.size();
}

template<class T>
inline size_t Tree<T>::Prune(pFnNodePredicate prunePredicate)
{
	return PruneRecurse(root, prunePredicate); 
}

// perftree.h(279): warning C4834: 
// discarding return value of function with 'nodiscard' attribute (compiling source file C:\Users\volkan\Documents\GitHub\VQEngine\Source\Utilities\Source\Profiler.cpp)