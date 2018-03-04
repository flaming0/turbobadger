// ================================================================================
// ==      This file is a part of Turbo Badger. (C) 2011-2014, Emil Segerås      ==
// ==                     See tb_core.h for more information.                    ==
// ================================================================================

#include "tb_window.h"
#include <assert.h>

namespace tb {

// == TBWindow ==========================================================================

TBWindow::TBWindow()
{
	SetIsGroupRoot(true);
}

TBWindow::~TBWindow()
{
}

TBRect TBWindow::GetResizeToFitContentRect(RESIZE_FIT fit)
{
	PreferredSize ps = GetPreferredSize();
	int new_w = ps.pref_w;
	int new_h = ps.pref_h;
	if (fit == RESIZE_FIT_MINIMAL)
	{
		new_w = ps.min_w;
		new_h = ps.min_h;
	}
	else if (fit == RESIZE_FIT_CURRENT_OR_NEEDED)
	{
		new_w = CLAMP(GetRect().w, ps.min_w, ps.max_w);
		new_h = CLAMP(GetRect().h, ps.min_h, ps.max_h);
	}
	if (GetParent())
	{
		new_w = MIN(new_w, GetParent()->GetRect().w);
		new_h = MIN(new_h, GetParent()->GetRect().h);
	}
	return TBRect(GetRect().x, GetRect().y, new_w, new_h);
}

void TBWindow::ResizeToFitContent(RESIZE_FIT fit)
{
	SetRect(GetResizeToFitContentRect(fit));
}

void TBWindow::Close()
{
	Die();
}

bool TBWindow::IsActive() const
{
	return GetState(WIDGET_STATE_SELECTED);
}

TBWindow *TBWindow::GetTopMostOtherWindow(bool only_activable_windows)
{
	TBWindow *other_window = nullptr;
	TBWidget *sibling = GetParent()->GetLastChild();
	while (sibling && !other_window)
	{
		if (sibling != this)
			other_window = TBSafeCast<TBWindow>(sibling);

		//if (only_activable_windows && other_window)
		//	other_window = nullptr;

		sibling = sibling->GetPrev();
	}
	return other_window;
}

void TBWindow::Activate()
{
	if (!GetParent())
		return;
	if (IsActive())
	{
		// Already active, but we may still have lost focus,
		// so ensure it comes back to us.
		EnsureFocus();
		return;
	}

	// Deactivate currently active window
	TBWindow *active_window = GetTopMostOtherWindow(true);
	if (active_window)
		active_window->DeActivate();

	// Activate this window

	SetZ(WIDGET_Z_TOP);
	SetWindowActiveState(true);
	EnsureFocus();
}

bool TBWindow::EnsureFocus()
{
	// If we already have focus, we're done.
	if (focused_widget && IsAncestorOf(focused_widget))
		return true;

	// Focus last focused widget (if we have one)
	bool success = false;
	if (m_last_focus.Get())
		success = m_last_focus.Get()->SetFocus(WIDGET_FOCUS_REASON_UNKNOWN);
	// We didn't have one or failed, so try focus any child.
	if (!success)
		success = SetFocusRecursive(WIDGET_FOCUS_REASON_UNKNOWN);
    if (success)
    {
        // if (CheckCanFocus && CheckCanFocus())
            SetAutoFocusState(true);
    }
	return success;
}

void TBWindow::DeActivate()
{
	if (!IsActive())
		return;
	SetWindowActiveState(false);
}

void TBWindow::SetWindowActiveState(bool active)
{
	SetState(WIDGET_STATE_SELECTED, active);
}

int TBWindow::GetTitleHeight()
{
	return 0;
}

TBRect TBWindow::GetPaddingRect()
{
	TBRect padding_rect = TBWidget::GetPaddingRect();
	int title_height = GetTitleHeight();
	padding_rect.y += title_height;
	padding_rect.h -= title_height;
	return padding_rect;
}

PreferredSize TBWindow::OnCalculatePreferredSize(const SizeConstraints &constraints)
{
	PreferredSize ps = OnCalculatePreferredContentSize(constraints);

	// Add window skin padding
	if (TBSkinElement *e = GetSkinBgElement())
	{
		ps.min_w += e->padding_left + e->padding_right;
		ps.pref_w += e->padding_left + e->padding_right;
		ps.min_h += e->padding_top + e->padding_bottom;
		ps.pref_h += e->padding_top + e->padding_bottom;
	}
	// Add window title bar height
	int title_height = GetTitleHeight();
	ps.min_h += title_height;
	ps.pref_h += title_height;
	return ps;
}

void TBWindow::OnAdded()
{
	// If we was added last, call Activate to update status etc.
	if (GetParent()->GetLastChild() == this)
		Activate();
}

void TBWindow::OnRemove()
{
	DeActivate();

	// Active the top most other window
	if (TBWindow *active_window = GetTopMostOtherWindow(true))
		active_window->Activate();
}

bool TBWindow::OnEvent(const tb::TBWidgetEvent &ev)
{
    if (m_closeOnEsc)
    {
        if (ev.type == tb::EVENT_TYPE_KEY_UP && ev.special_key == tb::TB_KEY_ESC)
        {
            Close();
            return true;
        }
    }

    return tb::TBWidget::OnEvent(ev);
}

std::function<bool()> TBWindow::CheckCanFocus;

} // namespace tb
