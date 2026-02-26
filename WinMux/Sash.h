#pragma once
#include "Defines.h"
#include <wx/wx.h>

namespace WinMux
{
	struct Sash
	{
		Node* container;
		int index;

		wxPoint pos;
		wxSize size;

	public:
		Sash();
		Sash(Node*container, int index, wxPoint pos, wxSize size);
		Sash(Node*container, int index, int posx, int posy, int sizex, int sizey);
	};

	struct SashKey
	{
		Node* a;
		Node* b;

	public:
		SashKey(Node* a, Node* b);

		bool operator<(const SashKey& other) const
		{
			if (this->a < other.a)
				return true;

			if (this->a == other.a)
				return this->b < other.b;

			return false;
		}
	};
}