#pragma once

#include "types.h"

/**
 * @brief Prints to screen the assets found. This functions show the type of
 * file, the user permissions, group, owner, date, etc...
 *
 * @param content       pointer to the directory containing the assets
 * @param directoryName name of the listed directory
 * @param arguments     pointer to the parsed arguments structure
 */
void PrintAssetLongFormat(const directory_t *content, const char *directoryName, const arguments_t *arguments);

/**
 * @brief Prints to screen the assets found. This functions show only basic
 * information as the icon and the name.
 *
 * @param content pointer to the directory containing the assets
 * @param arguments     pointer to the parsed arguments structure
 */
void PrintAssetShortFormat(const directory_t *content, const arguments_t *arguments);

/**
 * @brief Display the information of the available extensions. For each entry,
 * a line will be printed with the RGB color values, the icon and the extension
 * name. If if the possible the line will have the predefined color of the
 * extension.
 *
 * @param arguments pointer to the parsed arguments structure
 */
void ShowMetaData(const arguments_t *arguments);
