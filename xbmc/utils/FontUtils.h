/*
 *  Copyright (C) 2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <stdint.h>
#include <string>
#include <vector>

namespace UTILS
{
namespace FONT
{
constexpr const char* SUPPORTED_EXTENSIONS_MASK = ".ttf|.otf";

// The default application font
constexpr const char* FONT_DEFAULT_FILENAME = "arial.ttf";

namespace FONTPATH
{
// Temporary font path (where MKV fonts are extracted and temporarily stored)
constexpr std::string_view TEMP = "special://temp/fonts/";
// Directory where user defined fonts are located
constexpr std::string_view USER = "special://home/media/Fonts/";
// Directory where Kodi bundled fonts files are located
constexpr std::string_view SYSTEM = "special://xbmc/media/Fonts/";

/*!
    *  \brief Provided a font path, translates and returns the
    *  final path
    *  \param fontPath The font path
    *  \return The translated font path
    */
std::string GetTranslatedFontPath(const std::string_view& fontPath);

/*!
    *  \brief Provided a font filename returns the complete path for the font in the
    *  system font folder (if it exists) or an empty string otherwise
    *  \param filename The font file name
    *  \return The path for the font or an empty string if the path does not exist
    */
std::string GetSystemFontPath(const std::string& filename);
}; // namespace FONTPATH

/*!
 *  \brief Get the font family name from a font file
 *  \param filepath The path where read the font data
 *  \param buffer [OUT] The font data read
 *  \return The font family name, otherwise empty if fails
 */
std::string GetFontFamily(const std::string& filepath, std::vector<uint8_t>& buffer);

/*!
 *  \brief Get the font family name from a font file
 *  \param filepath The path where read the font data
 *  \return The font family name, otherwise empty if fails
 */
std::string GetFontFamily(const std::string& filepath);

/*!
 *  \brief Check if a filename have a supported font extension.
 *  \param filepath The font file path
 *  \return True if it has a supported extension, false otherwise
 */
bool IsSupportedFontExtension(const std::string& filepath);

/*!
 *  \brief Removes all temporary fonts, e.g. those extract from mkv containers
 *  that are only available during playback
 */
void ClearTemporaryFonts();
} // namespace FONT
} // namespace UTILS
