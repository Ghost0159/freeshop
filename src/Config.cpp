#include <cpp3ds/System/FileSystem.hpp>
#include <fstream>
#include <rapidjson/ostreamwrapper.h>
#include <rapidjson/writer.h>
#include <cpp3ds/System/FileInputStream.hpp>
#include "Config.hpp"
#include "Util.hpp"

namespace {
	// Matches with Config::Key enum in Config.hpp
	const char *keyStrings[] = {
		"version",
		"cache_version",
		"trigger_update",
		"show_news",
		// Filter
		"filter_region",
		"filter_genre",
		"filter_language",
		"filter_platform",
		"filter_publisher",
		"filter_features",
		// Sort
		"sortGL",
		"sortGLDir",
		"sortIL",
		"sortILDir",
		// Update
		"auto-update",
		"last_updated",
		"download_title_keys",
		"key_urls",
		"download_multiple_url",
		// Download
		"download_timeout",
		"download_buffer_size",
		"play_sound_after_download",
		"power_off_after_download",
		"power_off_time",
		// Music
		"music_mode",
		"music_filename",
		"music_off_low_slider",
		// Locales
		"language",
		"keyboard",
		"use_system_keyboard",
		// Notifiers
		"led_startup",
		"led_on_download_finish",
		"led_on_download_error",
		"news_download_finish",
		"news_no_led",
		// Inactivity
		"sleep_mode",
		"sleep_mode_bottom",
		"dim_leds",
		"sound_on_inactivity",
		"music_on_inactivity",
		"inactivity_seconds",
		// Other
		"title_id",
		"show_battery_percentage",
		"show_game_counter",
		"show_game_description",
		"darktheme",
		"restartfix",
		// Internals
		"reset_eshop_music",
		"clean_exit",
		"ap_fuse",
	};
}

namespace FreeShop {

Config::Config()
{
	static_assert(KEY_COUNT == sizeof(keyStrings)/sizeof(*keyStrings), "Key string count must match the enum count.");
	loadDefaults();
}

Config &Config::getInstance()
{
	static Config config;
	return config;
}

bool Config::loadFromFile(const std::string &filename)
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

void Config::saveToFile(const std::string &filename)
{
	std::string path = cpp3ds::FileSystem::getFilePath(filename);
	std::ofstream file(path);
	rapidjson::OStreamWrapper osw(file);
	rapidjson::Writer<rapidjson::OStreamWrapper> writer(osw);
	getInstance().m_json.Accept(writer);
}

bool Config::keyExists(const char *key)
{
	return getInstance().m_json.HasMember(key);
}

const rapidjson::Value &Config::get(Key key)
{
	return getInstance().m_json[keyStrings[key]];
}

#define ADD_DEFAULT(key, val) \
	if (!m_json.HasMember(keyStrings[key])) \
		m_json.AddMember(rapidjson::StringRef(keyStrings[key]), val, m_json.GetAllocator());

void Config::loadDefaults()
{
	if (!m_json.IsObject())
		m_json.SetObject();

	ADD_DEFAULT(Version, "");
	ADD_DEFAULT(CacheVersion, "");
	ADD_DEFAULT(TriggerUpdateFlag, false);
	ADD_DEFAULT(ShowNews, true);

	// Filter
	ADD_DEFAULT(FilterRegion, rapidjson::kArrayType);
	ADD_DEFAULT(FilterGenre, rapidjson::kArrayType);
	ADD_DEFAULT(FilterLanguage, rapidjson::kArrayType);
	ADD_DEFAULT(FilterPlatform, rapidjson::kArrayType);
	ADD_DEFAULT(FilterPublisher, rapidjson::kArrayType);
	ADD_DEFAULT(FilterFeatures, rapidjson::kArrayType);

	// Sort
	ADD_DEFAULT(SortGameList, 0);
	ADD_DEFAULT(SortGameListDirection, 1);
	ADD_DEFAULT(SortInstalledList, 1);
	ADD_DEFAULT(SortInstalledListDirection, 0);

	// Update
	ADD_DEFAULT(AutoUpdate, true);
	ADD_DEFAULT(LastUpdatedTime, 0);
	ADD_DEFAULT(DownloadTitleKeys, false);
	ADD_DEFAULT(AP_FUSE, false);
	ADD_DEFAULT(KeyURLs, rapidjson::kArrayType);
	ADD_DEFAULT(DownloadFromMultipleURLs, true);

	// Download
	ADD_DEFAULT(DownloadTimeout, 6.f);
	ADD_DEFAULT(DownloadBufferSize, 128u);
	ADD_DEFAULT(PlaySoundAfterDownload, true);
	ADD_DEFAULT(PowerOffAfterDownload, false);
	ADD_DEFAULT(PowerOffTime, 120);

	// Music
	ADD_DEFAULT(MusicMode, "eshop");
	ADD_DEFAULT(MusicFilename, "");
	ADD_DEFAULT(MusicTurnOffSlider, true);

	// Locales
	ADD_DEFAULT(Language, "auto");
	ADD_DEFAULT(Keyboard, "qwerty");
	ADD_DEFAULT(SystemKeyboard, false);

	// Notifiers
	ADD_DEFAULT(LEDStartup, true);
	ADD_DEFAULT(LEDDownloadFinished, true);
	ADD_DEFAULT(LEDDownloadError, true);
	ADD_DEFAULT(NEWSDownloadFinished, false);
	ADD_DEFAULT(NEWSNoLED, false);

	// Inactivity
	ADD_DEFAULT(SleepMode, true);
	ADD_DEFAULT(SleepModeBottom, false);
	ADD_DEFAULT(DimLEDs, false);
	ADD_DEFAULT(SoundOnInactivity, true);
	ADD_DEFAULT(MusicOnInactivity, false);
	ADD_DEFAULT(InactivitySeconds, 60.f);

	// Other
	ADD_DEFAULT(TitleID, false);
	ADD_DEFAULT(ShowBattery, false);
	ADD_DEFAULT(ShowGameCounter, true);
	ADD_DEFAULT(ShowGameDescription, true);
	ADD_DEFAULT(DarkTheme, false);
	ADD_DEFAULT(RestartFix, false);

	// Internals
	ADD_DEFAULT(ResetEshopMusic, false);
	ADD_DEFAULT(CleanExit, true);
}

void Config::set(Key key, const char *val)
{
	rapidjson::Value v(val, getInstance().m_json.GetAllocator());
	set(key, v);
}

void Config::set(Key key, rapidjson::Value &val)
{
	const char *keyStr = keyStrings[key];
	if (keyExists(keyStr))
		getInstance().m_json[keyStr] = val;
	else
		getInstance().m_json.AddMember(rapidjson::StringRef(keyStr), val, getInstance().m_json.GetAllocator());
}

rapidjson::Document::AllocatorType &Config::getAllocator()
{
	return getInstance().m_json.GetAllocator();
}


} // namespace FreeShop
