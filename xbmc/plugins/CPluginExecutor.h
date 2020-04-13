/*
 *  Copyright (C) 2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "messaging/IMessageTarget.h"
#include "plugins/actions/AsyncGetPluginResult.h"
#include "threads/CriticalSection.h"

namespace PLUGIN
{
class CPluginExecutor : public KODI::MESSAGING::IMessageTarget
{
public:
  CPluginExecutor();
  ~CPluginExecutor();

  void Initialize();
  
  bool IsBusy() const;

  int GetMessageMask() override;
  void OnApplicationMessage(KODI::MESSAGING::ThreadMessage* pMsg) override;

private:
  
  template<typename F>
  void Submit(const std::string& path, AsyncGetPluginResult* action, F& sucessCallback);
  
  void StopAll() const;
  
  void Add(AsyncGetPluginResult* action);
  
  void OnExecutionEnded(AsyncGetPluginResult* action);
  
  std::vector<AsyncGetPluginResult*> m_actions;
  
  mutable CCriticalSection m_criticalsection;
  
};
} // namespace PLUGIN
