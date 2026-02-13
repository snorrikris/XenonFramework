#pragma once
#include <string>

struct TOOLTIP_SETTINGS
{
	std::wstring m_strTooltipText;
	bool m_bCoverCell = false;			// true when tooltip window should cover the cell, false when tooltip shown below cell.
	bool m_bCanChangeXpos = false;		// When tooltip is wide the X position can be changed, more to the left.
};

