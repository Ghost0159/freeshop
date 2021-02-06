#include <cpp3ds/System/FileSystem.hpp>
#include <cpp3ds/System/I18n.hpp>
#include <fstream>
#include <rapidjson/ostreamwrapper.h>
#include <rapidjson/writer.h>
#include <cpp3ds/System/FileInputStream.hpp>
#include "Theme.hpp"
#include "Util.hpp"

namespace FreeShop {
//Images vars
bool Theme::isFlagsThemed = false;
bool Theme::isItemBG9Themed = false;
bool Theme::isButtonRadius9Themed = false;
bool Theme::isFSBGSD9Themed = false;
bool Theme::isFSBGNAND9Themed = false;
bool Theme::isInstalledItemBG9Themed = false;
bool Theme::isItemBGSelected9Themed = false;
bool Theme::isListItemBG9Themed = false;
bool Theme::isMissingIconThemed = false;
bool Theme::isNotification9Themed = false;
bool Theme::isQrSelector9Themed = false;
bool Theme::isScrollbar9Themed = false;

//Sounds vars
bool Theme::isSoundBlipThemed = false;
bool Theme::isSoundChimeThemed = false;
bool Theme::isSoundStartupThemed = false;

//Text theming
bool Theme::isTextThemed = false;
cpp3ds::Color Theme::primaryTextColor = cpp3ds::Color::Black;
cpp3ds::Color Theme::secondaryTextColor = cpp3ds::Color(130, 130, 130, 255);
cpp3ds::Color Theme::iconSetColor = cpp3ds::Color(100, 100, 100);
cpp3ds::Color Theme::iconSetColorActive = cpp3ds::Color(0, 0, 0);
cpp3ds::Color Theme::transitionScreenColor = cpp3ds::Color(255, 255, 255);
cpp3ds::Color Theme::loadingIcon = cpp3ds::Color(110, 110, 110, 255);
cpp3ds::Color Theme::loadingText = cpp3ds::Color::Black;
cpp3ds::Color Theme::freText = cpp3ds::Color(255, 255, 255, 0);
cpp3ds::Color Theme::versionText = cpp3ds::Color(0, 0, 0, 100);
cpp3ds::Color Theme::percentageText = cpp3ds::Color::Black;
cpp3ds::Color Theme::boxColor = cpp3ds::Color(245, 245, 245);
cpp3ds::Color Theme::boxOutlineColor = cpp3ds::Color(158, 158, 158, 255);
cpp3ds::Color Theme::dialogBackground = cpp3ds::Color(255, 255, 255, 128);
cpp3ds::Color Theme::dialogButton = cpp3ds::Color(158, 158, 158, 0);
cpp3ds::Color Theme::dialogButtonText = cpp3ds::Color(3, 169, 244, 0);
cpp3ds::Color Theme::themeDescColor = cpp3ds::Color(0, 0, 0, 0);
//DarkTheme Text theming
cpp3ds::Color Theme::darkPrimaryTextColor = cpp3ds::Color::Black;
cpp3ds::Color Theme::darkSecondaryTextColor = cpp3ds::Color::Black;
cpp3ds::Color Theme::darkIconSetColor = cpp3ds::Color::Black;
cpp3ds::Color Theme::darkIconSetColorActive = cpp3ds::Color::Black;
cpp3ds::Color Theme::darkTransitionScreenColor = cpp3ds::Color::Black;
cpp3ds::Color Theme::darkLoadingIcon = cpp3ds::Color::Black;
cpp3ds::Color Theme::darkLoadingText = cpp3ds::Color::Black;
cpp3ds::Color Theme::darkFreText = cpp3ds::Color::Black;
cpp3ds::Color Theme::darkVersionText = cpp3ds::Color::Black;
cpp3ds::Color Theme::darkPercentageText = cpp3ds::Color::Black;
cpp3ds::Color Theme::darkBoxColor = cpp3ds::Color::Black;
cpp3ds::Color Theme::darkBoxOutlineColor = cpp3ds::Color::Black;
cpp3ds::Color Theme::darkDialogBackground = cpp3ds::Color::Black;
cpp3ds::Color Theme::darkDialogButton = cpp3ds::Color::Black;
cpp3ds::Color Theme::darkDialogButtonText = cpp3ds::Color::Black;
cpp3ds::Color Theme::darkThemeDescColor = cpp3ds::Color::Black;

//Theme informations
std::string Theme::themeName = _("Classic").toAnsiString();
std::string Theme::themeDesc = _("The default theme of freeShop.\nMade by arc13 / Cruel.").toAnsiString();
std::string Theme::themeVersion = FREESHOP_VERSION;
//DarkTheme informations
std::string Theme::darkThemeName = _("Glowing Dark").toAnsiString();
std::string Theme::darkThemeDesc = _("The first official theme of freeShop.\nMade by Paul_GameDev").toAnsiString();
std::string Theme::darkThemeVersion = FREESHOP_VERSION;

Theme &Theme::getInstance()
{
	static Theme theme;
	return theme;
}

#define ADD_DEFAULT(key, val) \
	if (!m_json.HasMember(key)) \
		m_json.AddMember(rapidjson::StringRef(key), val, m_json.GetAllocator());

void Theme::loadDefaults()
{
	if (!m_json.IsObject())
		m_json.SetObject();

	ADD_DEFAULT("primaryText", "000000");
	ADD_DEFAULT("secondaryText", "828282");
	ADD_DEFAULT("iconSet", "646464");
	ADD_DEFAULT("iconSetActive", "363636");
	ADD_DEFAULT("transitionScreen", "FFFFFF");
	ADD_DEFAULT("loadingColor", "898989");
	ADD_DEFAULT("loadingText", "353535");
	ADD_DEFAULT("freText", "420420");
	ADD_DEFAULT("versionText", "546978");
	ADD_DEFAULT("percentageText", "115599");
	ADD_DEFAULT("boxColor", "00FF7F");
	ADD_DEFAULT("boxOutlineColor", "008899");
	ADD_DEFAULT("dialogBackground", "CE2F06");
	ADD_DEFAULT("dialogButton", "FDE243");
	ADD_DEFAULT("dialogButtonText", "320F3C");
	ADD_DEFAULT("darkPrimaryText", "FFFFFF");
	ADD_DEFAULT("darkSecondaryText", "FFFFFF");
	ADD_DEFAULT("darkIconSet", "FFFFFF");
	ADD_DEFAULT("darkIconSetActive", "FFFFFF");
	ADD_DEFAULT("darkTransitionScreen", "FFFFFF");
	ADD_DEFAULT("darkLoadingColor", "FFFFFF");
	ADD_DEFAULT("darkLoadingText", "FFFFFF");
	ADD_DEFAULT("darkFreText", "FFFFFF");
	ADD_DEFAULT("darkVersionText", "FFFFFF");
	ADD_DEFAULT("darkPercentageText", "FFFFFF");
	ADD_DEFAULT("darkBoxColor", "FFFFFF");
	ADD_DEFAULT("darkBoxOutlineColor", "FFFFFF");
	ADD_DEFAULT("darkDialogBackground", "FFFFFF");
	ADD_DEFAULT("darkDialogButton", "FFFFFF");
	ADD_DEFAULT("darkDialogButtonText", "FFFFFF");

	ADD_DEFAULT("themeName", "My custom theme");
	ADD_DEFAULT("themeDesc", "Theme made by someone.");
	ADD_DEFAULT("themeVer", "x.y");
	ADD_DEFAULT("darkThemeName", "Ohaii~!");
	ADD_DEFAULT("darkThemeDesc", "You can't even see this, lol.");
	ADD_DEFAULT("darkThemeVer", "Code.Lurker");

	getInstance().saveToFile();
}

bool Theme::loadFromFile(const std::string &filename)
{
	rapidjson::Document &json = getInstance().m_json;
	std::string path = cpp3ds::FileSystem::getFilePath(filename);
	std::string jsonString;
	cpp3ds::FileInputStream file;
	if (!file.open(filename))
		return false;
	jsonString.resize(file.getSize());
	file.read(&jsonString[0], jsonString.size());
	json.Parse(jsonString.c_str());
	getInstance().loadDefaults();
	return !json.HasParseError();
}

const rapidjson::Value &Theme::get(std::string key)
{
	return getInstance().m_json[key.c_str()];
}

void Theme::saveToFile(const std::string &filename)
{
	std::string path = cpp3ds::FileSystem::getFilePath(filename);
	std::ofstream file(path);
	rapidjson::OStreamWrapper osw(file);
	rapidjson::Writer<rapidjson::OStreamWrapper> writer(osw);
	getInstance().m_json.Accept(writer);
}

void Theme::loadNameDesc()
{
	themeName = _("Classic").toAnsiString();
	themeDesc = _("The default theme of freeShop.\nMade by arc13 / Cruel.").toAnsiString();
}

} // namespace FreeShop
