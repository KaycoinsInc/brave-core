/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_SIDEBAR_SIDEBAR_ITEMS_CONTENTS_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_SIDEBAR_SIDEBAR_ITEMS_CONTENTS_VIEW_H_

#include <memory>

#include "base/memory/weak_ptr.h"
#include "base/scoped_observer.h"
#include "brave/browser/ui/sidebar/sidebar_model.h"
#include "brave/browser/ui/views/sidebar/sidebar_button_view.h"
#include "brave/components/sidebar/sidebar_item.h"
#include "ui/base/models/simple_menu_model.h"
#include "ui/views/context_menu_controller.h"
#include "ui/views/view.h"

namespace base {
class CancelableTaskTracker;
}  // namespace base

namespace favicon_base {
struct FaviconRawBitmapResult;
}  // namespace favicon_base

namespace views {
class MenuRunner;
}  // namespace views

class BraveBrowser;

class SidebarItemsContentsView : public views::View,
                                 public SidebarButtonView::Delegate,
                                 public views::ContextMenuController,
                                 public ui::SimpleMenuModel::Delegate,
                                 public sidebar::SidebarModel::Observer {
 public:
  explicit SidebarItemsContentsView(BraveBrowser* browser);
  ~SidebarItemsContentsView() override;

  SidebarItemsContentsView(const SidebarItemsContentsView&) = delete;
  SidebarItemsContentsView operator=(const SidebarItemsContentsView&) = delete;

  // views::View overrides:
  gfx::Size CalculatePreferredSize() const override;
  void OnThemeChanged() override;

  // SidebarButtonView::Delegate overrides:
  base::string16 GetTooltipTextFor(const views::View* view) const override;

  // views::ContextMenuController overrides:
  void ShowContextMenuForViewImpl(views::View* source,
                                  const gfx::Point& point,
                                  ui::MenuSourceType source_type) override;

  // ui::SimpleMenuModel::Delegate overrides:
  void ExecuteCommand(int command_id, int event_flags) override;

  // sidebar::SidebarModel::Observer overrides:
  void OnItemAdded(const sidebar::SidebarItem& item,
                   int index,
                   bool user_gesture) override;
  void OnItemRemoved(int index) override;
  void OnActiveIndexChanged(int old_index, int new_index) override;

 private:
  enum ContextMenuIDs {
    kItemRemove,
  };

  void AddItemView(
      const sidebar::SidebarItem& item, int index, bool user_gesture);
  void UpdateItemViewStateAt(int index, bool active);

  bool IsBuiltInTypeItemView(views::View* view) const;

  // Called when each item is pressed.
  void OnItemPressed(const views::View* item);
  void FetchFavicon(const sidebar::SidebarItem& item, int index);
  void OnGetFaviconImage(
      const sidebar::SidebarItem& item,
      const favicon_base::FaviconRawBitmapResult& bitmapt_result);

  void OnContextMenuClosed();

  gfx::ImageSkia GetImageForBuiltInItems(const GURL& item_url,
                                         bool focus) const;
  void UpdateAllBuiltInItemsViewState();
  void ShowItemAddedFeedbackBubble(views::View* anchor_view);

  BraveBrowser* browser_ = nullptr;
  views::View* view_for_context_menu_ = nullptr;
  sidebar::SidebarModel* sidebar_model_ = nullptr;
  std::unique_ptr<base::CancelableTaskTracker> task_tracker_;
  std::unique_ptr<ui::SimpleMenuModel> context_menu_model_;
  std::unique_ptr<views::MenuRunner> context_menu_runner_;
  ScopedObserver<sidebar::SidebarModel, sidebar::SidebarModel::Observer>
      observed_{this};
  base::WeakPtrFactory<SidebarItemsContentsView> weak_ptr_factory_{this};
};

#endif  // BRAVE_BROWSER_UI_VIEWS_SIDEBAR_SIDEBAR_ITEMS_CONTENTS_VIEW_H_
