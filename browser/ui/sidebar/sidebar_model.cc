/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/sidebar/sidebar_model.h"

#include <string>

#include "base/logging.h"
#include "brave/browser/ui/sidebar/sidebar_model_data.h"
#include "brave/browser/ui/sidebar/sidebar_service_factory.h"
#include "brave/components/sidebar/sidebar_item.h"

namespace sidebar {

namespace {

SidebarService* GetSidebarService(Profile* profile) {
  auto* service = SidebarServiceFactory::GetForProfile(profile);
  DCHECK(service);

  return service;
}

}  // namespace

SidebarModel::SidebarModel(Profile* profile) : profile_(profile) {}

SidebarModel::~SidebarModel() = default;

void SidebarModel::Init() {
  // Start with saved item list.
  for (const auto& item : GetAllSidebarItems())
    AddItem(item, -1, false);

  observed_.Add(GetSidebarService(profile_));
}

void SidebarModel::AddObserver(Observer* observer) {
  observers_.AddObserver(observer);
}

void SidebarModel::RemoveObserver(Observer* observer) {
  observers_.RemoveObserver(observer);
}

void SidebarModel::AddItem(const SidebarItem& item,
                           int index,
                           bool user_gesture) {
  data_.push_back(std::make_unique<SidebarModelData>(profile_));
  for (Observer& obs : observers_) {
    // Index starts at zero. If |index| is -1, add as a last item.
    obs.OnItemAdded(item, index == -1 ? data_.size() - 1 : index, user_gesture);
  }

  // If active_index_ is not -1, check this addition affetcs active index.
  if (active_index_ != -1 && active_index_ >= index)
    UpdateActiveIndexAndNotify(index);
}

void SidebarModel::OnItemAdded(const SidebarItem& item, int index) {
  AddItem(item, index, true);
}

void SidebarModel::OnWillRemoveItem(const SidebarItem& item, int index) {
  if (index == active_index_)
    UpdateActiveIndexAndNotify(-1);
}

void SidebarModel::OnItemRemoved(const SidebarItem& item, int index) {
  RemoveItemAt(index);
}

void SidebarModel::RemoveItemAt(int index) {
  data_.erase(data_.begin() + index);
  for (Observer& obs : observers_)
    obs.OnItemRemoved(index);

  if (active_index_ > index) {
    active_index_--;
    UpdateActiveIndexAndNotify(active_index_);
  }
}

void SidebarModel::SetActiveIndex(int index) {
  if (index == active_index_)
    return;

  // Don't load url if it's already loaded. If not, new loading is started
  // whenever item is activated.
  // TODO(simonhong): Maybe we should have reload option?
  if (index != -1 && !IsLoadedAt(index))
    LoadURLAt(GetAllSidebarItems()[index].url, index);

  UpdateActiveIndexAndNotify(index);
}

content::WebContents* SidebarModel::GetWebContentsAt(int index) {
  // Only webcontents is requested for items that opens in panel.
  // Opens in new tab doesn't need to get webcontents here.
  DCHECK(GetAllSidebarItems()[index].open_in_panel);

  return data_[index]->GetWebContents();
}

const std::vector<SidebarItem> SidebarModel::GetAllSidebarItems() const {
  return GetSidebarService(profile_)->items();
}

bool SidebarModel::IsLoadedAt(int index) const {
  DCHECK(GetAllSidebarItems()[index].open_in_panel);

  return data_[index]->IsLoaded();
}

bool SidebarModel::IsSidebarHasAllBuiltiInItems() const {
  return GetSidebarService(profile_)->GetNotAddedDefaultSidebarItems().empty();
}

int SidebarModel::GetIndexOf(const SidebarItem& item) const {
  const auto items = GetAllSidebarItems();
  const auto iter =
      std::find_if(items.begin(), items.end(),
                   [item](const auto& i) { return item.url == i.url; });
  if (iter == items.end())
    return -1;

  return std::distance(items.begin(), iter);
}

void SidebarModel::LoadURLAt(const GURL& url, int index) {
  DCHECK(GetAllSidebarItems()[index].open_in_panel);

  data_[index]->LoadURL(url);
}

void SidebarModel::UpdateActiveIndexAndNotify(int new_active_index) {
  const int old_active_index = active_index_;
  active_index_ = new_active_index;

  for (Observer& obs : observers_)
    obs.OnActiveIndexChanged(old_active_index, active_index_);
}

}  // namespace sidebar
