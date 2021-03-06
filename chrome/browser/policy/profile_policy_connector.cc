// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/policy/profile_policy_connector.h"

#include <vector>

#include "base/logging.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/policy/browser_policy_connector.h"
#include "chrome/browser/policy/cloud/cloud_policy_manager.h"
#include "chrome/browser/policy/configuration_policy_provider.h"
#include "chrome/browser/policy/policy_service.h"

#if defined(OS_CHROMEOS)
#include "base/bind.h"
#include "base/prefs/pref_service.h"
#include "chrome/browser/chromeos/login/user.h"
#include "chrome/browser/chromeos/login/user_manager.h"
#include "chrome/browser/chromeos/policy/device_local_account_policy_provider.h"
#include "chrome/browser/chromeos/policy/login_profile_policy_provider.h"
#include "chrome/browser/chromeos/policy/user_network_configuration_updater.h"
#include "chrome/browser/policy/policy_service.h"
#include "chrome/common/pref_names.h"
#include "chromeos/network/network_handler.h"
#include "chromeos/network/onc/onc_certificate_importer_impl.h"
#endif

namespace policy {

ProfilePolicyConnector::ProfilePolicyConnector(Profile* profile)
    :
#if defined(OS_CHROMEOS)
      is_primary_user_(false),
      weak_ptr_factory_(this),
#endif
      profile_(profile) {}

ProfilePolicyConnector::~ProfilePolicyConnector() {}

void ProfilePolicyConnector::Init(
    bool force_immediate_load,
#if defined(OS_CHROMEOS)
    const chromeos::User* user,
#endif
    CloudPolicyManager* user_cloud_policy_manager) {
  BrowserPolicyConnector* connector =
      g_browser_process->browser_policy_connector();
  // |providers| contains a list of the policy providers available for the
  // PolicyService of this connector.
  std::vector<ConfigurationPolicyProvider*> providers;

  if (user_cloud_policy_manager)
    providers.push_back(user_cloud_policy_manager);

#if defined(OS_CHROMEOS)
  bool allow_trusted_certs_from_policy = false;
  if (!user) {
    // This case occurs for the signin profile.
    special_user_policy_provider_.reset(
        new LoginProfilePolicyProvider(connector->GetPolicyService()));
    special_user_policy_provider_->Init();
  } else {
    // |user| should never be NULL except for the signin profile.
    is_primary_user_ = user == chromeos::UserManager::Get()->GetPrimaryUser();
    if (user->GetType() == chromeos::User::USER_TYPE_PUBLIC_ACCOUNT)
      InitializeDeviceLocalAccountPolicyProvider(user->email());
    // Allow trusted certs from policy only for managed regular accounts.
    const bool is_managed = connector->GetUserAffiliation(user->email()) ==
                            USER_AFFILIATION_MANAGED;
    if (is_managed && user->GetType() == chromeos::User::USER_TYPE_REGULAR)
      allow_trusted_certs_from_policy = true;
  }
  if (special_user_policy_provider_)
    providers.push_back(special_user_policy_provider_.get());
#endif

  policy_service_ = connector->CreatePolicyService(providers);

#if defined(OS_CHROMEOS)
  if (is_primary_user_) {
    if (user_cloud_policy_manager)
      connector->SetUserPolicyDelegate(user_cloud_policy_manager);
    else if (special_user_policy_provider_)
      connector->SetUserPolicyDelegate(special_user_policy_provider_.get());

    network_configuration_updater_ =
        UserNetworkConfigurationUpdater::CreateForUserPolicy(
            allow_trusted_certs_from_policy,
            *user,
            scoped_ptr<chromeos::onc::CertificateImporter>(
                new chromeos::onc::CertificateImporterImpl),
            policy_service(),
            chromeos::NetworkHandler::Get()
                ->managed_network_configuration_handler());
  }
#endif
}

void ProfilePolicyConnector::InitForTesting(scoped_ptr<PolicyService> service) {
  policy_service_ = service.Pass();
}

void ProfilePolicyConnector::Shutdown() {
#if defined(OS_CHROMEOS)
  if (is_primary_user_)
    g_browser_process->browser_policy_connector()->SetUserPolicyDelegate(NULL);
  network_configuration_updater_.reset();
  if (special_user_policy_provider_)
    special_user_policy_provider_->Shutdown();
#endif
}

#if defined(OS_CHROMEOS)
void ProfilePolicyConnector::SetPolicyCertVerifier(
    PolicyCertVerifier* cert_verifier) {
  if (network_configuration_updater_)
    network_configuration_updater_->SetPolicyCertVerifier(cert_verifier);
}

base::Closure ProfilePolicyConnector::GetPolicyCertTrustedCallback() {
  return base::Bind(&ProfilePolicyConnector::SetUsedPolicyCertificatesOnce,
                    weak_ptr_factory_.GetWeakPtr());
}

void ProfilePolicyConnector::GetWebTrustedCertificates(
    net::CertificateList* certs) const {
  certs->clear();
  if (network_configuration_updater_)
    network_configuration_updater_->GetWebTrustedCertificates(certs);
}
#endif

bool ProfilePolicyConnector::UsedPolicyCertificates() {
#if defined(OS_CHROMEOS)
  return profile_->GetPrefs()->GetBoolean(prefs::kUsedPolicyCertificatesOnce);
#else
  return false;
#endif
}

#if defined(OS_CHROMEOS)
void ProfilePolicyConnector::SetUsedPolicyCertificatesOnce() {
  profile_->GetPrefs()->SetBoolean(prefs::kUsedPolicyCertificatesOnce, true);
}

void ProfilePolicyConnector::InitializeDeviceLocalAccountPolicyProvider(
    const std::string& username) {
  BrowserPolicyConnector* connector =
      g_browser_process->browser_policy_connector();
  DeviceLocalAccountPolicyService* device_local_account_policy_service =
      connector->GetDeviceLocalAccountPolicyService();
  if (!device_local_account_policy_service)
    return;
  special_user_policy_provider_.reset(new DeviceLocalAccountPolicyProvider(
      username, device_local_account_policy_service));
  special_user_policy_provider_->Init();
}
#endif

}  // namespace policy
