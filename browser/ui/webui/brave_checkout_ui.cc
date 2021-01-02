/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/brave_checkout_ui.h"

#include <memory>
#include <utility>

#include "base/memory/weak_ptr.h"
#include "base/values.h"
#include "brave/browser/ui/webui/basic_ui.h"
#include "brave/browser/brave_rewards/rewards_service_factory.h"
#include "brave/components/brave_rewards/browser/rewards_service.h"
#include "brave/components/brave_rewards/browser/rewards_service_observer.h"
#include "brave/components/brave_rewards/resources/grit/brave_rewards_checkout_generated_map.h"
#include "brave/components/brave_rewards/resources/grit/brave_rewards_resources.h"
#include "chrome/browser/profiles/profile.h"
#include "content/public/browser/web_ui_data_source.h"
#include "content/public/browser/web_ui_message_handler.h"

namespace {

using brave_rewards::RewardsService;
using brave_rewards::RewardsServiceFactory;
using brave_rewards::RewardsServiceObserver;

class CheckoutMessageHandler : public content::WebUIMessageHandler {
 public:
  CheckoutMessageHandler();
  ~CheckoutMessageHandler() override;

  void RegisterMessages() override;

 private:
  RewardsService* GetRewardsService();

  // Message handlers
  void OnGetWalletBalance(const base::ListValue* args);
  void GetExternalWallet(const base::ListValue* args);

  // Rewards service callbacks
  void FetchBalanceCallback(const ledger::type::Result result,
                            ledger::type::BalancePtr balance);

  void GetUpholdWalletCallback(
      const ledger::type::Result result,
      ledger::type::UpholdWalletPtr wallet);

  RewardsService* rewards_service_ = nullptr;  // NOT OWNED
  base::WeakPtrFactory<CheckoutMessageHandler> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(CheckoutMessageHandler);
};

CheckoutMessageHandler::CheckoutMessageHandler() : weak_factory_(this) {}

CheckoutMessageHandler::~CheckoutMessageHandler() {
  // TODO(zenparsing): Remove observer
}

RewardsService* CheckoutMessageHandler::GetRewardsService() {
  if (!rewards_service_) {
    Profile* profile = Profile::FromWebUI(web_ui());
    rewards_service_ = RewardsServiceFactory::GetForProfile(profile);
    // TODO(zenparsing): Add observer
  }
  return rewards_service_;
}

void CheckoutMessageHandler::RegisterMessages() {
  web_ui()->RegisterMessageCallback(
      "getWalletBalance",
      base::BindRepeating(&CheckoutMessageHandler::OnGetWalletBalance,
                          base::Unretained(this)));

  web_ui()->RegisterMessageCallback(
      "getExternalWallet",
      base::BindRepeating(&CheckoutMessageHandler::GetExternalWallet,
                          base::Unretained(this)));
}

void CheckoutMessageHandler::OnGetWalletBalance(const base::ListValue* args) {
  if (auto* service = GetRewardsService()) {
    AllowJavascript();
    service->FetchBalance(
        base::BindOnce(&CheckoutMessageHandler::FetchBalanceCallback,
                       weak_factory_.GetWeakPtr()));
  }
}

void CheckoutMessageHandler::GetExternalWallet(const base::ListValue* args) {
  if (auto* service = GetRewardsService()) {
    AllowJavascript();
    service->GetUpholdWallet(
        base::BindOnce(&CheckoutMessageHandler::GetUpholdWalletCallback,
                       weak_factory_.GetWeakPtr()));
  }
}

void CheckoutMessageHandler::FetchBalanceCallback(
    const ledger::type::Result result,
    ledger::type::BalancePtr balance) {
  if (!IsJavascriptAllowed()) {
    return;
  }

  base::Value data(base::Value::Type::DICTIONARY);
  data.SetIntKey("status", static_cast<int>(result));

  if (result == ledger::type::Result::LEDGER_OK && balance) {
    base::Value wallets(base::Value::Type::DICTIONARY);
    for (const auto& wallet : balance->wallets) {
      wallets.SetDoubleKey(wallet.first, wallet.second);
    }

    base::Value balance_value(base::Value::Type::DICTIONARY);
    balance_value.SetDoubleKey("total", balance->total);
    balance_value.SetKey("wallets", std::move(wallets));

    data.SetKey("balance", std::move(balance_value));
  }

  FireWebUIListener("balanceUpdated", data);
}

void CheckoutMessageHandler::GetUpholdWalletCallback(
    const ledger::type::Result result,
    ledger::type::UpholdWalletPtr wallet) {
  if (!IsJavascriptAllowed()) {
    return;
  }

  base::Value data(base::Value::Type::DICTIONARY);

  if (wallet) {
    data.SetStringKey("token", wallet->token);
    data.SetStringKey("address", wallet->address);
    data.SetStringKey("verifyUrl", wallet->verify_url);
    data.SetStringKey("addUrl", wallet->add_url);
    data.SetStringKey("withdrawUrl", wallet->withdraw_url);
    data.SetStringKey("userName", wallet->user_name);
    data.SetStringKey("accountUrl", wallet->account_url);
    data.SetStringKey("loginUrl", wallet->login_url);
    data.SetIntKey("status", static_cast<int>(wallet->status));
  }

  FireWebUIListener("externalWalletUpdated", data);
}

}  // namespace

BraveCheckoutUI::BraveCheckoutUI(content::WebUI* web_ui,
                                 const std::string& name)
    : ConstrainedWebDialogUI(web_ui) {
  // TODO(zenparsing): Handle profile->IsOffTheRecord()?
  Profile* profile = Profile::FromWebUI(web_ui);

  content::WebUIDataSource::Add(
      profile,
      CreateBasicUIHTMLSource(profile, name, kBraveRewardsCheckoutGenerated,
                              kBraveRewardsCheckoutGeneratedSize,
                              IDR_BRAVE_REWARDS_CHECKOUT_HTML));

  web_ui->AddMessageHandler(std::make_unique<CheckoutMessageHandler>());
}

BraveCheckoutUI::~BraveCheckoutUI() {}
