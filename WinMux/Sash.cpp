#include "Sash.h"

namespace WinMux
{
	Sash::Sash()
	{}

	Sash::Sash(Node* container, int index, wxPoint pos, wxSize size)
		: container(container),
		  index(index),
		  pos(pos),
		  size(size)
	{}

	Sash::Sash(Node*container, int index, int posx, int posy, int sizex, int sizey)
		: container(container),
		  index(index),
		  pos(posx, posy),
		  size(sizex, sizey)
	{}

	SashKey::SashKey(Node* a, Node* b)
	{
		if(a < b)
		{
			this->a = a;
			this->b = b;
		}
		else
		{
			this->a = b;
			this->b = a;
		}
	}
}