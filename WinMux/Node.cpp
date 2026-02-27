#include <assert.h>
#include "Node.h"
#include "Sash.h"
#include "Context.h"
#include "SubTitlebar.h"
#include "SubTabs.h"

namespace WinMux
{
	int Node::debugCtr = 0;

	Node::Node()
		: type(NodeType::Err),
		  parent(NULL),
		  winHandle(NULL)
	{
		this->debugID = debugCtr;
		++debugCtr;
	}

	Node::Node(NodeType type, Node* parent, float prop)
		: type(type),
		  parent(parent),
		  winHandle(NULL),
		  prop(prop)
	{
		this->debugID = debugCtr;
		++debugCtr;
	}

	Node::~Node()
	{
		for (Node* n : this->children)
			delete n;

		this->children.clear();

		this->CleanSupportingUI();
	}

	Node* Node::MakeWinNode(Node* parent, float prop, HWND hwnd, HANDLE process)
	{
		Node* node = new Node(NodeType::Window, parent, prop);
		node->winHandle = hwnd;
		node->process = process;
		return node;
	}

	Node* Node::MakeSplitContainerNode(
		NodeType containerType, 
		Node* parent, 
		float prop,
		Node* left, 
		Node* right, 
		bool swap)
	{
		assert(
			containerType == NodeType::SplitHoriz || 
			containerType == NodeType::SplitVert ||
			containerType == NodeType::Tabs);

		Node* newNode = new Node(containerType, parent, prop);
		if(!swap)
		{
			newNode->children.push_back(left);
			newNode->children.push_back(right);
		}
		else
		{
			newNode->children.push_back(right);
			newNode->children.push_back(left);
		}
		left->parent = newNode;
		right->parent = newNode;
		return newNode;
	}

	void Node::CalculateMinSizeRecursive(const Context& ctx)
	{
		const size_t childCt = children.size();

		switch(this->type)
		{
		case NodeType::Window:
			this->minSize = ctx.minWindowSize;
			break;
		case NodeType::SplitHoriz:
			{
				int accumMinWidth = 0;
				int maxMinHeight = 0;
				for(size_t i = 0; i < childCt; ++i)
				{
					Node* child = children[i];
					child->CalculateMinSizeRecursive(ctx);

					accumMinWidth += child->minSize.x;
					maxMinHeight = std::max(maxMinHeight, child->minSize.y);

					if(i != childCt - 1)
						accumMinWidth += ctx.sashWidth;
				}
				this->minSize.Set(accumMinWidth, maxMinHeight);
			}
			break;
		case NodeType::SplitVert:
			{
				int maxMinWidth = 0;
				int accumMinHeight = 0;
				for(size_t i = 0; i < childCt; ++i)
				{
					Node* child = children[i];
					child->CalculateMinSizeRecursive(ctx);

					accumMinHeight += child->minSize.y;
					maxMinWidth = std::max(maxMinWidth, child->minSize.x);

					if(i != childCt - 1)
						accumMinHeight += ctx.sashWidth;
				}
				this->minSize.Set(maxMinWidth, accumMinHeight);
			}
			break;
		case NodeType::Tabs:
			this->minSize = wxSize(ctx.minWindowSize.x, ctx.minWindowSize.y + ctx.tabHeight);
			break;
		}
	}

	void Node::CacheChildProportions(bool recurse)
	{
		const size_t childCt = this->children.size();
		int accumExtra = 0;

		if(this->type == NodeType::SplitHoriz)
		{
			for(int i = 0; i < childCt; ++i)
			{
				Node* child = this->children[i];
				child->prop = std::max(0, child->size.x - child->minSize.x);
			}
			NormalizeChildProportions();
			
		}
		else if(this->type == NodeType::SplitVert)
		{
			for (int i = 0; i < childCt; ++i)
			{
				Node* child = this->children[i];
				child->prop = std::max(0, child->size.y - child->minSize.y);
			}
			NormalizeChildProportions();
		}
		if (recurse)
		{ 
			for(Node* child : this->children)
				child->CacheChildProportions(true);
		}
	}

	void Node::NormalizeChildProportions()
	{
		if(this->children.size() == 0)
			return;

		float accumProp = 0.0f;
		for(Node* child : this->children)
			accumProp += child->prop;

		if(accumProp == 0.0f)
		{
			float avg = 1.0f / this->children.size();
			for (Node* child : this->children)
				child->prop = avg;
		}
		else
		{
			for (Node* child : this->children)
				child->prop = child->prop / accumProp;
		}
	}

	float Node::AvgChildProp()
	{
		assert(this->children.size() > 0);
		return 1.0f / this->children.size();
	}

	void Node::CacheCalculatedSizeRecursive(const wxPoint& newPos, const wxSize& newSize, const Context& ctx)
	{
		const size_t childCt = children.size();

		this->pos = newPos;
		this->size = newSize;

		switch(this->type)
		{
		case NodeType::Window:
			break;

		case NodeType::SplitHoriz:
			{
				int allocatableSpace = newSize.x - this->minSize.x;
				int x = newPos.x;
				for(size_t i = 0; i < childCt; ++i)
				{
					bool isLast = i == (childCt - 1);
					Node* child = this->children[i];
					int spaceTaken = child->minSize.x + child->prop * allocatableSpace;

					child->CacheCalculatedSizeRecursive(
						wxPoint(x, newPos.y),
						wxSize(spaceTaken, newSize.y),
						ctx);

					x += spaceTaken + ctx.sashWidth;
				}
			}
			break;

		case NodeType::SplitVert:
			{
			int allocatableSpace = newSize.y - this->minSize.y;
			int y = newPos.y;
			for (size_t i = 0; i < childCt; ++i)
			{
				bool isLast = i == (childCt - 1);
				Node* child = this->children[i];
				int spaceTaken = child->minSize.y + child->prop * allocatableSpace;

				child->CacheCalculatedSizeRecursive(
					wxPoint(newPos.x, y),
					wxSize(newSize.x, spaceTaken),
					ctx);

				y += spaceTaken + ctx.sashWidth;
			}
			}
			break;

		case NodeType::Tabs:
			{
				this->pos = newPos;
				this->size = newSize;
				wxPoint innerPos(newPos.x, newPos.y + ctx.tabHeight);
				wxSize innerSize(newSize.x, newSize.y - ctx.tabHeight);
				for(Node* child : children)
					child->CacheCalculatedSizeRecursive(innerPos, innerSize, ctx);
			}
			break;
		}
	}

	int Node::GetChildIndex(Node* child)
	{
		for(size_t i = 0; i < this->children.size(); ++i)
		{
			if(this->children[i] == child)
				return i;
		}

		return -1;
	}

	bool Node::SwapChild(Node* src, Node* dst, bool deparentSrc)
	{
		int childIdx = GetChildIndex(src);
		if(childIdx == -1)
			return false;

		if(deparentSrc)
			src->parent = nullptr;

		this->children[childIdx] = dst;
		dst->parent = this;
		return true;
	}

	bool Node::InsertAsChild(Node* toInsert, Node* existingChild, Insertion insertSpot)
	{
		int idx = GetChildIndex(existingChild);
		if(idx == -1)
			return false;

		if(insertSpot == Insertion::After)
			++idx;

		return InsertAsChild(toInsert, idx);
	}

	bool Node::InsertAsChild(Node* toInsert, int insertIdx)
	{
		if(insertIdx < 0 || insertIdx > children.size())
			return false;

		children.insert(children.begin() + insertIdx, toInsert);
		toInsert->parent = this;
		return true;
	}

	bool Node::ClampTabId()
	{
		assert(this->type == NodeType::Tabs);
		// Not >1, because could have had a child just deleted and it's about to be
		// cleaned out (replaced with the single inner window child).
		assert(this->children.size() > 0); 

		if(this->activeTab >= children.size())
		{
			this->activeTab = children.size() - 1;
			return true;
		}
		return false;
	}

	float Node::ChildPropTakenUpByAllExcept(Node* a, Node* b)
	{
		assert(this->type == NodeType::SplitHoriz || this->type == NodeType::SplitVert);
		float accum = 0.0f;
		for(Node* n : this->children)
		{
			if(n != a && n != b)
				accum += n->prop;
		}
		return accum;
	}

	void Node::ApplyCachedlayout(const Context& ctx)
	{
		ApplyNodeLayout(this->pos, this->size, ctx);
	}

	void Node::ApplyNodeLayout(const wxPoint& manualPos, const wxSize& manualSize, const Context& ctx)
	{
		switch (this->type)
		{
		case NodeType::Window:
			assert(this->titlebar != nullptr);
			this->titlebar->Show(true);
			this->titlebar->SetSize(manualPos.x, manualPos.y, manualSize.x, ctx.titleHeight);

			if (this->winHandle != NULL) // !TODO: Convert this to an assert when finally implemented
			{
				// SWP_NOSENDCHANGING to override minimum size constraint.
				// Although may not be enough for all cases of WM_GETMINMAXINFO
				::ShowWindow(this->winHandle, SW_SHOW);
				::SetWindowPos(
					this->winHandle,
					NULL,
					manualPos.x,
					manualPos.y + ctx.titleHeight,
					manualSize.x,
					manualSize.y - ctx.titleHeight,
					SWP_DEFERERASE | SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOZORDER | SWP_NOSENDCHANGING);

				assert(children.size() == 0);
			}
			break;

		case NodeType::SplitHoriz:
		case NodeType::SplitVert:
			for (Node* child : children)
				child->ApplyCachedlayout(ctx);
			break;

		case NodeType::Tabs:
			assert(children.size() != 0);
			assert(this->tabsBar != nullptr);
			this->tabsBar->SetSize(manualPos.x, manualPos.y, manualSize.x, ctx.tabHeight);

			assert(this->activeTab >= 0 && this->activeTab < children.size());
			for (size_t i = 0; i < children.size(); ++i)
			{
				Node* child = children[i];
				assert(child->type == NodeType::Window);

				if (i == this->activeTab)
				{
					child->ApplyCachedlayout(ctx);
				}
				else
				{
					child->titlebar->Hide();
					if (child->winHandle != nullptr)
						::ShowWindow(children[i]->winHandle, SW_HIDE);
				}
			}
			break;
		}
	}

	wxString Node::GetWindowString()
	{
		assert(this->type == NodeType::Window);

		const int stringBufferLen = 128;
		wchar_t stringBuffer[stringBufferLen];
		bool filledText = ::GetWindowText(this->winHandle, stringBuffer, stringBufferLen) != 0;
		wxString titleText = filledText ? stringBuffer : (wchar_t*)L"...";
		titleText << "(" << wxString::Format(wxT("%i"), (int)this->debugID) << ")";
		return titleText;
	}

	void Node::CleanSupportingUI()
	{
		if (this->titlebar != nullptr)
		{
			this->titlebar->Destroy();
			this->titlebar = nullptr;
		}
		if (this->tabsBar != nullptr)
		{
			this->tabsBar->Destroy();
			this->tabsBar = nullptr;
		}
	}

	bool Node::VALI(bool recurse)
	{
		switch(this->type)
		{
		case NodeType::Window:
			assert(children.size() == 0);
			break;
		case NodeType::SplitHoriz:
			assert(children.size() > 1);
			for(Node* child : children)
				assert(child->type != NodeType::SplitHoriz);
			break;
		case NodeType::SplitVert:
			assert(children.size() > 1);
			for(Node* child : children)
				assert(child->type != NodeType::SplitVert);
			break;
		case NodeType::Tabs:
			assert(children.size() > 1);
			for(Node* child : children)
				assert(child->type == NodeType::Window);
			break;
		}

		for(Node* c : children)
		{ 
			assert(c->parent == this);
			assert(c->type != this->type);
		}

		if(recurse)
		{
			for(Node* c : children)
				c->VALI(true);
		}
		return true;
	}

	bool Node::VALI_IsFilledWinNode()
	{
		assert(this->type == NodeType::Window);
		assert(this->titlebar != nullptr);
		return true;
	}
}