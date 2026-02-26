#pragma once
#define ASSERT_REACHED_ILLEGAL_POINT() assert(!"Reached illegal code that shouldn't be reachable");

namespace WinMux
{
	struct Context;
	struct Node;
	struct OverlapDropDst;
	struct Sash;
	struct SashKey;

	class App;
	class OverlayDropDstPreview;
	class SubTabs;
	class SubTitlebar;
	class TaskbarIcon;
	class WinInst;

	class DragWinNoticeEvent;

	enum class TabFeature
	{
		Text
	};

	enum class NodeType
	{
		Err = -1,
		Window,
		SplitHoriz,
		SplitVert,
		Tabs
	};

	enum class Insertion
	{
		Before,
		After
	};

	enum class DockDir
	{
		Into,
		Top,
		Left,
		Bottom,
		Right
	};

	enum class CloseMode
	{
		DestroyAll,
		ReleaseAll,
		BlockingCloseAll
	};

	struct Color3
	{
		unsigned char r;
		unsigned char g;
		unsigned char b;

		Color3()
		{}

		Color3(unsigned char r, unsigned char g, unsigned char b)
			: r(r), g(g), b(b)
		{}
	};

	struct ButtonColors
	{
		Color3 normal;
		Color3 hovered;
		Color3 pressed;

		ButtonColors(
			const Color3& normal,
			const Color3& hovered,
			const Color3& pressed)
			: normal(normal),
			  hovered(hovered),
			  pressed(pressed)
		{}

		const Color3& GetColor(bool isPressed, bool isHovered) const
		{
			if(isPressed)
				return this->pressed;
			if(isHovered)
				return this->hovered;
			return this->normal;

		}
	};
}
