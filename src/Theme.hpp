#ifndef FREESHOP_THEME_HPP
#define FREESHOP_THEME_HPP

#include <string>
#include <rapidjson/document.h>
#include <cpp3ds/Graphics/Color.hpp>

namespace FreeShop {

class Theme {
public:
	//Images vars
	static bool isFlagsThemed;
	static bool isItemBG9Themed;
	static bool isButtonRadius9Themed;
	static bool isFSBGSD9Themed;
	static bool isFSBGNAND9Themed;
	static bool isInstalledItemBG9Themed;
	static bool isItemBGSelected9Themed;
	static bool isListItemBG9Themed;
	static bool isMissingIconThemed;
	static bool isNotification9Themed;
	static bool isQrSelector9Themed;
	static bool isScrollbar9Themed;

	//Sounds vars
	static bool isSoundBlipThemed;
	static bool isSoundChimeThemed;
	static bool isSoundStartupThemed;

	//Text theming
	static Theme& getInstance();
	void loadDefaults();
	static bool loadFromFile(const std::string& filename = FREESHOP_DIR "/theme/texts.json");
	static void saveToFile(const std::string& filename = FREESHOP_DIR "/theme/texts.json");
	static const rapidjson::Value &get(std::string key);
	static void loadNameDesc();

	static bool isTextThemed;
	static cpp3ds::Color primaryTextColor;
	static cpp3ds::Color secondaryTextColor;
	static cpp3ds::Color iconSetColor;
	static cpp3ds::Color iconSetColorActive;
	static cpp3ds::Color transitionScreenColor;
	static cpp3ds::Color loadingIcon;
	static cpp3ds::Color loadingText;
	static cpp3ds::Color freText;
	static cpp3ds::Color versionText;
	static cpp3ds::Color percentageText;
	static cpp3ds::Color boxColor;
	static cpp3ds::Color boxOutlineColor;
	static cpp3ds::Color dialogBackground;
	static cpp3ds::Color dialogButton;
	static cpp3ds::Color dialogButtonText;
	static cpp3ds::Color themeDescColor;
	//DarkTheme
	static cpp3ds::Color darkPrimaryTextColor;
	static cpp3ds::Color darkSecondaryTextColor;
	static cpp3ds::Color darkIconSetColor;
	static cpp3ds::Color darkIconSetColorActive;
	static cpp3ds::Color darkTransitionScreenColor;
	static cpp3ds::Color darkLoadingIcon;
	static cpp3ds::Color darkLoadingText;
	static cpp3ds::Color darkFreText;
	static cpp3ds::Color darkVersionText;
	static cpp3ds::Color darkPercentageText;
	static cpp3ds::Color darkBoxColor;
	static cpp3ds::Color darkBoxOutlineColor;
	static cpp3ds::Color darkDialogBackground;
	static cpp3ds::Color darkDialogButton;
	static cpp3ds::Color darkDialogButtonText;
	static cpp3ds::Color darkThemeDescColor;

	//Theme informations
	static std::string themeName;
	static std::string themeDesc;
	static std::string themeVersion;
	//DarkTheme informations
	static std::string darkThemeName;
	static std::string darkThemeDesc;
	static std::string darkThemeVersion;

private:
	rapidjson::Document m_json;
};

} // namespace FreeShop

#endif // FREESHOP_THEME_HPP
