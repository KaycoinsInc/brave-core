/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_SIDEBAR_SIDEBAR_MODEL_H_
#define BRAVE_BROWSER_UI_SIDEBAR_SIDEBAR_MODEL_H_

#include <memory>
#include <vector>

#include "base/observer_list.h"
#include "base/optional.h"
#include "base/scoped_observer.h"
#include "brave/browser/ui/sidebar/sidebar_model_data.h"
#include "brave/components/sidebar/sidebar_service.h"

class Profile;

namespace sidebar {

class SidebarModelData;

// Manage sidebar's runtime state.
// Each browser window has different runtime state.
// Observe SidebarService to get item add/deletion notification.
class SidebarModel : SidebarService::Observer {
 public:
  class Observer : public base::CheckedObserver {
   public:
    virtual void OnItemAdded(const SidebarItem& item,
                             int index,
                             bool user_gesture) {}
    virtual void OnItemRemoved(int index) {}
    virtual void OnActiveIndexChanged(int old_index, int new_index) {}

   protected:
    ~Observer() override = default;
  };

  explicit SidebarModel(Profile* profile);
  ~SidebarModel() override;

  SidebarModel(const SidebarModel&) = delete;
  SidebarModel& operator=(const SidebarModel&) = delete;

  void Init();

  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);

  void SetActiveIndex(int index);
  // Returns true if webcontents of item at |index| already loaded url.
  bool IsLoadedAt(int index) const;
  bool IsSidebarHasAllBuiltiInItems() const;
  int GetIndexOf(const SidebarItem& item) const;

  // Don't cache web_contents. It can be deleted during the runtime.
  content::WebContents* GetWebContentsAt(int index);

  // Don't cache item list. list can be changed during the runtime.
  const std::vector<SidebarItem> GetAllSidebarItems() const;

  // Return -1 if sidebar panel is not opened.
  int active_index() const { return active_index_; }

  // SidebarService::Observer overrides:
  void OnItemAdded(const SidebarItem& item, int index) override;
  void OnWillRemoveItem(const SidebarItem& item, int index) override;
  void OnItemRemoved(const SidebarItem& item, int index) override;

 private:
  // Add item at last.
  void AddItem(const SidebarItem& item, int index, bool user_gesture);
  void RemoveItemAt(int index);
  void UpdateActiveIndexAndNotify(int new_active_index);
  void LoadURLAt(const GURL& url, int index);

  // Non-negative if sidebar panel is opened.
  int active_index_ = -1;
  Profile* profile_ = nullptr;
  base::ObserverList<Observer> observers_;
  std::vector<std::unique_ptr<SidebarModelData>> data_;
  ScopedObserver<SidebarService, SidebarService::Observer> observed_{this};
};

}  // namespace sidebar

#endif  // BRAVE_BROWSER_UI_SIDEBAR_SIDEBAR_MODEL_H_
