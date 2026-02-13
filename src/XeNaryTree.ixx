module;

#include <vector>
#include <map>
#include <memory>
#include "XeAssert.h"

export module Xe.NaryTree;

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// N-ary tree class - used by XeTreeCtrl

export template<class T>
class CXeNaryTreeNode
{
protected:
	CXeNaryTreeNode<T>*	m_parent;
	std::vector<std::unique_ptr<CXeNaryTreeNode<T>>> m_children;

	// Flag to indicate if children shold be visible in tree view.
	bool m_isExpanded = false;

	// Level count from root (0=root)
	int m_level = 0;

public:
	T		m_data;

	CXeNaryTreeNode() = default;
	CXeNaryTreeNode(CXeNaryTreeNode* parent, const T& data) : m_parent(parent), m_data(data) {}

	bool IsRootNode() const { return m_parent == nullptr; }

	const CXeNaryTreeNode<T>* GetConstParentNode() const
	{
		return m_parent;
	}
	CXeNaryTreeNode<T>* GetParentNode() const
	{
		return m_parent;
	}

	CXeNaryTreeNode<T>* AddChild(const T& data)
	{
		std::unique_ptr<CXeNaryTreeNode<T>> child = std::make_unique<CXeNaryTreeNode<T>>(this, data);
		CXeNaryTreeNode<T>* pNewChild = child.get();
		pNewChild->m_level = m_level + 1;
		m_children.push_back(std::move(child));
		return pNewChild;
	}

	bool HasChildren() const { return m_children.size() > 0; }
	const std::vector<std::unique_ptr<CXeNaryTreeNode<T>>>& GetChildren() const
	{
		return m_children;
	}

	bool IsExpanded() const { return m_isExpanded; }
	void _SetExpandedFlag(bool isExpanded)
	{
		m_isExpanded = isExpanded;
	}

	int GetLevel() const { return m_level; }

	bool IsLastChild() const
	{
		if (m_parent)
		{
			return m_parent->m_children[m_parent->m_children.size() - 1].get() == this;
		}
		return true;
	}
};

export typedef void* HXETREEITEM;

export template<class T>
class CXeNaryTree
{
protected:
	std::unique_ptr<CXeNaryTreeNode<T>> m_root;

	std::map<HXETREEITEM, bool> m_valid_nodes;

	bool m_isTotalNodeCountChanged = false, m_isVisibleNodeCountChanged = false;
	size_t m_totalNodeCount = 0, m_visibleNodeCount = 0;

public:
	bool HasRoot() const { return m_root.get() != nullptr; }
	bool CreateRoot(const T& data)
	{
		if (m_root.get())
		{
			XeASSERT(false);	// Delete root before create again.
			return false;
		}
		m_root = std::make_unique<CXeNaryTreeNode<T>>(nullptr, data);
		HXETREEITEM hItem = reinterpret_cast<HXETREEITEM>(m_root.get());
		m_valid_nodes[hItem] = true;
		m_isVisibleNodeCountChanged = m_isTotalNodeCountChanged = true;
		return true;
	}

	void DeleteRoot()
	{
		if (HasRoot())
		{
			m_isVisibleNodeCountChanged = m_isTotalNodeCountChanged = true;
			m_root.reset();
			m_valid_nodes.clear();
		}
	}

	const CXeNaryTreeNode<T>* GetRoot() const { return m_root.get(); }

	// Returns pointer to the new child node.
	CXeNaryTreeNode<T>* AddNode(CXeNaryTreeNode<T>* parent, const T& data)
	{
		if (GetNodeHandle(parent) != 0)
		{
			m_isVisibleNodeCountChanged = m_isTotalNodeCountChanged = true;
			CXeNaryTreeNode<T>* pNode = parent->AddChild(data);
			HXETREEITEM hItem = reinterpret_cast<HXETREEITEM>(pNode);
			m_valid_nodes[hItem] = true;
			return pNode;
		}
		return nullptr;
	}

	HXETREEITEM GetNodeHandle(const CXeNaryTreeNode<T>* pNode) const
	{
		HXETREEITEM hItem
			= reinterpret_cast<HXETREEITEM>(const_cast<CXeNaryTreeNode<T>*>(pNode));
		if (m_valid_nodes.contains(hItem))
		{
			return hItem;
		}
		XeASSERT(false);	// node not found.
		return 0;
	}

	const CXeNaryTreeNode<T>* GetNode(HXETREEITEM hItem) const
	{
		if (m_valid_nodes.contains(hItem))
		{
			return reinterpret_cast<const CXeNaryTreeNode<T>*>(hItem);
		}
		XeASSERT(false);	// node not found.
		return nullptr;
	}

	void SetExpandedFlag(const CXeNaryTreeNode<T>* node, bool isExpanded)
	{
		if (GetNodeHandle(node) != 0)
		{
			m_isVisibleNodeCountChanged = true;
			CXeNaryTreeNode<T>* pNonConstNode = const_cast<CXeNaryTreeNode<T>*>(node);
			pNonConstNode->_SetExpandedFlag(isExpanded);
		}
	}

	size_t GetTotalNodeCount()
	{
		if (m_isTotalNodeCountChanged)
		{
			m_isTotalNodeCountChanged = false;
			m_totalNodeCount = 0;
			if (HasRoot())
			{
				CXeNaryTreeNode<T>* pNode = m_root.get();
				_FindNodeAtIdx(pNode, m_totalNodeCount, (size_t)-1, false);
			}
		}
		return m_totalNodeCount;
	}

	size_t GetVisibleNodeCount()
	{
		if (m_isVisibleNodeCountChanged)
		{
			m_isVisibleNodeCountChanged = false;
			m_visibleNodeCount = 0;
			if (HasRoot())
			{
				CXeNaryTreeNode<T>* pNode = m_root.get();
				_FindNodeAtIdx(pNode, m_visibleNodeCount, (size_t)-1, true);
			}
		}
		return m_visibleNodeCount;
	}

	size_t GetIndexOfNode(const CXeNaryTreeNode<T>* pNodeToFind) const
	{
		if (GetNodeHandle(pNodeToFind) != 0
			&& HasRoot())
		{
			const CXeNaryTreeNode<T>* pNode = m_root.get();
			size_t count = 0;
			return _FindIndexOfNode(pNode, count, pNodeToFind);
		}
		return (size_t)-1;
	}

	const CXeNaryTreeNode<T>* GetNodeAtIdx(size_t idx) const
	{
		const CXeNaryTreeNode<T>* pFoundNode = nullptr;
		if (HasRoot())
		{
			const CXeNaryTreeNode<T>* pNode = m_root.get();
			size_t count = 0;
			pFoundNode = _FindNodeAtIdx(pNode, count, idx, true);
		}
		return pFoundNode;
	}

	std::vector<const CXeNaryTreeNode<T>*> GetVisibleNodesIdxRange(
		size_t idx_start, size_t how_many) const
	{
		std::vector<const CXeNaryTreeNode<T>*> nodes;
		if (HasRoot() && how_many > 0)
		{
			const CXeNaryTreeNode<T>* pNode = m_root.get();
			size_t count = 0;
			_GetVisibleNodesIdxRange(pNode, nodes, count, idx_start, how_many);
		}
		return nodes;
	}

protected:
	void _GetVisibleNodesIdxRange(const CXeNaryTreeNode<T>* pNode,
		std::vector<const CXeNaryTreeNode<T>*>& nodes,
		size_t& count, size_t idx_start, size_t& how_many) const
	{
		if (count >= idx_start)
		{
			nodes.push_back(pNode);
			if (--how_many == 0)
			{
				return;
			}
		}
		++count;
		if (pNode->IsExpanded() && pNode->HasChildren())
		{
			const std::vector<std::unique_ptr<CXeNaryTreeNode<T>>>& children
				= pNode->GetChildren();
			for (const std::unique_ptr<CXeNaryTreeNode<T>>& child : children)
			{
				_GetVisibleNodesIdxRange(child.get(), nodes, count, idx_start, how_many);
				if (how_many == 0)
				{
					return;
				}
			}
		}
	}

	const CXeNaryTreeNode<T>* _FindNodeAtIdx(const CXeNaryTreeNode<T>* pNode, size_t& count,
		size_t stop_at, bool onlyVisibleNodes) const
	{
		if (count == stop_at)
		{
			return pNode;
		}
		++count;
		if (pNode->HasChildren()
			&& (!onlyVisibleNodes || (onlyVisibleNodes && pNode->IsExpanded())))
		{
			const std::vector<std::unique_ptr<CXeNaryTreeNode<T>>>& children
				= pNode->GetChildren();
			const CXeNaryTreeNode<T>* pFoundNode;
			for (const std::unique_ptr<CXeNaryTreeNode<T>>& child : children)
			{
				if ((pFoundNode = _FindNodeAtIdx(child.get(), count, stop_at, onlyVisibleNodes)))
				{
					return pFoundNode;
				}
			}
		}
		return nullptr;
	}

	size_t _FindIndexOfNode(const CXeNaryTreeNode<T>* pNode, size_t& count,
		const CXeNaryTreeNode<T>* pNodeToFind) const
	{
		if (pNode == pNodeToFind)
		{
			return count;
		}
		++count;
		if (pNode->IsExpanded() && pNode->HasChildren())
		{
			const std::vector<std::unique_ptr<CXeNaryTreeNode<T>>>& children
				= pNode->GetChildren();
			size_t idx;
			for (const std::unique_ptr<CXeNaryTreeNode<T>>& child : children)
			{
				if ((idx = _FindIndexOfNode(child.get(), count, pNodeToFind)) != (size_t)-1)
				{
					return idx;
				}
			}
		}
		return (size_t)-1;
	}
};

