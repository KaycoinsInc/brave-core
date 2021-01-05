/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/sidebar/sidebar_items_scroll_view.h"

#include <memory>

#include "base/threading/sequenced_task_runner_handle.h"
#include "brave/app/vector_icons/vector_icons.h"
#include "brave/browser/themes/theme_properties.h"
#include "brave/browser/ui/brave_browser.h"
#include "brave/browser/ui/sidebar/sidebar_controller.h"
#include "brave/browser/ui/views/sidebar/sidebar_button_view.h"
#include "brave/browser/ui/views/sidebar/sidebar_items_contents_view.h"
#include "chrome/browser/ui/browser_list.h"
#include "cc/paint/paint_flags.h"
#include "ui/base/theme_provider.h"
#include "ui/events/event.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/paint_vector_icon.h"
#include "ui/views/background.h"
#include "ui/views/controls/button/image_button.h"

namespace {

class SidebarItemsArrowView : public views::ImageButton {
 public:
  SidebarItemsArrowView() {
    SetImageHorizontalAlignment(views::ImageButton::ALIGN_CENTER);
    SetImageVerticalAlignment(views::ImageButton::ALIGN_MIDDLE);
  }

  ~SidebarItemsArrowView() override = default;

  SidebarItemsArrowView(const SidebarItemsArrowView&) = delete;
  SidebarItemsArrowView& operator=(const SidebarItemsArrowView&) = delete;

  gfx::Size CalculatePreferredSize() const override { return {42, 24}; }

  void OnPaintBackground(gfx::Canvas* canvas) override {
    if (const ui::ThemeProvider* theme_provider = GetThemeProvider()) {
      const SkColor background_color = theme_provider->GetColor(
          BraveThemeProperties::COLOR_SIDEBAR_BACKGROUND);
      gfx::Rect bounds = GetContentsBounds();
      canvas->FillRect(bounds, background_color);

      // Draw additional rounded rect over background for hover effect.
      if (GetState() == STATE_HOVERED) {
        const SkColor hovered_bg_color = theme_provider->GetColor(
            BraveThemeProperties::COLOR_SIDEBAR_ARROW_BACKGROUND_HOVERED);
        cc::PaintFlags flags;
        flags.setColor(hovered_bg_color);
        flags.setStyle(cc::PaintFlags::kFill_Style);
        // Use smaller area for hover rounded rect.
        constexpr int kInset = 2, kRadius = 34;
        bounds.Inset(kInset, 0);
        canvas->DrawRoundRect(bounds, kRadius, flags);
      }
    }
  }
};

}  // namespace

SidebarItemsScrollView::SidebarItemsScrollView(BraveBrowser* browser)
    : browser_(browser) {
  observed_.Add(browser->sidebar_controller()->model());
  contents_view_ =
      AddChildView(std::make_unique<SidebarItemsContentsView>(browser_));
  up_arrow_ = AddChildView(std::make_unique<SidebarItemsArrowView>());
  up_arrow_->SetCallback(
      base::BindRepeating(&SidebarItemsScrollView::OnButtonPressed,
                          base::Unretained(this), up_arrow_));
  down_arrow_ = AddChildView(std::make_unique<SidebarItemsArrowView>());
  down_arrow_->SetCallback(
      base::BindRepeating(&SidebarItemsScrollView::OnButtonPressed,
                          base::Unretained(this), down_arrow_));
}

SidebarItemsScrollView::~SidebarItemsScrollView() = default;

void SidebarItemsScrollView::Layout() {
  // |contents_view_| always has it's preferred size. and this scroll view only
  // shows some parts of it if scroll view can't get enough rect.
  contents_view_->SizeToPreferredSize();

  const bool show_arrow = IsScrollable();
  const bool arrow_was_not_shown = !up_arrow_->GetVisible();
  up_arrow_->SetVisible(show_arrow);
  down_arrow_->SetVisible(show_arrow);

  const gfx::Rect bounds = GetContentsBounds();
  const int arrow_height = up_arrow_->GetPreferredSize().height();
  // Locate arrows.
  if (show_arrow) {
    up_arrow_->SizeToPreferredSize();
    up_arrow_->SetPosition(bounds.origin());
    down_arrow_->SizeToPreferredSize();
    down_arrow_->SetPosition({bounds.x(), bounds.bottom() - arrow_height});
  }

  if (show_arrow) {
    // Attach contents view to up arrow view when overflow mode is started.
    if (arrow_was_not_shown) {
      contents_view_->SetPosition(up_arrow_->bounds().bottom_left());
      UpdateArrowViewsState();
      return;
    }

    // Pull contents view when scroll view is getting longer.
    int dist = down_arrow_->bounds().y() - contents_view_->bounds().bottom();
    if (dist > 0) {
      contents_view_->SetPosition(
          {contents_view_->x(), contents_view_->y() + dist});
    }

    UpdateArrowViewsState();
  } else {
    // Scroll view has enough space for contents view.
    contents_view_->SetPosition(bounds.origin());
  }
}

void SidebarItemsScrollView::OnMouseEvent(ui::MouseEvent* event) {
  if (!event->IsMouseWheelEvent())
    return;

  if (!IsScrollable())
    return;

  const int y_offset = event->AsMouseWheelEvent()->y_offset();
  if (y_offset == 0)
    return;

  ScrollContentsViewBy(y_offset);
  UpdateArrowViewsState();
}

gfx::Size SidebarItemsScrollView::CalculatePreferredSize() const {
  DCHECK(contents_view_);
  return contents_view_->GetPreferredSize() + GetInsets().size();
}

void SidebarItemsScrollView::OnThemeChanged() {
  View::OnThemeChanged();

  UpdateArrowViewsTheme();
}

void SidebarItemsScrollView::OnItemAdded(const sidebar::SidebarItem& item,
                                         int index,
                                         bool user_gesture) {
  // Don't need to scroll to bottom if added item is not by user gesture.
  if (!user_gesture)
    return;

  // Only scroll to bottm to show newly added item for active browser window.
  if (browser_ != BrowserList::GetInstance()->GetLastActive())
    return;

  // Give a time to update contents view for new item and then scroll down to
  // show newly added item if scrollable.
  base::SequencedTaskRunnerHandle::Get()->PostTask(
      FROM_HERE,
      base::BindOnce(&SidebarItemsScrollView::ScrollToBottomAfterNewItemAdded,
                     weak_ptr_factory_.GetWeakPtr()));
}

void SidebarItemsScrollView::ScrollToBottomAfterNewItemAdded() {
  // contents_view_->SizeToPreferredSize();
  Layout();
  ScrollContentsViewTo(false);
}

void SidebarItemsScrollView::UpdateArrowViewsTheme() {
  if (const ui::ThemeProvider* theme_provider = GetThemeProvider()) {
    const SkColor arrow_normal = theme_provider->GetColor(
        BraveThemeProperties::COLOR_SIDEBAR_ARROW_NORMAL);
    const SkColor arrow_disabled = theme_provider->GetColor(
        BraveThemeProperties::COLOR_SIDEBAR_ARROW_DISABLED);

    up_arrow_->SetImage(
        views::Button::STATE_NORMAL,
        gfx::CreateVectorIcon(kSidebarItemsUpArrowIcon, arrow_normal));
    up_arrow_->SetImage(
        views::Button::STATE_DISABLED,
        gfx::CreateVectorIcon(kSidebarItemsUpArrowIcon, arrow_disabled));
    down_arrow_->SetImage(
        views::Button::STATE_NORMAL,
        gfx::CreateVectorIcon(kSidebarItemsDownArrowIcon, arrow_normal));
    down_arrow_->SetImage(
        views::Button::STATE_DISABLED,
        gfx::CreateVectorIcon(kSidebarItemsDownArrowIcon, arrow_disabled));
  }
}

void SidebarItemsScrollView::UpdateArrowViewsState() {
  DCHECK(up_arrow_->GetVisible() && down_arrow_->GetVisible());
  const gfx::Rect up_arrow_bounds = up_arrow_->bounds();
  const gfx::Rect down_arrow_bounds = down_arrow_->bounds();
  up_arrow_->SetEnabled(contents_view_->origin() !=
                        up_arrow_bounds.bottom_left());
  down_arrow_->SetEnabled(contents_view_->bounds().bottom_left() !=
                          down_arrow_bounds.origin());
}

bool SidebarItemsScrollView::IsScrollable() const {
  return bounds().height() < GetPreferredSize().height();
}

void SidebarItemsScrollView::OnButtonPressed(views::View* view) {
  const int scroll_offset = SidebarButtonView::kSidebarButtonSize;
  if (view == up_arrow_)
    ScrollContentsViewBy(scroll_offset);

  if (view == down_arrow_)
    ScrollContentsViewBy(-scroll_offset);

  UpdateArrowViewsState();
}

void SidebarItemsScrollView::ScrollContentsViewBy(int offset) {
  if (offset == 0)
    return;

  // If scroll goes up, it should not go further if the origin of contents view
  // is same with bottom_right of up arrow.
  if (offset > 0) {
    const gfx::Rect up_arrow_bounds = up_arrow_->bounds();
    // If contents view already stick to up_arrow bottom, just return.
    if (contents_view_->origin() == up_arrow_bounds.bottom_left())
      return;

    // If contents view top meets or exceeds the up arrow bottom, attach it to
    // up arrow bottom.
    if ((contents_view_->origin().y() + offset) >= up_arrow_bounds.bottom()) {
      ScrollContentsViewTo(true);
      return;
    }
  }

  // If scroll goes down, it should not go further if the bottom left of
  // contents view is same with origin of down arrow.
  if (offset < 0) {
    const gfx::Rect down_arrow_bounds = down_arrow_->bounds();
    // If contents view already stick to down_arrow top, just return.
    if (contents_view_->bounds().bottom_left() == down_arrow_bounds.origin())
      return;

    // If contents view bottom meets or exceeds the down arrow top, attach it to
    // down arrow top.
    if ((contents_view_->bounds().bottom() + offset) <= down_arrow_bounds.y()) {
      ScrollContentsViewTo(false);
      return;
    }
  }

  // Move contents view vertically for |offset|.
  contents_view_->SetPosition(
      {contents_view_->origin().x(), contents_view_->origin().y() + offset});
}

void SidebarItemsScrollView::ScrollContentsViewTo(bool top) {
  if (!IsScrollable())
    return;

  contents_view_->SizeToPreferredSize();

  const gfx::Rect bounds = GetContentsBounds();
  if (top) {
    const gfx::Rect up_arrow_bounds = up_arrow_->bounds();
    contents_view_->SetPosition({bounds.x(), up_arrow_bounds.bottom()});
  } else {
    const gfx::Rect down_arrow_bounds = down_arrow_->bounds();
    contents_view_->SetPosition(
        {bounds.x(), down_arrow_bounds.y() - contents_view_->height()});
  }
}
