/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_SIDEBAR_SIDEBAR_ITEMS_SCROLL_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_SIDEBAR_SIDEBAR_ITEMS_SCROLL_VIEW_H_

#include "base/memory/weak_ptr.h"
#include "brave/browser/ui/sidebar/sidebar_model.h"
#include "ui/views/view.h"

namespace views {
class ImageButton;
}  // namespace views

class BraveBrowser;
class SidebarItemsContentsView;

class SidebarItemsScrollView : public views::View,
                               public sidebar::SidebarModel::Observer {
 public:
  explicit SidebarItemsScrollView(BraveBrowser* browser);
  ~SidebarItemsScrollView() override;

  SidebarItemsScrollView(const SidebarItemsScrollView&) = delete;
  SidebarItemsScrollView operator=(const SidebarItemsScrollView&) = delete;

  // views::View overrides:
  void Layout() override;
  void OnMouseEvent(ui::MouseEvent* event) override;
  gfx::Size CalculatePreferredSize() const override;
  void OnThemeChanged() override;

  // sidebar::SidebarModel::Observer overrides:
  void OnItemAdded(const sidebar::SidebarItem& item,
                   int index,
                   bool user_gesture) override;

 private:
  void UpdateArrowViewsTheme();
  void UpdateArrowViewsState();
  // Return true if scroll view's area doesn't have enough bounds to show whole
  // contents view.
  bool IsScrollable() const;

  void OnButtonPressed(views::View* view);

  // With true, top of contents view is attached to bottom of up arrow.
  // Otherwise, bottom of contents view is attached to top of down arrow.
  void ScrollContentsViewTo(bool top);
  void ScrollContentsViewBy(int offset);
  void ScrollToBottomAfterNewItemAdded();

  BraveBrowser* browser_ = nullptr;
  views::ImageButton* up_arrow_ = nullptr;
  views::ImageButton* down_arrow_ = nullptr;
  SidebarItemsContentsView* contents_view_ = nullptr;
  ScopedObserver<sidebar::SidebarModel, sidebar::SidebarModel::Observer>
      observed_{this};
  base::WeakPtrFactory<SidebarItemsScrollView> weak_ptr_factory_{this};
};

#endif  // BRAVE_BROWSER_UI_VIEWS_SIDEBAR_SIDEBAR_ITEMS_SCROLL_VIEW_H_
