// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/extension_browsertest.h"

#include <vector>

#include "base/command_line.h"
#include "base/file_path.h"
#include "base/path_service.h"
#include "chrome/browser/browser.h"
#include "chrome/browser/browser_window.h"
#include "chrome/browser/location_bar.h"
#include "chrome/browser/extensions/extension_host.h"
#include "chrome/browser/extensions/extensions_service.h"
#include "chrome/browser/profile.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/common/extensions/extension_error_reporter.h"
#include "chrome/common/notification_registrar.h"
#include "chrome/common/notification_service.h"
#include "chrome/common/notification_type.h"
#include "chrome/test/ui_test_utils.h"

namespace {
// Amount of time to wait to load an extension. This is purposely obscenely
// long because it will only get used in the case of failure and we want to
// minimize false positives.
static const int kTimeoutMs = 60 * 1000;  // 1 minute
};

// Base class for extension browser tests. Provides utilities for loading,
// unloading, and installing extensions.
void ExtensionBrowserTest::SetUpCommandLine(CommandLine* command_line) {
  // This enables DOM automation for tab contentses.
  EnableDOMAutomation();

  // This enables it for extension hosts.
  ExtensionHost::EnableDOMAutomation();

  PathService::Get(chrome::DIR_TEST_DATA, &test_data_dir_);
  test_data_dir_ = test_data_dir_.AppendASCII("extensions");
}

bool ExtensionBrowserTest::LoadExtension(const FilePath& path) {
  ExtensionsService* service = browser()->profile()->GetExtensionsService();
  size_t num_before = service->extensions()->size();
  {
    NotificationRegistrar registrar;
    registrar.Add(this, NotificationType::EXTENSION_LOADED,
                  NotificationService::AllSources());
    service->LoadExtension(path);
    MessageLoop::current()->PostDelayedTask(
        FROM_HERE, new MessageLoop::QuitTask, kTimeoutMs);
    ui_test_utils::RunMessageLoop();
  }
  size_t num_after = service->extensions()->size();
  if (num_after != (num_before + 1))
    return false;

  return WaitForExtensionHostsToLoad();
}

bool ExtensionBrowserTest::InstallOrUpdateExtension(
    const std::string& id, const FilePath& path, int expected_change) {
  ExtensionsService* service = browser()->profile()->GetExtensionsService();
  service->set_show_extensions_prompts(false);
  size_t num_before = service->extensions()->size();

  {
    NotificationRegistrar registrar;
    registrar.Add(this, NotificationType::EXTENSION_LOADED,
                  NotificationService::AllSources());
    registrar.Add(this, NotificationType::EXTENSION_UPDATE_DISABLED,
                  NotificationService::AllSources());

    if (!id.empty()) {
      // We need to copy this to a temporary location because Update() will
      // delete it.
      FilePath temp_dir;
      PathService::Get(base::DIR_TEMP, &temp_dir);
      FilePath copy = temp_dir.Append(path.BaseName());
      file_util::CopyFile(path, copy);
      service->UpdateExtension(id, copy);
    } else {
      service->InstallExtension(path);
    }

    MessageLoop::current()->PostDelayedTask(
        FROM_HERE, new MessageLoop::QuitTask, kTimeoutMs);
    ui_test_utils::RunMessageLoop();
  }

  size_t num_after = service->extensions()->size();
  if (num_after != (num_before + expected_change)) {
    std::cout << "Num extensions before: " << IntToString(num_before) << " "
              << "num after: " << IntToString(num_after) << " "
              << "Installed extensions follow:\n";

    for (size_t i = 0; i < service->extensions()->size(); ++i)
      std::cout << "  " << service->extensions()->at(i)->id() << "\n";

    std::cout << "Errors follow:\n";
    const std::vector<std::string>* errors =
        ExtensionErrorReporter::GetInstance()->GetErrors();
    for (std::vector<std::string>::const_iterator iter = errors->begin();
         iter != errors->end(); ++iter) {
      std::cout << *iter << "\n";
    }

    return false;
  }

  return WaitForExtensionHostsToLoad();
}

void ExtensionBrowserTest::UnloadExtension(const std::string& extension_id) {
  ExtensionsService* service = browser()->profile()->GetExtensionsService();
  service->UnloadExtension(extension_id);
}

void ExtensionBrowserTest::UninstallExtension(const std::string& extension_id) {
  ExtensionsService* service = browser()->profile()->GetExtensionsService();
  service->UninstallExtension(extension_id, false);
}

bool ExtensionBrowserTest::WaitForPageActionCountChangeTo(int count) {
  base::Time start_time = base::Time::Now();
  while (true) {
    LocationBarTesting* loc_bar =
        browser()->window()->GetLocationBar()->GetLocationBarForTesting();

    int actions = loc_bar->PageActionCount();
    if (actions == count)
      return true;

    if ((base::Time::Now() - start_time).InMilliseconds() > kTimeoutMs) {
      std::cout << "Timed out waiting for page actions to (un)load.\n"
                << "Currently loaded page actions: " << IntToString(actions)
                << "\n";
      return false;
    }

    MessageLoop::current()->PostDelayedTask(FROM_HERE,
                                            new MessageLoop::QuitTask, 200);
    ui_test_utils::RunMessageLoop();
  }
}

bool ExtensionBrowserTest::WaitForPageActionVisibilityChangeTo(int count) {
  base::Time start_time = base::Time::Now();
  while (true) {
    LocationBarTesting* loc_bar =
        browser()->window()->GetLocationBar()->GetLocationBarForTesting();

    int visible = loc_bar->PageActionVisibleCount();
    if (visible == count)
      return true;

    if ((base::Time::Now() - start_time).InMilliseconds() > kTimeoutMs) {
      std::cout << "Timed out waiting for page actions to become (in)visible.\n"
                << "Currently visible page actions: " << IntToString(visible)
                << "\n";
      return false;
    }

    MessageLoop::current()->PostDelayedTask(FROM_HERE,
                                            new MessageLoop::QuitTask, 200);
    ui_test_utils::RunMessageLoop();
  }
}

bool ExtensionBrowserTest::WaitForExtensionHostsToLoad() {
  // Wait for all the extension hosts that exist to finish loading.
  // NOTE: This assumes that the extension host list is not changing while
  // this method is running.

  NotificationRegistrar registrar;
  registrar.Add(this, NotificationType::EXTENSION_HOST_DID_STOP_LOADING,
                NotificationService::AllSources());

  ExtensionProcessManager* manager =
        browser()->profile()->GetExtensionProcessManager();
  for (ExtensionProcessManager::const_iterator iter = manager->begin();
       iter != manager->end();) {
    if ((*iter)->did_stop_loading())
      ++iter;
    else
      ui_test_utils::RunMessageLoop();
  }
  LOG(INFO) << "All ExtensionHosts loaded";

  return true;
}

void ExtensionBrowserTest::Observe(NotificationType type,
                                   const NotificationSource& source,
                                   const NotificationDetails& details) {
  switch (type.value) {
    case NotificationType::EXTENSION_LOADED:
      last_loaded_extension_id_ = Details<Extension>(details).ptr()->id();
      std::cout << "Got EXTENSION_LOADED notification.\n";
      MessageLoopForUI::current()->Quit();
      break;

    case NotificationType::EXTENSION_UPDATE_DISABLED:
      std::cout << "Got EXTENSION_UPDATE_DISABLED notification.\n";
      MessageLoopForUI::current()->Quit();
      break;

    case NotificationType::EXTENSION_HOST_DID_STOP_LOADING:
      std::cout << "Got EXTENSION_HOST_DID_STOP_LOADING notification.\n";
      MessageLoopForUI::current()->Quit();
      break;

    default:
      NOTREACHED();
      break;
  }
}
