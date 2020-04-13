/*
*  Copyright (C) 2020 Team Kodi
*  This file is part of Kodi - https://kodi.tv
*
*  SPDX-License-Identifier: GPL-2.0-or-later
*  See LICENSES/README.md for more information.
*/
#pragma once

#include "threads/Event.h"
#include "threads/IRunnable.h"

#include <atomic>

class CFileItem;

namespace XFILE
{
class CPluginDirectory;
}

namespace PLUGIN
{

/*!
 \brief AsyncGetPluginResultAction is an async action to resolve plugin paths (e.g. derive the
 dynpath playable path from the raw path of a plugin:// based fileitem). It is intended to be driven
 by busy loaders (e.g. CGUIDialogBusy) and can also be executed syncronously on the callee thread
 (see \ref AsyncGetPluginResultAction_ExecuteSynchronously "ExecuteSynchronously"). It is usefull
 since plugins are written in python and kodi has no control over the addon code or the time it
 takes to execute. Hence, if the core execution depends on a plugin call, this call should be
 wrapped in some busyloader that drives the render loop and allows the cancelation of its builtin
 execution
*/
class AsyncGetPluginResult : public IRunnable
{
public:
  /*!
  \brief No default constructor
  */
  AsyncGetPluginResult() = delete;

  /*!
  \brief Creates the plugin resolver action
  \param strPath[in] The plugin path
  \param resultItem[in,out] The CFIleitem that wll be filled with the result of the strPath
  execution \param resume[in] if resume should be passed to the script that will be executed as an
  argument
  */
  AsyncGetPluginResult(CFileItem& item, const bool resume);

  /*!
  \brief Checks if the async execution had success i.e., if the plugin (and ihnerently the script)
  executed sucessfully and the respective CFileItem is properly filled \return true if the execution
  was successfull
  */
  bool ExecutionSuceeded() const;
  
  bool IsCancelled() const;
  
  bool ResolutionSuceeded() const;

  /*!
  \brief Cancels the async operation
  */
  void Cancel() override;

  /*!
  \brief Executes this async action syncronously, waiting for the result
  \anchor AsyncGetPluginResultAction_ExecuteSynchronously
  \return true if the execution was successfull
  */
  bool ExecuteAndWait();

private:
  /*!
  \brief IRunnable implementation
  */
  void Run() override;

  /*!
  \brief Flags the async event, sets the success variables and updates the result item (called
  internally as part of Run)
  */
  void Finish();

  // propagated for script execution
  CFileItem* m_item;
  bool m_resume = false;

  // part of future/async
  CEvent m_event;
  std::atomic<bool> m_success;
  std::atomic<bool> m_cancelled;

  // handler for CPluginDirectory static function calls
  XFILE::CPluginDirectory* m_pluginDirHandler;
};
} // namespace PLUGIN
