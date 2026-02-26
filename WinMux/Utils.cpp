#include "Utils.h"

namespace WinMux
{
	bool IsPointInRect(
		const wxPoint& point, 
		const wxPoint& rectPos, 
		const wxSize& rectSize)
	{
		return 
			point.x >= rectPos.x && 
			point.x <= rectPos.x + rectSize.x &&
			point.y >= rectPos.y && 
			point.y <= rectPos.y + rectSize.y;
	}
}