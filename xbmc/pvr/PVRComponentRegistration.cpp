/*
 *  Copyright (C) 2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PVRComponentRegistration.h"

#include "pvr/guilib/PVRGUIActionsChannels.h"
#include "pvr/guilib/PVRGUIActionsClients.h"

#include <memory>

using namespace PVR;

CPVRComponentRegistration::CPVRComponentRegistration()
{
  RegisterComponent(std::make_shared<CPVRGUIActionsChannels>());
  RegisterComponent(std::make_shared<CPVRGUIActionsClients>());
}

CPVRComponentRegistration::~CPVRComponentRegistration()
{
  DeregisterComponent(typeid(CPVRGUIActionsClients));
  DeregisterComponent(typeid(CPVRGUIActionsChannels));
}
