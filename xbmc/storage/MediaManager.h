/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "IDiscDriveHandler.h"
#include "IStorageProvider.h"
#include "MediaSource.h" // for VECSOURCES
#include "threads/CriticalSection.h"
#include "utils/DiscsUtils.h"
#include "utils/Job.h"

#include <map>
#include <vector>
#include <memory>

#include "PlatformDefs.h"

class CFileItem;

class CNetworkLocation
{
public:
  CNetworkLocation() { id = 0; }
  int id;
  std::string path;
};

class CMediaManager : public IStorageEventsCallback, public IJobCallback
{
public:
  CMediaManager();

  void Initialize();
  void Stop();

  bool LoadSources();
  bool SaveSources();

  void GetLocalDrives(VECSOURCES &localDrives, bool includeQ = true);
  void GetRemovableDrives(VECSOURCES &removableDrives);
  void GetNetworkLocations(VECSOURCES &locations, bool autolocations = true);

  bool AddNetworkLocation(const std::string &path);
  bool HasLocation(const std::string& path) const;
  bool RemoveLocation(const std::string& path);
  bool SetLocationPath(const std::string& oldPath, const std::string& newPath);

  void AddAutoSource(const CMediaSource &share, bool bAutorun=false);
  void RemoveAutoSource(const CMediaSource &share);
  bool IsDiscInDrive(const std::string& devicePath="");
  bool IsAudio(const std::string& devicePath="");
  bool HasOpticalDrive();
  std::string TranslateDevicePath(const std::string& devicePath, bool bReturnAsDevice=false);
  DriveState GetDriveStatus(const std::string& devicePath="");
#ifdef HAS_DVD_DRIVE
  MEDIA_DETECT::CCdInfo* GetCdInfo(const std::string& devicePath="");
  bool RemoveCdInfo(const std::string& devicePath="");
  std::string GetDiskLabel(const std::string& devicePath="");
  std::string GetDiskUniqueId(const std::string& devicePath="");
#endif
  std::string GetDiscPath();
  void SetHasOpticalDrive(bool bstatus);

  bool Eject(const std::string& mountpath);

  std::shared_ptr<IDiscDriveHandler> GetDiscDriveHandler();

  void ProcessEvents();

  std::vector<std::string> GetDiskUsage();

  /*! \brief Callback executed when a new storage device is added
    * \sa IStorageEventsCallback
    * @param device the storage device
  */
  void OnStorageAdded(const MEDIA_DETECT::STORAGE::StorageDevice& device) override;

  /*! \brief Callback executed when a new storage device is safely removed
    * \sa IStorageEventsCallback
    * @param device the storage device
  */
  void OnStorageSafelyRemoved(const MEDIA_DETECT::STORAGE::StorageDevice& device) override;

  /*! \brief Callback executed when a new storage device is unsafely removed
    * \sa IStorageEventsCallback
    * @param device the storage device
  */
  void OnStorageUnsafelyRemoved(const MEDIA_DETECT::STORAGE::StorageDevice& device) override;

  void OnJobComplete(unsigned int jobID, bool success, CJob *job) override { }

  bool playStubFile(const CFileItem& item);

protected:
  std::vector<CNetworkLocation> m_locations;

  CCriticalSection m_muAutoSource, m_CritSecStorageProvider;
#ifdef HAS_DVD_DRIVE
  std::map<std::string,MEDIA_DETECT::CCdInfo*> m_mapCdInfo;
#endif
  bool m_bhasoptical;
  std::string m_strFirstAvailDrive;

private:
  IStorageProvider *m_platformStorage;
  std::shared_ptr<IDiscDriveHandler> m_discDriveHander = nullptr;

  UTILS::DISCS::DiscInfo GetDiscInfo(const std::string& mediaPath);
  void RemoveDiscInfo(const std::string& devicePath);
  std::map<std::string, UTILS::DISCS::DiscInfo> m_mapDiscInfo;
};
