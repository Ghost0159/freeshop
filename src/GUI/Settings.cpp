#include <cpp3ds/System/I18n.hpp>
#include <Gwen/Skins/TexturedBase.h>
#include <cpp3ds/System/FileInputStream.hpp>
#include <unistd.h>
#include <cpp3ds/System/FileSystem.hpp>
#include <cmath>
#include <dirent.h>
#include <stdlib.h>
#include "Settings.hpp"
#include "../Download.hpp"
#include "../Util.hpp"
#include "../AppList.hpp"
#include "../InstalledList.hpp"
#include "../TitleKeys.hpp"
#include "../Notification.hpp"
#include "../States/DialogState.hpp"
#include "../States/BrowseState.hpp"
#include "../Theme.hpp"

#ifdef _3DS
#include "../KeyboardApplet.hpp"
#include <3ds.h>
#endif

using namespace Gwen::Controls;

namespace FreeShop {
extern cpp3ds::Uint64 g_requestJump;
namespace GUI {


Settings::Settings(Gwen::Skin::TexturedBase *skin,  State *state)
: m_ignoreCheckboxChange(false)
, m_state(state)
{
	// Seed rand in case this is initialized in loading thread
	srand(time(NULL));

	m_canvas = new Canvas(skin);
	m_canvas->SetPos(0, 0);

	m_input.Initialize(m_canvas);

	m_tabControl = new TabControl(m_canvas);
	m_tabControl->SetBounds(0, 40, 320, 200);

	// Get country code for language-specific fetching
	m_countryCode = getCountryCode(0);

	// Filters
	Base *page = m_tabControl->AddPage(_("Filter").toAnsiString())->GetPage();
	m_filterTabControl = new TabControl(page);
	m_filterTabControl->Dock(Gwen::Pos::Fill);
	m_filterTabControl->SetTabStripPosition(Gwen::Pos::Left);
	m_buttonFilterSave = new Button(page);
	m_buttonFilterSave->SetFont(L"fonts/fontawesome.ttf", 18, false);
	m_buttonFilterSave->SetText("\uf0c7"); // Save floppy icon
	m_buttonFilterSave->SetBounds(0, 140, 22, 22);
	m_buttonFilterSave->SetPadding(Gwen::Padding(0, 0, 0, 3));
	m_buttonFilterSave->onPress.Add(this, &Settings::filterSaveClicked);
	m_buttonFilterSaveClear = new Button(page);
	m_buttonFilterSaveClear->SetFont(L"fonts/fontawesome.ttf", 18, false);
	m_buttonFilterSaveClear->SetText("\uf014"); // Trash can icon
	m_buttonFilterSaveClear->SetBounds(26, 140, 22, 22);
	m_buttonFilterSaveClear->SetPadding(Gwen::Padding(0, 0, 0, 3));
	m_buttonFilterSaveClear->onPress.Add(this, &Settings::filterClearClicked);

	ScrollControl *scrollBox;
	scrollBox = addFilterPage("Regions");
	fillFilterRegions(scrollBox);
	scrollBox = addFilterPage("Genre");
	fillFilterGenres(scrollBox);
	scrollBox = addFilterPage("Language");
	fillFilterLanguages(scrollBox);
	scrollBox = addFilterPage("Feature");
	fillFilterFeature(scrollBox);
	scrollBox = addFilterPage("Platform");
	fillFilterPlatforms(scrollBox);
	scrollBox = addFilterPage("Publisher");
	fillFilterPublisher(scrollBox);

	// Sorting
	page = m_tabControl->AddPage(_("Sort").toAnsiString())->GetPage();
	fillSortPage(page);

	page = m_tabControl->AddPage(_("Update").toAnsiString())->GetPage();
	fillUpdatePage(page);

	page = m_tabControl->AddPage(_("Download").toAnsiString())->GetPage();
	fillDownloadPage(page);

	page = m_tabControl->AddPage(_("Music").toAnsiString())->GetPage();
	fillMusicPage(page);

	page = m_tabControl->AddPage(_("Locales").toAnsiString())->GetPage();
	fillLocalesPage(page);

	page = m_tabControl->AddPage(_("Theme Info").toAnsiString())->GetPage();
	fillThemeInfoPage(page);

	page = m_tabControl->AddPage(_("Beta Options").toAnsiString())->GetPage();
	fillThemeSettingsPage(page);

	page = m_tabControl->AddPage(_("Notifiers").toAnsiString())->GetPage();
	fillNotifiersPage(page);

	page = m_tabControl->AddPage(_("Inactivity").toAnsiString())->GetPage();
	fillInactivityPage(page);

	page = m_tabControl->AddPage(_("Other").toAnsiString())->GetPage();
	fillOtherPage(page);


	loadConfig();
	updateEnabledStates();
	updateSorting(nullptr);
}

Settings::~Settings()
{
	saveToConfig();
	delete m_canvas;
}

bool Settings::processEvent(const cpp3ds::Event &event)
{
	return m_input.ProcessMessage(event);
}

bool Settings::update(float delta)
{
	return false;
}

void Settings::render()
{
	m_canvas->RenderCanvas();
}

int Settings::getValues(int tweenType, float *returnValues)
{
	switch (tweenType) {
		case POSITION_XY:
			returnValues[0] = getPosition().x;
			returnValues[1] = getPosition().y;
			return 2;
		default: return -1;
	}
}

void Settings::setValues(int tweenType, float *newValues)
{
	switch (tweenType) {
		case POSITION_XY: setPosition(cpp3ds::Vector2f(newValues[0], newValues[1])); break;
		default: break;
	}
}

void Settings::saveToConfig()
{
	Config::set(Config::AutoUpdate, m_checkboxAutoUpdate->Checkbox()->IsChecked());
	Config::set(Config::DownloadTitleKeys, m_checkboxDownloadKeys->Checkbox()->IsChecked());
	Config::set(Config::DownloadFromMultipleURLs, m_checkboxDownloadMultipleKeys->Checkbox()->IsChecked());

	rapidjson::Value list(rapidjson::kArrayType);
	if (m_comboBoxUrls->GetSelectedItem())
	{
		auto menuList = m_comboBoxUrls->GetSelectedItem()->GetParent()->GetChildren();
		for (auto it = menuList.begin(); it != menuList.end(); ++it)
		{
			rapidjson::Value val(rapidjson::StringRef((*it)->GetValue().c_str()), Config::getAllocator());
			list.PushBack(val, Config::getAllocator());
		}
	}
	Config::set(Config::KeyURLs, list);

	// Download
	Config::set(Config::DownloadTimeout, m_sliderTimeout->GetFloatValue());
	Config::set(Config::DownloadBufferSize, static_cast<size_t>(std::ceil(m_sliderDownloadBufferSize->GetFloatValue())));
	Config::set(Config::PlaySoundAfterDownload, m_checkboxPlaySound->Checkbox()->IsChecked());
	Config::set(Config::PowerOffAfterDownload, m_checkboxPowerOff->Checkbox()->IsChecked());
	Config::set(Config::PowerOffTime, static_cast<int>(strtol(m_comboPowerOffTime->GetSelectedItem()->GetName().c_str(), nullptr, 10)));

	// Music
	Config::set(Config::MusicMode, m_comboMusicMode->GetSelectedItem()->GetName().c_str());
	auto selectedMusic = m_listboxMusicFiles->GetSelectedRow();
	Config::set(Config::MusicFilename, selectedMusic ? selectedMusic->GetText(0).c_str() : "");
	Config::set(Config::MusicTurnOffSlider, m_checkboxTurnOffMusicSlider->Checkbox()->IsChecked());

	// Locales
	auto language = m_listboxLanguages->GetSelectedRow();
	Config::set(Config::Language, language ? language->GetName().c_str() : "auto");
	Config::set(Config::Keyboard, m_listboxKeyboards->GetSelectedRow()->GetName().c_str());
	Config::set(Config::SystemKeyboard, m_checkboxSystemKeyboard->Checkbox()->IsChecked());

	// Notifiers
	Config::set(Config::LEDStartup, m_checkboxLEDStartup->Checkbox()->IsChecked());
	Config::set(Config::LEDDownloadFinished, m_checkboxLEDDownloadFinished->Checkbox()->IsChecked());
	Config::set(Config::LEDDownloadError, m_checkboxLEDDownloadErrored->Checkbox()->IsChecked());
	Config::set(Config::NEWSDownloadFinished, m_checkboxNEWSDownloadFinished->Checkbox()->IsChecked());
	Config::set(Config::NEWSNoLED, m_checkboxNEWSNoLED->Checkbox()->IsChecked());
	Config::set(Config::InactivitySeconds, m_sliderInactivityTime->GetFloatValue());

	// Inactivity
	Config::set(Config::SleepMode, m_checkboxSleep->Checkbox()->IsChecked());
	Config::set(Config::SleepModeBottom, m_checkboxSleepBottom->Checkbox()->IsChecked());
#ifndef EMULATION
	if (!envIsHomebrew())
#endif
	Config::set(Config::DimLEDs, m_checkboxDimLEDs->Checkbox()->IsChecked());
	Config::set(Config::SoundOnInactivity, m_checkboxInactivitySoundAllowed->Checkbox()->IsChecked());
	Config::set(Config::MusicOnInactivity, m_checkboxInactivityMusicAllowed->Checkbox()->IsChecked());

	// Other
	Config::set(Config::TitleID, m_checkboxTitleID->Checkbox()->IsChecked());
#ifndef EMULATION
	if (!envIsHomebrew())
#endif
	Config::set(Config::ShowBattery, m_checkboxBatteryPercent->Checkbox()->IsChecked());
	Config::set(Config::ShowGameCounter, m_checkboxGameCounter->Checkbox()->IsChecked());
	Config::set(Config::ShowGameDescription, m_checkboxGameDescription->Checkbox()->IsChecked());
	Config::set(Config::DarkTheme, m_checkboxDarkTheme->Checkbox()->IsChecked());
	Config::set(Config::RestartFix, m_checkboxRestartFix->Checkbox()->IsChecked());
}

void Settings::loadConfig()
{
	// Filters
	m_buttonFilterSaveClear->SetDisabled(true);
	loadFilter(Config::FilterRegion, m_filterRegionCheckboxes);
	loadFilter(Config::FilterGenre, m_filterGenreCheckboxes);
	loadFilter(Config::FilterLanguage, m_filterLanguageCheckboxes);
	loadFilter(Config::FilterPlatform, m_filterPlatformCheckboxes);
	loadFilter(Config::FilterPublisher, m_filterPublisherCheckboxes);
	loadFilter(Config::FilterFeatures, m_filterFeatureCheckboxes);

	// Update
	m_checkboxAutoUpdate->Checkbox()->SetChecked(Config::get(Config::AutoUpdate).GetBool());
	m_checkboxDownloadKeys->Checkbox()->SetChecked(Config::get(Config::DownloadTitleKeys).GetBool());
	m_checkboxDownloadMultipleKeys->Checkbox()->SetChecked(Config::get(Config::DownloadFromMultipleURLs).GetBool());

	char strTime[100];
	time_t lastUpdatedTime = Config::get(Config::LastUpdatedTime).GetInt();
	strftime(strTime, sizeof(strTime), "%D %T", localtime(&lastUpdatedTime));
	if (lastUpdatedTime == 0)
		m_labelLastUpdated->SetText(_("(Never checked)").toAnsiString());
	else
		m_labelLastUpdated->SetText(_("(Last checked: %s)", strTime).toAnsiString());

	auto urls = Config::get(Config::KeyURLs).GetArray();
	m_comboBoxUrls->ClearItems();
	for (int i = 0; i < urls.Size(); ++i)
	{
		const char *url = urls[i].GetString();
		std::wstring ws(url, url + urls[i].GetStringLength());
		m_comboBoxUrls->AddItem(ws);
	}

	// Download
	m_checkboxPlaySound->Checkbox()->SetChecked(Config::get(Config::PlaySoundAfterDownload).GetBool());
	m_checkboxPowerOff->Checkbox()->SetChecked(Config::get(Config::PowerOffAfterDownload).GetBool());
	m_comboPowerOffTime->SelectItemByName(_("%d", Config::get(Config::PowerOffTime).GetInt()));
	m_sliderTimeout->SetFloatValue(Config::get(Config::DownloadTimeout).GetFloat());
	m_sliderDownloadBufferSize->SetFloatValue(Config::get(Config::DownloadBufferSize).GetUint());
	downloadTimeoutChanged(m_sliderTimeout);
	downloadBufferSizeChanged(m_sliderDownloadBufferSize);

	// Music
	auto musicFileRows = m_listboxMusicFiles->GetChildren().front()->GetChildren();
	for (auto& row : musicFileRows)
		if (gwen_cast<Layout::TableRow>(row)->GetText(0) == Config::get(Config::MusicFilename).GetString())
		{
			m_listboxMusicFiles->SetSelectedRow(row, true);
			break;
		}
	m_comboMusicMode->SelectItemByName(Config::get(Config::MusicMode).GetString());
	m_checkboxTurnOffMusicSlider->Checkbox()->SetChecked(Config::get(Config::MusicTurnOffSlider).GetBool());

	// Locales
	m_checkboxSystemKeyboard->Checkbox()->SetChecked(Config::get(Config::SystemKeyboard).GetBool());
	auto languageRows = m_listboxLanguages->GetChildren().front()->GetChildren();
	for (auto& row : languageRows)
		if (row->GetName() == Config::get(Config::Language).GetString())
		{
			m_listboxLanguages->SetSelectedRow(row, true);
			break;
		}
	auto keyboardRows = m_listboxKeyboards->GetChildren().front()->GetChildren();
	for (auto& row : keyboardRows)
		if (row->GetName() == Config::get(Config::Keyboard).GetString())
		{
			m_listboxKeyboards->SetSelectedRow(row, true);
			break;
		}

	// Notifiers
	m_checkboxLEDStartup->Checkbox()->SetChecked(Config::get(Config::LEDStartup).GetBool());
	m_checkboxLEDDownloadFinished->Checkbox()->SetChecked(Config::get(Config::LEDDownloadFinished).GetBool());
	m_checkboxLEDDownloadErrored->Checkbox()->SetChecked(Config::get(Config::LEDDownloadError).GetBool());
	m_checkboxNEWSDownloadFinished->Checkbox()->SetChecked(Config::get(Config::NEWSDownloadFinished).GetBool());
	m_checkboxNEWSNoLED->Checkbox()->SetChecked(Config::get(Config::NEWSNoLED).GetBool());

	// Inactivity
	m_checkboxSleep->Checkbox()->SetChecked(Config::get(Config::SleepMode).GetBool());
	m_checkboxSleepBottom->Checkbox()->SetChecked(Config::get(Config::SleepModeBottom).GetBool());
#ifndef EMULATION
	if (!envIsHomebrew())
#endif
	m_checkboxDimLEDs->Checkbox()->SetChecked(Config::get(Config::DimLEDs).GetBool());
	m_checkboxInactivitySoundAllowed->Checkbox()->SetChecked(Config::get(Config::SoundOnInactivity).GetBool());
	m_checkboxInactivityMusicAllowed->Checkbox()->SetChecked(Config::get(Config::MusicOnInactivity).GetBool());
	m_sliderInactivityTime->SetFloatValue(Config::get(Config::InactivitySeconds).GetFloat());
	inactivityTimeChanged(m_sliderInactivityTime);

	// Other
	m_checkboxTitleID->Checkbox()->SetChecked(Config::get(Config::TitleID).GetBool());
#ifndef EMULATION
	if (!envIsHomebrew())
#endif
	m_checkboxBatteryPercent->Checkbox()->SetChecked(Config::get(Config::ShowBattery).GetBool());
	m_checkboxGameCounter->Checkbox()->SetChecked(Config::get(Config::ShowGameCounter).GetBool());
	m_checkboxGameDescription->Checkbox()->SetChecked(Config::get(Config::ShowGameDescription).GetBool());
	m_checkboxDarkTheme->Checkbox()->SetChecked(Config::get(Config::DarkTheme).GetBool());
	m_checkboxRestartFix->Checkbox()->SetChecked(Config::get(Config::RestartFix).GetBool());
}

void Settings::saveFilter(Config::Key key, std::vector<Gwen::Controls::CheckBoxWithLabel*> &checkboxArray)
{
	rapidjson::Value val;
	val.SetArray();
	for (auto& checkbox : checkboxArray)
		if (checkbox->Checkbox()->IsChecked())
		{
			int i = atoi(checkbox->Checkbox()->GetValue().c_str());
			val.PushBack(i, Config::getAllocator());
		}
	Config::set(key, val);
}

void Settings::loadFilter(Config::Key key, std::vector<Gwen::Controls::CheckBoxWithLabel*> &checkboxArray)
{
	// Ignore the checkbox change events.
	// Manually invoke the event once at the end.
	if (checkboxArray.empty())
		return;

	m_ignoreCheckboxChange = true;
	auto filterArray = Config::get(key).GetArray();
	if (!filterArray.Empty())
	{
		// Enable clear button since a filter is evidently saved
		m_buttonFilterSaveClear->SetDisabled(false);
		for (auto& checkbox : checkboxArray)
		{
			int val = strtol(checkbox->Checkbox()->GetValue().c_str(), nullptr, 10);
			for (auto& filterId : filterArray)
				if (filterId.GetInt() == val)
					checkbox->Checkbox()->SetChecked(true);
		}
	}
	m_ignoreCheckboxChange = false;
	auto checkbox = checkboxArray.front()->Checkbox();
	if (!filterArray.Empty())
		checkbox->onCheckChanged.Call(checkbox);
}

void Settings::setPosition(const cpp3ds::Vector2f &position)
{
	m_position = position;
	m_canvas->SetPos(position.x, position.y);
}

const cpp3ds::Vector2f &Settings::getPosition() const
{
	return m_position;
}

void Settings::fillFilterGenres(Gwen::Controls::Base *parent)
{
	// Use English instead of Chinese when fetching genre data (Chinese lacks most genres)
	std::string countryCode = m_countryCode;
	if (countryCode == "HK" || countryCode == "TW")
		countryCode = "GB";

	std::string jsonFilename = _(FREESHOP_DIR "/cache/genres.%s.json", m_countryCode.c_str());
	if (!pathExists(jsonFilename.c_str()))
	{
		Download download(_("https://samurai.ctr.shop.nintendo.net/samurai/ws/%s/genres/?shop_id=1", countryCode.c_str()), jsonFilename);
		download.setField("Accept", "application/json");
		download.run();
	}

	rapidjson::Document doc;
	std::string json;
	cpp3ds::FileInputStream file;
	if (file.open(jsonFilename))
	{
		json.resize(file.getSize());
		file.read(&json[0], json.size());
		doc.Parse(json.c_str());
		if (doc.HasParseError())
		{
			unlink(cpp3ds::FileSystem::getFilePath(jsonFilename).c_str());
			return;
		}

		CheckBoxWithLabel* checkbox;
		Label *labelCount;
		rapidjson::Value &list = doc["genres"]["genre"];
		std::vector<std::pair<int, std::string>> genres;
		for (int i = 0; i < list.Size(); ++i)
		{
			int genreId = list[i]["id"].GetInt();
			std::string genreName = list[i]["name"].GetString();
			genres.push_back(std::make_pair(genreId, genreName));
		}

		// Alphabetical sort
		std::sort(genres.begin(), genres.end(), [=](std::pair<int, std::string>& a, std::pair<int, std::string>& b) {
			return a.second < b.second;
		});

		for (int i = 0; i < genres.size(); ++i)
		{
			int genreId = genres[i].first;
			std::string &genreName = genres[i].second;
			int gameCount = 0;
			for (auto &app : AppList::getInstance().getList())
				for (auto &id : app->getAppItem()->getGenres())
					if (id == genreId)
						gameCount++;

			if (gameCount > 0) {
				labelCount = new Label(parent);
				labelCount->SetText(_("%d", gameCount).toAnsiString());
				labelCount->SetBounds(0, 2 + m_filterGenreCheckboxes.size() * 18, 31, 18);
				labelCount->SetAlignment(Gwen::Pos::Right);

				checkbox = new CheckBoxWithLabel(parent);
				checkbox->SetPos(35, m_filterGenreCheckboxes.size() * 18);
				checkbox->Label()->SetText(genreName);
				checkbox->Checkbox()->SetValue(_("%d", genreId).toAnsiString());
				checkbox->Checkbox()->onCheckChanged.Add(this, &Settings::filterCheckboxChanged);

				m_filterGenreCheckboxes.push_back(checkbox);
			}
		}
	}
}

void Settings::fillFilterPlatforms(Gwen::Controls::Base *parent)
{
	// Use English instead of Chinese when fetching platform data (doesn't exist in Chinese)
	std::string countryCode = m_countryCode;
	if (countryCode == "HK" || countryCode == "TW")
		countryCode = "GB";

	std::string jsonFilename = _(FREESHOP_DIR "/cache/platforms.%s.json", m_countryCode.c_str());
	if (!pathExists(jsonFilename.c_str()))
	{
		Download download(_("https://samurai.ctr.shop.nintendo.net/samurai/ws/%s/platforms/?shop_id=1", countryCode.c_str()), jsonFilename);
		download.setField("Accept", "application/json");
		download.run();
	}

	rapidjson::Document doc;
	std::string json;
	cpp3ds::FileInputStream file;
	if (file.open(jsonFilename))
	{
		json.resize(file.getSize());
		file.read(&json[0], json.size());
		doc.Parse(json.c_str());
		if (doc.HasParseError())
		{
			unlink(cpp3ds::FileSystem::getFilePath(jsonFilename).c_str());
			return;
		}

		CheckBoxWithLabel* checkbox;
		Label *labelCount;
		rapidjson::Value &list = doc["platforms"]["platform"];
		std::vector<std::pair<int, std::string>> platforms;
		for (int i = 0; i < list.Size(); ++i)
		{
			int platformId = list[i]["id"].GetInt();
			std::string platformName = list[i]["name"].GetString();
			platforms.push_back(std::make_pair(platformId, platformName));
		}

		//Put GBA games, as they are not in the eShop platforms list
		platforms.push_back(std::make_pair(23, "Game Boy Advance"));

		// Alphabetical sort
		std::sort(platforms.begin(), platforms.end(), [=](std::pair<int, std::string>& a, std::pair<int, std::string>& b) {
			return a.second < b.second;
		});

		for (int i = 0; i < platforms.size(); ++i)
		{
			int platformId = platforms[i].first;
			std::string &platformName = platforms[i].second;
			int gameCount = 0;
			for (auto &app : AppList::getInstance().getList())
				if (platformId == app->getAppItem()->getPlatform())
					gameCount++;

			if (gameCount > 0) {
				labelCount = new Label(parent);
				labelCount->SetText(_("%d", gameCount).toAnsiString());
				labelCount->SetBounds(0, 2 + m_filterPlatformCheckboxes.size() * 18, 31, 18);
				labelCount->SetAlignment(Gwen::Pos::Right);

				checkbox = new CheckBoxWithLabel(parent);
				checkbox->SetPos(35, m_filterPlatformCheckboxes.size() * 18);
				checkbox->Label()->SetText(platformName);
				checkbox->Checkbox()->SetValue(_("%d", platformId).toAnsiString());
				checkbox->Checkbox()->onCheckChanged.Add(this, &Settings::filterCheckboxChanged);

				m_filterPlatformCheckboxes.push_back(checkbox);
			}
		}
	}
}

void Settings::fillFilterPublisher(Gwen::Controls::Base *parent)
{
	// Use English instead of Chinese when fetching platform data (doesn't exist in Chinese)
	std::string countryCode = m_countryCode;
	if (countryCode == "HK" || countryCode == "TW")
		countryCode = "GB";

	std::string jsonFilename = _(FREESHOP_DIR "/cache/publishers.%s.json", m_countryCode.c_str());
	if (!pathExists(jsonFilename.c_str()))
	{
		Download download(_("https://samurai.ctr.shop.nintendo.net/samurai/ws/%s/publishers/?shop_id=1", countryCode.c_str()), jsonFilename);
		download.setField("Accept", "application/json");
		download.run();
	}

	rapidjson::Document doc;
	std::string json;
	cpp3ds::FileInputStream file;
	if (file.open(jsonFilename))
	{
		json.resize(file.getSize());
		file.read(&json[0], json.size());
		doc.Parse(json.c_str());
		if (doc.HasParseError())
		{
			unlink(cpp3ds::FileSystem::getFilePath(jsonFilename).c_str());
			return;
		}

		CheckBoxWithLabel* checkbox;
		Label *labelCount;
		rapidjson::Value &list = doc["publishers"]["publisher"];
		std::vector<std::pair<int, std::string>> publishers;
		for (int i = 0; i < list.Size(); ++i)
		{
			int publisherId = list[i]["id"].GetInt();
			std::string publisherName = list[i]["name"].GetString();
			publishers.push_back(std::make_pair(publisherId, publisherName));
		}

		// No need to do alphabetical sort, the list is already sorted, and we want our Nintendo at top ;p

		for (int i = 0; i < publishers.size(); ++i)
		{
			int publisherId = publishers[i].first;
			std::string &publisherName = publishers[i].second;
			int gameCount = 0;
			for (auto &app : AppList::getInstance().getList())
				if (publisherId == app->getAppItem()->getPublisher())
					gameCount++;

			if (gameCount > 0) {
				labelCount = new Label(parent);
				labelCount->SetText(_("%d", gameCount).toAnsiString());
				labelCount->SetBounds(0, 2 + m_filterPublisherCheckboxes.size() * 18, 31, 18);
				labelCount->SetAlignment(Gwen::Pos::Right);

				checkbox = new CheckBoxWithLabel(parent);
				checkbox->SetPos(35, m_filterPublisherCheckboxes.size() * 18);
				checkbox->Label()->SetText(publisherName);
				checkbox->Checkbox()->SetValue(_("%d", publisherId).toAnsiString());
				checkbox->Checkbox()->onCheckChanged.Add(this, &Settings::filterCheckboxChanged);

				m_filterPublisherCheckboxes.push_back(checkbox);
			}
		}
	}
}

void Settings::fillFilterFeature(Gwen::Controls::Base *parent)
{
	// Use English instead of Chinese when fetching genre data (Chinese lacks most genres)
	std::string countryCode = m_countryCode;
	if (countryCode == "HK" || countryCode == "TW")
		countryCode = "GB";

	const char *url = "https://api.github.com/repos/Arc13/feature-cache-releases/releases/latest";
	const char *latestJsonFilename = FREESHOP_DIR "/tmp/latest.json";
	Download featureLatest(url, latestJsonFilename);
	featureLatest.run();

	cpp3ds::FileInputStream jsonFile;
	if (jsonFile.open(latestJsonFilename))
	{
		std::string jsonLatest;
		rapidjson::Document docLatest;
		int size = jsonFile.getSize();
		jsonLatest.resize(size);
		jsonFile.read(&jsonLatest[0], size);
		docLatest.Parse(jsonLatest.c_str());

		// Get the latest file
		std::string tag = docLatest["tag_name"].GetString();

		std::string jsonFilename = cpp3ds::FileSystem::getFilePath(_(FREESHOP_DIR "/cache/features.%s.json", countryCode.c_str()));
		if (!pathExists(jsonFilename.c_str()))
		{
			Download download(_("https://github.com/Arc13/feature-cache-releases/releases/download/%s/features.%s.json", tag.c_str(), countryCode.c_str()), jsonFilename);
			download.run();

			// If the download returned 404, the language is not (yet) available, retry with the default file
			if (download.getLastResponse().getStatus() == cpp3ds::Http::Response::NotFound) {
				remove(jsonFilename.c_str());
				countryCode = "GB";

				jsonFilename = _(FREESHOP_DIR "/cache/features.%s.json", countryCode.c_str());
				Download downloadRetry(_("https://github.com/Arc13/feature-cache-releases/releases/download/%s/features.%s.json", tag.c_str(), countryCode.c_str()), jsonFilename);
				downloadRetry.run();
			}
		}

		rapidjson::Document doc;
		std::string json;
		cpp3ds::FileInputStream file;
		if (file.open(jsonFilename))
		{
			json.resize(file.getSize());
			file.read(&json[0], json.size());
			doc.Parse(json.c_str());
			if (doc.HasParseError())
			{
				unlink(cpp3ds::FileSystem::getFilePath(jsonFilename).c_str());
				return;
			}

			CheckBoxWithLabel* checkbox;
			Label *labelCount;
			rapidjson::Value &list = doc;
			std::vector<std::pair<int, std::string>> features;
			for (rapidjson::Value::ConstMemberIterator itr = doc.MemberBegin(); itr != doc.MemberEnd(); ++itr) {
				int featureId = std::stoi(itr->name.GetString());
				std::string featureName = itr->value.GetString();
				features.push_back(std::make_pair(featureId, featureName));
			}

			// Alphabetical sort
			std::sort(features.begin(), features.end(), [=](std::pair<int, std::string>& a, std::pair<int, std::string>& b) {
				return a.second < b.second;
			});

			for (int i = 0; i < features.size(); ++i)
			{
				int featureId = features[i].first;
				std::string &featureName = features[i].second;
				int gameCount = 0;
				for (auto &app : AppList::getInstance().getList())
					for (auto &id : app->getAppItem()->getFeatures())
						if (id == featureId)
							gameCount++;

				if (gameCount > 0) {
					labelCount = new Label(parent);
					labelCount->SetText(_("%d", gameCount).toAnsiString());
					labelCount->SetBounds(0, 2 + m_filterFeatureCheckboxes.size() * 18, 31, 18);
					labelCount->SetAlignment(Gwen::Pos::Right);

					checkbox = new CheckBoxWithLabel(parent);
					checkbox->SetPos(35, m_filterFeatureCheckboxes.size() * 18);
					checkbox->Label()->SetText(featureName);
					checkbox->Checkbox()->SetValue(_("%d", featureId).toAnsiString());
					checkbox->Checkbox()->onCheckChanged.Add(this, &Settings::filterCheckboxChanged);

					m_filterFeatureCheckboxes.push_back(checkbox);
				}
			}
		}
	}
}


void Settings::fillFilterRegions(Gwen::Controls::Base *parent)
{
	for (int i = 0; i < 8; ++i)
	{
		cpp3ds::String strRegion;
		int region;
		if (i < 8)
			region = 1 << i;
		else
			region = 0x7FFFFFFF;
		int count = 0;

		// Get region title counts
		for (auto &app : AppList::getInstance().getList())
			if (app->getAppItem()->getRegions() & region)
				count++;

		if (count > 0) {
			if (i == 0) strRegion = _("Japan");
			else if (i == 1) strRegion = _("USA");
			else if (i == 2) strRegion = _("Europe");
			else if (i == 3) strRegion = _("Australia");
			else if (i == 4) strRegion = _("China");
			else if (i == 5) strRegion = _("Korea");
			else if (i == 6) strRegion = _("Taiwan");
			else strRegion = _("Region Free");

			auto labelCount = new Label(parent);
			labelCount->SetText(_("%d", count).toAnsiString());
			labelCount->SetBounds(0, 2 + i * 18, 31, 18);
			labelCount->SetAlignment(Gwen::Pos::Right);

			auto checkbox = new CheckBoxWithLabel(parent);
			checkbox->SetPos(35, i * 18);
			checkbox->Label()->SetText(strRegion.toAnsiString());
			checkbox->Checkbox()->SetValue(_("%d", region).toAnsiString());
			checkbox->Checkbox()->onCheckChanged.Add(this, &Settings::filterRegionCheckboxChanged);

			m_filterRegionCheckboxes.push_back(checkbox);
		}
	}

	auto labelSpacer = new Label(parent);
	labelSpacer->SetText("");
	labelSpacer->SetBounds(0, 2 + m_filterRegionCheckboxes.size() * 18, 31, 18);
	labelSpacer->SetAlignment(Gwen::Pos::Right);
}

void Settings::fillFilterLanguages(Gwen::Controls::Base *parent)
{
	CheckBoxWithLabel* checkbox;
	Label *labelCount;

	for (int i = 0; i < 9; ++i)
	{
		auto lang = static_cast<FreeShop::AppItem::Language>(1 << i);
		cpp3ds::String langString;
		switch (lang) {
			default:
			case FreeShop::AppItem::English: langString = _("English"); break;
			case FreeShop::AppItem::Japanese: langString = _("Japanese"); break;
			case FreeShop::AppItem::Spanish: langString = _("Spanish"); break;
			case FreeShop::AppItem::French: langString = _("French"); break;
			case FreeShop::AppItem::German: langString = _("German"); break;
			case FreeShop::AppItem::Italian: langString = _("Italian"); break;
			case FreeShop::AppItem::Dutch: langString = _("Dutch"); break;
			case FreeShop::AppItem::Portuguese: langString = _("Portuguese"); break;
			case FreeShop::AppItem::Russian: langString = _("Russian"); break;
		}

		// Get language title counts
		int gameCount = 0;
		for (auto &app : AppList::getInstance().getList())
			if (app->getAppItem()->getLanguages() & lang)
				gameCount++;

		if (gameCount > 0) {
			labelCount = new Label(parent);
			labelCount->SetText(_("%d", gameCount).toAnsiString());
			labelCount->SetBounds(0, 2 + m_filterLanguageCheckboxes.size() * 18, 31, 18);
			labelCount->SetAlignment(Gwen::Pos::Right);

			checkbox = new CheckBoxWithLabel(parent);
			checkbox->SetPos(35, m_filterLanguageCheckboxes.size() * 18);
			checkbox->Label()->SetText(langString.toAnsiString());
			checkbox->Checkbox()->SetValue(_("%d", lang).toAnsiString());
			checkbox->Checkbox()->onCheckChanged.Add(this, &Settings::filterCheckboxChanged);

			m_filterLanguageCheckboxes.push_back(checkbox);
		}
	}
}

void Settings::filterSaveClicked(Gwen::Controls::Base *base)
{
	// Region
	saveFilter(Config::FilterRegion, m_filterRegionCheckboxes);
	saveFilter(Config::FilterGenre, m_filterGenreCheckboxes);
	saveFilter(Config::FilterLanguage, m_filterLanguageCheckboxes);
	saveFilter(Config::FilterPlatform, m_filterPlatformCheckboxes);
	saveFilter(Config::FilterPublisher, m_filterPublisherCheckboxes);
	saveFilter(Config::FilterFeatures, m_filterFeatureCheckboxes);

	m_buttonFilterSaveClear->SetDisabled(false);
	Config::saveToFile();
	Notification::spawn(_("Filter settings saved"));
}

void Settings::filterClearClicked(Gwen::Controls::Base *base)
{
	Config::set(Config::FilterRegion, rapidjson::kArrayType);
	Config::set(Config::FilterGenre, rapidjson::kArrayType);
	Config::set(Config::FilterLanguage, rapidjson::kArrayType);
	Config::set(Config::FilterPlatform, rapidjson::kArrayType);
	Config::set(Config::FilterPublisher, rapidjson::kArrayType);
	Config::set(Config::FilterFeatures, rapidjson::kArrayType);

	m_buttonFilterSaveClear->SetDisabled(true);
	Config::saveToFile();
	Notification::spawn(_("Cleared saved filter settings"));
}

#define CHECKBOXES_SET(checkboxes, value) \
	{ \
        for (auto &checkbox : checkboxes) \
            checkbox->Checkbox()->SetChecked(value); \
		m_ignoreCheckboxChange = false; \
		auto checkbox = checkboxes.front()->Checkbox(); \
		checkbox->onCheckChanged.Call(checkbox); \
    }

void Settings::selectAll(Gwen::Controls::Base *control)
{
	std::string filterName = control->GetParent()->GetParent()->GetName();
	m_ignoreCheckboxChange = true;
	if (filterName == "Regions")
		CHECKBOXES_SET(m_filterRegionCheckboxes, true)
	else if (filterName == "Genre")
		CHECKBOXES_SET(m_filterGenreCheckboxes, true)
	else if (filterName == "Language")
		CHECKBOXES_SET(m_filterLanguageCheckboxes, true)
	else if (filterName == "Platform")
		CHECKBOXES_SET(m_filterPlatformCheckboxes, true)
	else if (filterName == "Feature")
		CHECKBOXES_SET(m_filterFeatureCheckboxes, true)
	else if (filterName == "Publisher")
		CHECKBOXES_SET(m_filterPublisherCheckboxes, true)
}

void Settings::selectNone(Gwen::Controls::Base *control)
{
	std::string filterName = control->GetParent()->GetParent()->GetName();
	m_ignoreCheckboxChange = true;
	if (filterName == "Regions")
		CHECKBOXES_SET(m_filterRegionCheckboxes, false)
	else if (filterName == "Genre")
		CHECKBOXES_SET(m_filterGenreCheckboxes, false)
	else if (filterName == "Language")
		CHECKBOXES_SET(m_filterLanguageCheckboxes, false)
	else if (filterName == "Platform")
		CHECKBOXES_SET(m_filterPlatformCheckboxes, false)
	else if (filterName == "Feature")
		CHECKBOXES_SET(m_filterFeatureCheckboxes, false)
	else if (filterName == "Publisher")
		CHECKBOXES_SET(m_filterPublisherCheckboxes, false)
}

void Settings::filterCheckboxChanged(Gwen::Controls::Base *control)
{
	if (m_ignoreCheckboxChange)
		return;
	std::string filterName = control->GetParent()->GetParent()->GetParent()->GetName();
	if (filterName == "Genre")
	{
		std::vector<int> genres;
		for (auto &checkbox : m_filterGenreCheckboxes)
			if (checkbox->Checkbox()->IsChecked())
			{
				int genreId = atoi(checkbox->Checkbox()->GetValue().c_str());
				genres.push_back(genreId);
			}
		AppList::getInstance().setFilterGenres(genres);
	}
	else if (filterName == "Language")
	{
		int languages = 0;
		for (auto &checkbox : m_filterLanguageCheckboxes)
			if (checkbox->Checkbox()->IsChecked())
				languages |= atoi(checkbox->Checkbox()->GetValue().c_str());
		AppList::getInstance().setFilterLanguages(languages);
	}
	else if (filterName == "Platform")
	{
		std::vector<int> platforms;
		for (auto &checkbox : m_filterPlatformCheckboxes)
			if (checkbox->Checkbox()->IsChecked())
			{
				int platformId = atoi(checkbox->Checkbox()->GetValue().c_str());
				platforms.push_back(platformId);
			}
		AppList::getInstance().setFilterPlatforms(platforms);
	}
	else if (filterName == "Feature")
	{
		std::vector<int> features;
		for (auto &checkbox : m_filterFeatureCheckboxes)
			if (checkbox->Checkbox()->IsChecked())
			{
				int genreId = atoi(checkbox->Checkbox()->GetValue().c_str());
				features.push_back(genreId);
			}
		AppList::getInstance().setFilterFeatures(features);
	}
	else if (filterName == "Publisher")
	{
		std::vector<int> publishers;
		for (auto &checkbox : m_filterPublisherCheckboxes)
			if (checkbox->Checkbox()->IsChecked())
			{
				int publisherId = atoi(checkbox->Checkbox()->GetValue().c_str());
				publishers.push_back(publisherId);
			}
		AppList::getInstance().setFilterPublishers(publishers);
	}
}

void Settings::filterRegionCheckboxChanged(Gwen::Controls::Base *control)
{
	if (m_ignoreCheckboxChange)
		return;
	int regions = 0;
	for (auto &checkbox : m_filterRegionCheckboxes)
		if (checkbox->Checkbox()->IsChecked())
		{
			int region = atoi(checkbox->Checkbox()->GetValue().c_str());
			regions |= region;
		}
	AppList::getInstance().setFilterRegions(regions);
}

Gwen::Controls::ScrollControl *Settings::addFilterPage(const std::string &name)
{
	// Add tab page
	Base *filterPage = m_filterTabControl->AddPage(_(name).toAnsiString())->GetPage();
	filterPage->SetName(name);
	// Add group parent for the buttons
	Base *group = new Base(filterPage);
	group->Dock(Gwen::Pos::Top);
	group->SetSize(200, 18);
	group->SetMargin(Gwen::Margin(0, 0, 0, 3));
	// Add the two Select buttons
	Button *button = new Button(group);
	button->Dock(Gwen::Pos::Left);
	button->SetText(_("Select All").toAnsiString());
	button->onPress.Add(this, &Settings::selectAll);
	button = new Button(group);
	button->Dock(Gwen::Pos::Right);
	button->SetText(_("Select None").toAnsiString());
	button->onPress.Add(this, &Settings::selectNone);
	// Scrollbox to put under the two buttons
	ScrollControl *scrollBox = new ScrollControl(filterPage);
	scrollBox->Dock(Gwen::Pos::Fill);
	scrollBox->SetScroll(false, true);
	// Return scrollbox to be filled with controls (probably checkboxes)
	return scrollBox;
}

Gwen::Controls::ScrollControl *Settings::addSortPage(const std::string &name)
{
	// Add tab page
	Base *sortPage = m_sortTabControl->AddPage(_(name).toAnsiString())->GetPage();
	sortPage->SetName(name);
	// Scrollbox to put under the two buttons
	ScrollControl *scrollBox = new ScrollControl(sortPage);
	scrollBox->Dock(Gwen::Pos::Fill);
	scrollBox->SetScroll(false, true);
	// Return scrollbox to be filled with controls (probably checkboxes)
	return scrollBox;
}

void Settings::fillSortPage(Gwen::Controls::Base *page)
{
	m_sortTabControl = new TabControl(page);
	m_sortTabControl->Dock(Gwen::Pos::Fill);
	m_sortTabControl->SetTabStripPosition(Gwen::Pos::Left);

	m_buttonFilterSaveSort = new Button(page);
	m_buttonFilterSaveSort->SetFont(L"fonts/fontawesome.ttf", 18, false);
	m_buttonFilterSaveSort->SetText("\uf0c7"); // Save floppy icon
	m_buttonFilterSaveSort->SetBounds(0, 140, 22, 22);
	m_buttonFilterSaveSort->SetPadding(Gwen::Padding(0, 0, 0, 3));
	m_buttonFilterSaveSort->onPress.Add(this, &Settings::sortSaveClicked);

	m_buttonFilterSaveClearSort = new Button(page);
	m_buttonFilterSaveClearSort->SetFont(L"fonts/fontawesome.ttf", 18, false);
	m_buttonFilterSaveClearSort->SetText("\uf00d"); // Cross icon
	m_buttonFilterSaveClearSort->SetBounds(26, 140, 22, 22);
	m_buttonFilterSaveClearSort->SetPadding(Gwen::Padding(0, 0, 0, 3));
	m_buttonFilterSaveClearSort->onPress.Add(this, &Settings::sortClearClicked);

	ScrollControl *scrollBox;
	scrollBox = addSortPage("Game List");
	fillSortGameList(scrollBox);
	scrollBox = addSortPage("Installed");
	fillSortInstalledList(scrollBox);
}

void Settings::fillSortGameList(Gwen::Controls::Base *parent)
{
	m_radioSortType = new RadioButtonController(parent);
	m_radioSortType->SetBounds(10, 60, 180, 120);
	m_sortGameListRadioButtons.push_back(m_radioSortType->AddOption(_("Release Date").toWideString(), "Release Date"));
	m_sortGameListRadioButtons.push_back(m_radioSortType->AddOption(_("Name").toWideString(), "Name"));
	m_sortGameListRadioButtons.push_back(m_radioSortType->AddOption(_("Size").toWideString(), "Size"));
	m_sortGameListRadioButtons.push_back(m_radioSortType->AddOption(_("Vote Score").toWideString(), "Vote Score"));
	m_sortGameListRadioButtons.push_back(m_radioSortType->AddOption(_("Vote Count").toWideString(), "Vote Count"));

	m_radioSortDirection = new RadioButtonController(parent);
	m_radioSortDirection->SetBounds(10, 0, 150, 40);
	m_sortGameListRadioButtonsDirection.push_back(m_radioSortDirection->AddOption(_("Ascending").toWideString(), "Ascending"));
	m_sortGameListRadioButtonsDirection.push_back(m_radioSortDirection->AddOption(_("Descending").toWideString(), "Descending"));

	loadSort(Config::SortGameList, m_sortGameListRadioButtons);
	loadSort(Config::SortGameListDirection, m_sortGameListRadioButtonsDirection);

	m_radioSortType->onSelectionChange.Add(this, &Settings::updateSorting);
	m_radioSortDirection->onSelectionChange.Add(this, &Settings::updateSorting);
}

void Settings::fillSortInstalledList(Gwen::Controls::Base *parent)
{
	m_radioSortTypeInstalled = new RadioButtonController(parent);
	m_radioSortTypeInstalled->SetBounds(10, 60, 180, 120);
	m_sortInstalledListRadioButtons.push_back(m_radioSortTypeInstalled->AddOption(_("Release Date").toWideString(), "Release Date"));
	m_sortInstalledListRadioButtons.push_back(m_radioSortTypeInstalled->AddOption(_("Name").toWideString(), "Name"));
	m_sortInstalledListRadioButtons.push_back(m_radioSortTypeInstalled->AddOption(_("Size").toWideString(), "Size"));
	m_sortInstalledListRadioButtons.push_back(m_radioSortTypeInstalled->AddOption(_("Vote Score").toWideString(), "Vote Score"));
	m_sortInstalledListRadioButtons.push_back(m_radioSortTypeInstalled->AddOption(_("Vote Count").toWideString(), "Vote Count"));

	m_radioSortDirectionInstalled = new RadioButtonController(parent);
	m_radioSortDirectionInstalled->SetBounds(10, 0, 150, 40);
	m_sortInstalledListRadioButtonsDirection.push_back(m_radioSortDirectionInstalled->AddOption(_("Ascending").toWideString(), "Ascending"));
	m_sortInstalledListRadioButtonsDirection.push_back(m_radioSortDirectionInstalled->AddOption(_("Descending").toWideString(), "Descending"));

	loadSort(Config::SortInstalledList, m_sortInstalledListRadioButtons);
	loadSort(Config::SortInstalledListDirection, m_sortInstalledListRadioButtonsDirection);

	m_radioSortTypeInstalled->onSelectionChange.Add(this, &Settings::updateSorting);
	m_radioSortDirectionInstalled->onSelectionChange.Add(this, &Settings::updateSorting);
}

void Settings::sortSaveClicked(Gwen::Controls::Base *base)
{
	// Region
	saveSort(Config::SortGameList, m_sortGameListRadioButtons);
	saveSort(Config::SortGameListDirection, m_sortGameListRadioButtonsDirection);
	saveSort(Config::SortInstalledList, m_sortInstalledListRadioButtons);
	saveSort(Config::SortInstalledListDirection, m_sortInstalledListRadioButtonsDirection);

	Config::saveToFile();
	Notification::spawn(_("Sorting settings saved"));
}

void Settings::sortClearClicked(Gwen::Controls::Base *base)
{
	Config::set(Config::SortGameList, 0);
	Config::set(Config::SortGameListDirection, 1);
	Config::set(Config::SortInstalledList, 1);
	Config::set(Config::SortInstalledListDirection, 0);

	loadSort(Config::SortGameList, m_sortGameListRadioButtons);
	loadSort(Config::SortGameListDirection, m_sortGameListRadioButtonsDirection);
	loadSort(Config::SortInstalledList, m_sortInstalledListRadioButtons);
	loadSort(Config::SortInstalledListDirection, m_sortInstalledListRadioButtonsDirection);

	Config::saveToFile();
	Notification::spawn(_("Sorting settings reset"));
}

void Settings::saveSort(Config::Key key, std::vector<Gwen::Controls::LabeledRadioButton*> &radioArray)
{
	int i = 0;

	for (auto &radio : radioArray) {
		if (radio->GetRadioButton()->IsChecked()) {
			Config::set(key, i);
			break;
		}
		i++;
	}
}

void Settings::loadSort(Config::Key key, std::vector<Gwen::Controls::LabeledRadioButton*> &radioArray)
{
	int radioIndex = Config::get(key).GetInt();
	radioIndex %= 5;

	radioArray[radioIndex]->Select();
}

void Settings::fillUpdatePage(Gwen::Controls::Base *page)
{
	Gwen::Padding iconPadding(0, 0, 0, 4);
	auto base = new Base(page);
	base->SetBounds(0, 0, 320, 60);

	m_checkboxDownloadKeys = new CheckBoxWithLabel(base);
	m_checkboxDownloadKeys->SetBounds(0, 0, 300, 20);
	m_checkboxDownloadKeys->Label()->SetText(_("Auto-update title keys from URL").toAnsiString());
	m_checkboxDownloadKeys->Checkbox()->onCheckChanged.Add(this, &Settings::updateEnabledState);

	m_checkboxDownloadMultipleKeys = new CheckBoxWithLabel(base);
	m_checkboxDownloadMultipleKeys->SetBounds(0, 20, 300, 20);
	m_checkboxDownloadMultipleKeys->Label()->SetText(_("Download title keys from multiple URLs").toAnsiString());

	m_buttonUrlQr = new Button(base);
	m_buttonUrlQr->SetFont(L"fonts/fontawesome.ttf", 18, false);
	m_buttonUrlQr->SetText("\uf029"); // QR icon
	m_buttonUrlQr->SetPadding(iconPadding);
	m_buttonUrlQr->SetBounds(0, 40, 20, 20);
	m_buttonUrlQr->onPress.Add(this, &Settings::updateQrClicked);

	m_buttonUrlKeyboard = new Button(base);
	m_buttonUrlKeyboard->SetFont(L"fonts/fontawesome.ttf", 20, false);
	m_buttonUrlKeyboard->SetText("\uf11c"); // Keyboard icon
	m_buttonUrlKeyboard->SetPadding(Gwen::Padding(0, 0, 0, 6));
	m_buttonUrlKeyboard->SetBounds(21, 40, 26, 20);
	m_buttonUrlKeyboard->onPress.Add(this, &Settings::updateKeyboardClicked);

	m_buttonUrlDelete = new Button(base);
	m_buttonUrlDelete->SetFont(L"fonts/fontawesome.ttf", 18, false);
	m_buttonUrlDelete->SetText("\uf014"); // Trash can icon
	m_buttonUrlDelete->SetPadding(iconPadding);
	m_buttonUrlDelete->SetBounds(288, 40, 20, 20);
	m_buttonUrlDelete->onPress.Add(this, &Settings::updateUrlDeleteClicked);

	m_comboBoxUrls = new ComboBox(base);
	m_comboBoxUrls->SetBounds(50, 40, 235, 20);
	m_comboBoxUrls->onSelection.Add(this, &Settings::updateUrlSelected);


	base = new Base(page);
	base->SetBounds(0, 68, 320, 80);

	m_checkboxAutoUpdate = new CheckBoxWithLabel(base);
	m_checkboxAutoUpdate->SetBounds(0, 0, 300, 20);
	m_checkboxAutoUpdate->Label()->SetText(_("Auto-update freeShop").toAnsiString());
	m_checkboxAutoUpdate->Checkbox()->onCheckChanged.Add(this, &Settings::updateEnabledState);

	// Button to update - disabled when auto-update is checked
	// Label date last checked (and current version)
	m_buttonUpdate = new Button(base);
	m_buttonUpdate->SetBounds(0, 22, 75, 18);
	m_buttonUpdate->SetText(_("Update").toAnsiString());
	m_buttonUpdate->onPress.Add(this, &Settings::updateCheckUpdateClicked);

	m_labelLastUpdated = new Label(base);
	m_labelLastUpdated->SetBounds(0, 60, 300, 20);
	m_labelLastUpdated->SetTextColor(Gwen::Color(0, 0, 0, 80));

	auto button = new Button(base);
	button->SetBounds(80, 22, 105, 18);
	button->SetText(_("Refresh Cache").toAnsiString());
	button->onPress.Add(this, &Settings::updateRefreshCacheClicked);

	auto label = new Label(base);
	label->SetBounds(0, 44, 300, 20);
	label->SetText(_("freeShop %s", FREESHOP_VERSION).toAnsiString());
}

void Settings::fillDownloadPage(Gwen::Controls::Base *page)
{
	m_checkboxPlaySound = new CheckBoxWithLabel(page);
	m_checkboxPlaySound->SetBounds(0, 0, 320, 20);
	m_checkboxPlaySound->Label()->SetText(_("Play sound when queue finishes").toAnsiString());

	m_checkboxPowerOff = new CheckBoxWithLabel(page);
	m_checkboxPowerOff->SetBounds(0, 20, 320, 20);
	m_checkboxPowerOff->Label()->SetText(_("Power off after download inactivity").toAnsiString());
	m_checkboxPowerOff->Checkbox()->onCheckChanged.Add(this, &Settings::downloadPowerOffClicked);

	m_comboPowerOffTime = new ComboBox(page);
	m_comboPowerOffTime->SetDisabled(true);
	m_comboPowerOffTime->SetBounds(50, 40, 90, 20);
	m_comboPowerOffTime->AddItem(_("1 minute"), "60");
	m_comboPowerOffTime->AddItem(_("%d minutes", 2), "120");
	m_comboPowerOffTime->AddItem(_("%d minutes", 3), "180");
	m_comboPowerOffTime->AddItem(_("%d minutes", 4), "240");
	m_comboPowerOffTime->AddItem(_("%d minutes", 5), "300");

	m_labelTimeout = new Label(page);
	m_labelTimeout->SetBounds(0, 100, 150, 20);
	m_sliderTimeout = new HorizontalSlider(page);
	m_sliderTimeout->SetBounds(0, 116, 140, 20);
	m_sliderTimeout->SetRange(3, 12);
	m_sliderTimeout->SetNotchCount(9);
	m_sliderTimeout->SetClampToNotches(true);
	m_sliderTimeout->onValueChanged.Add(this, &Settings::downloadTimeoutChanged);

	m_labelDownloadBufferSize = new Label(page);
	m_labelDownloadBufferSize->SetBounds(155, 100, 150, 20);
	m_sliderDownloadBufferSize = new HorizontalSlider(page);
	m_sliderDownloadBufferSize->SetBounds(150, 116, 160, 20);
	m_sliderDownloadBufferSize->SetRange(1, 320);
	m_sliderDownloadBufferSize->onValueChanged.Add(this, &Settings::downloadBufferSizeChanged);

	auto button = new Button(page);
	button->SetBounds(95, 140, 110, 18);
	button->SetText(_("Use Defaults").toAnsiString());
	button->onPress.Add(this, &Settings::downloadUseDefaultsClicked);
}

void Settings::downloadPowerOffClicked(Gwen::Controls::Base *base)
{
	m_comboPowerOffTime->SetDisabled(!m_checkboxPowerOff->Checkbox()->IsChecked());
}

void Settings::downloadUseDefaultsClicked(Gwen::Controls::Base *base)
{
	m_sliderTimeout->SetFloatValue(6.f);
	m_sliderDownloadBufferSize->SetFloatValue(128u);
}

void Settings::downloadTimeoutChanged(Gwen::Controls::Base *base)
{
	auto slider = gwen_cast<HorizontalSlider>(base);
	m_labelTimeout->SetText(_("HTTP Timeout: %.0fs", slider->GetFloatValue()).toAnsiString());
}

void Settings::downloadBufferSizeChanged(Gwen::Controls::Base *base)
{
	auto slider = gwen_cast<HorizontalSlider>(base);
	m_labelDownloadBufferSize->SetText(_("Buffer size: %u KB", static_cast<size_t>(std::ceil(slider->GetFloatValue()))).toAnsiString());
}

void Settings::fillMusicPage(Gwen::Controls::Base *page)
{
	m_comboMusicMode = new ComboBox(page);
	m_comboMusicMode->SetBounds(10, 0, 290, 20);

	m_comboMusicMode->AddItem(_("Disable music"), "disable");
	m_comboMusicMode->AddItem(_("Play eShop music"), "eshop");
	m_comboMusicMode->AddItem(_("Play random track"), "random");
	m_comboMusicMode->AddItem(_("Play selected track"), "selected");

	m_listboxMusicFiles = new ListBox(page);
	m_listboxMusicFiles->SetBounds(10, 25, 290, 95);

	m_comboMusicMode->onSelection.Add(this, &Settings::musicComboChanged);
	m_listboxMusicFiles->onRowSelected.Add(this, &Settings::musicFileChanged);

	// Fill listbox with files from /music/ subdir
	DIR *dir;
	struct stat sb;
	struct dirent *ent;
	if (dir = opendir(cpp3ds::FileSystem::getFilePath(FREESHOP_DIR "/music").c_str()))
	{
		while (ent = readdir(dir))
		{
			std::string filename = ent->d_name;
			std::string path = cpp3ds::FileSystem::getFilePath(FREESHOP_DIR "/music/") + filename;

			if (filename == "." || filename == "..")
				continue;

			if (stat(path.c_str(), &sb) == 0 && S_ISREG(sb.st_mode))
				m_listboxMusicFiles->AddItem(filename);
		}
		closedir (dir);
	}

	m_labelAudioState = new Label(page);
	m_labelAudioState->SetBounds(10, 122, 320, 20);
	if (cpp3ds::Service::isEnabled(cpp3ds::Audio)) {
		m_labelAudioState->SetText(_("Audio Service: Enabled").toAnsiString());
		m_labelAudioState->SetTextColor(Gwen::Color(63, 81, 181, 255));
	} else {
		m_labelAudioState->SetText(_("Audio Service: Disabled, please dump your DSP").toAnsiString());
		m_labelAudioState->SetTextColor(Gwen::Color(244, 67, 54, 255));
	}

	m_checkboxTurnOffMusicSlider = new CheckBoxWithLabel(page);
	m_checkboxTurnOffMusicSlider->SetBounds(10, 140, 320, 20);
	m_checkboxTurnOffMusicSlider->Label()->SetText(_("Turn off the music if the volume slider is too low").toAnsiString());
}

void Settings::fillLocalesPage(Gwen::Controls::Base *page)
{
	auto label = new Label(page);
	label->SetBounds(0, 0, 150, 20);
	label->SetText(_("Language:").toAnsiString());

	auto label2 = new Label(page);
	label2->SetBounds(130, 0, 150, 20);
	label2->SetText(_("Keyboard:").toAnsiString());

	m_checkboxSystemKeyboard = new CheckBoxWithLabel(page);
	m_checkboxSystemKeyboard->SetBounds(0, 143, 320, 20);
	m_checkboxSystemKeyboard->Label()->SetText(_("Use system keyboard").toAnsiString());
	m_checkboxSystemKeyboard->Checkbox()->onCheckChanged.Add(this, &Settings::systemKeyboardChange);

	m_listboxLanguages = new ListBox(page);
	m_listboxLanguages->SetBounds(0, 20, 120, 120);
	m_listboxLanguages->AddItem(_("Auto-detect").toAnsiString(), "auto");

	m_listboxKeyboards = new ListBox(page);
	m_listboxKeyboards->SetBounds(130, 20, 120, 120);
	m_listboxKeyboards->AddItem("QWERTY", "qwerty");
	m_listboxKeyboards->AddItem("AZERTY", "azerty");
	m_listboxKeyboards->AddItem("QWERTZ", "qwertz");
	m_listboxKeyboards->AddItem("Katakana", "jap");
	m_listboxKeyboards->AddItem("Title ID", "tid");
	m_listboxKeyboards->onRowSelected.Add(this, &Settings::keyboardChange);

	// Only show Korean/Chinese on their respective region consoles.
	// Other consoles don't have the necessary system font, so it looks bad.
#ifdef _3DS
	u8 region;
	CFGU_SecureInfoGetRegion(&region);
	switch (region) {
		case CFG_REGION_KOR:
			m_listboxLanguages->AddItem("한국어",  "kr");
			break;
		case CFG_REGION_CHN:
		case CFG_REGION_TWN:
			m_listboxLanguages->AddItem("简体中文", "zh");
			m_listboxLanguages->AddItem("繁體中文", "tw");
			// Fall through
		default:
			m_listboxLanguages->AddItem("日本語", "jp");
	}
#endif

	m_listboxLanguages->AddItem("English", "en");
	m_listboxLanguages->AddItem("Français", "fr");
	m_listboxLanguages->AddItem("Deutsche", "de");
	m_listboxLanguages->AddItem("Español US", "es_US");
	m_listboxLanguages->AddItem("Español ES", "es_ES");
	m_listboxLanguages->AddItem("Português BR", "pt_BR");
	m_listboxLanguages->AddItem("Português PT", "pt_PT");
	m_listboxLanguages->AddItem("Italiano", "it");
	m_listboxLanguages->AddItem("Nederlands", "nl");
	m_listboxLanguages->AddItem("русский", "ru");
	m_listboxLanguages->AddItem("Polski", "pl");
	m_listboxLanguages->AddItem("Magyar", "hu");
	m_listboxLanguages->AddItem("Română", "ro");
	m_listboxLanguages->AddItem("Ελληνικά", "gr");
	m_listboxLanguages->AddItem("Türkçe", "tr");
	m_listboxLanguages->onRowSelected.Add(this, &Settings::languageChange);
}

void Settings::fillOtherPage(Gwen::Controls::Base *page)
{
	auto newsButton = new Button(page);
	newsButton->SetBounds(220, 140, 85, 20);
	newsButton->SetText(_("News", FREESHOP_VERSION).toAnsiString());
	newsButton->onPress.Add(this, &Settings::showNews);

	m_checkboxTitleID = new CheckBoxWithLabel(page);
	m_checkboxTitleID->SetBounds(0, 0, 320, 20);
	m_checkboxTitleID->Label()->SetText(_("Show Title ID in game description screen").toAnsiString());

	m_checkboxGameCounter = new CheckBoxWithLabel(page);
	m_checkboxGameCounter->SetBounds(0, 20, 320, 20);
	m_checkboxGameCounter->Label()->SetText(_("Show the number of game you have").toAnsiString());

	m_checkboxGameDescription = new CheckBoxWithLabel(page);
	m_checkboxGameDescription->SetBounds(0, 40, 320, 20);
	m_checkboxGameDescription->Label()->SetText(_("Show game informations in game info screen").toAnsiString());

#ifndef EMULATION
	if (!envIsHomebrew()) {
#endif
	m_checkboxBatteryPercent = new CheckBoxWithLabel(page);
	m_checkboxBatteryPercent->SetBounds(0, 60, 320, 20);
	m_checkboxBatteryPercent->Label()->SetText(_("Show battery percentage and clock").toAnsiString());
#ifndef EMULATION
	}
#endif

	m_buttonResetEshopMusic = new Button(page);
	m_buttonResetEshopMusic->SetBounds(0, 140, 200, 20);
	m_buttonResetEshopMusic->SetText(_("Update eShop music").toAnsiString());
	m_buttonResetEshopMusic->onPress.Add(this, &Settings::resetEshopMusicClicked);
}

void Settings::updateQrClicked(Gwen::Controls::Base *button)
{
	m_state->requestStackPush(States::QrScanner, true, [this](void *data){
		auto text = reinterpret_cast<cpp3ds::String*>(data);
		if (!text->isEmpty())
		{
			cpp3ds::String strError;
			if (TitleKeys::isValidUrl(*text, &strError))
			{
				m_comboBoxUrls->AddItem(text->toWideString());
				return true;
			}
			*text = strError;
		}
		return false;
	});
}

void Settings::updateCheckUpdateClicked(Gwen::Controls::Base *button)
{
	Config::set(Config::TriggerUpdateFlag, true);
	askUserToRestartApp();
}

void Settings::updateRefreshCacheClicked(Gwen::Controls::Base *button)
{
	Config::set(Config::CacheVersion, "");
	askUserToRestartApp();
}

void Settings::updateKeyboardClicked(Gwen::Controls::Base *textbox)
{
#ifdef _3DS
	KeyboardApplet kb(KeyboardApplet::URL);
	kb.addDictWord("http", "http://");
	kb.addDictWord("https", "https://");
	swkbdSetHintText(kb, "https://<encTitleKeys.bin>");
	cpp3ds::String input = kb.getInput();
	if (!input.isEmpty())
		m_comboBoxUrls->AddItem(input.toWideString());
#else
	m_comboBoxUrls->AddItem(L"Test");
#endif
}

void Settings::updateUrlSelected(Gwen::Controls::Base *combobox)
{
	updateEnabledStates();
}

void Settings::updateUrlDeleteClicked(Gwen::Controls::Base *button)
{
	MenuItem *selected = gwen_cast<MenuItem>(m_comboBoxUrls->GetSelectedItem());
	MenuItem *newSelected = nullptr;
	if (selected)
	{
		// Find next child to select when current one is deleted
		bool foundSelected = false;
		auto menuList = selected->GetParent()->GetChildren();
		for (auto it = menuList.begin(); it != menuList.end(); ++it)
		{
			auto menuItem = gwen_cast<MenuItem>(*it);
			if (foundSelected) {
				newSelected = menuItem;
				break;
			}

			if (menuItem == selected)
				foundSelected = true;
			else
				newSelected = menuItem;
		}

		selected->DelayedDelete();
		m_comboBoxUrls->SelectItem(newSelected);
		updateEnabledStates();
	}
}

void Settings::updateEnabledStates()
{
	Gwen::Color transparent(0,0,0,0); // Transparent cancels the color override
	bool isDownloadEnabled = m_checkboxDownloadKeys->Checkbox()->IsChecked();
	bool isUrlSelected = !!m_comboBoxUrls->GetSelectedItem();

	m_comboBoxUrls->SetDisabled(!isDownloadEnabled);
	m_buttonUrlQr->SetDisabled(!isDownloadEnabled);
	m_buttonUrlKeyboard->SetDisabled(!isDownloadEnabled);
	m_buttonUrlDelete->SetDisabled(!isDownloadEnabled || !isUrlSelected);
	if (!isUrlSelected)
		m_comboBoxUrls->SetText(_("Remote title key URL(s)").toAnsiString());
	if (isDownloadEnabled) {
		m_buttonUrlQr->SetTextColorOverride(Gwen::Color(0, 100, 0));
		m_buttonUrlKeyboard->SetTextColorOverride(Gwen::Color(0, 100, 0));
		m_buttonUrlDelete->SetTextColorOverride(isUrlSelected ? Gwen::Color(150, 0, 0, 200) : transparent);
	} else {
		m_buttonUrlQr->SetTextColorOverride(transparent);
		m_buttonUrlKeyboard->SetTextColorOverride(transparent);
		m_buttonUrlDelete->SetTextColorOverride(transparent);
	}
	m_buttonUpdate->SetDisabled(m_checkboxAutoUpdate->Checkbox()->IsChecked());
}

void Settings::updateEnabledState(Gwen::Controls::Base *control)
{
	updateEnabledStates();
}

void Settings::updateSorting(Gwen::Controls::Base *control)
{
	std::string sortName;
	if (control)
		sortName = control->GetParent()->GetParent()->GetName();

	bool isAscending;

	if (!control || sortName == "Game List") {
		AppList::SortType sortType;
		std::string sortTypeName(m_radioSortType->GetSelectedName());
		isAscending = (m_radioSortDirection->GetSelectedName() == "Ascending");
		if (sortTypeName == "Name")
			sortType = AppList::Name;
		else if (sortTypeName == "Size")
			sortType = AppList::Size;
		else if (sortTypeName == "Vote Score")
			sortType = AppList::VoteScore;
		else if (sortTypeName == "Vote Count")
			sortType = AppList::VoteCount;
		else
			sortType = AppList::ReleaseDate;

		AppList::getInstance().setSortType(sortType, isAscending);
	}

	if (!control || sortName == "Installed") {
		InstalledList::SortType sortTypeInstalled;
		std::string sortTypeNameInstalled(m_radioSortTypeInstalled->GetSelectedName());
		isAscending = (m_radioSortDirectionInstalled->GetSelectedName() == "Ascending");
		if (sortTypeNameInstalled == "Name")
			sortTypeInstalled = InstalledList::Name;
		else if (sortTypeNameInstalled == "Size")
			sortTypeInstalled = InstalledList::Size;
		else if (sortTypeNameInstalled == "Vote Score")
			sortTypeInstalled = InstalledList::VoteScore;
		else if (sortTypeNameInstalled == "Vote Count")
			sortTypeInstalled = InstalledList::VoteCount;
		else
			sortTypeInstalled = InstalledList::ReleaseDate;

		InstalledList::getInstance().setSortType(sortTypeInstalled, isAscending);
	}
}

void Settings::musicComboChanged(Gwen::Controls::Base *combobox)
{
	std::string mode = m_comboMusicMode->GetSelectedItem()->GetName();
	if (mode == "disable")
		g_browseState->stopBGM();
	else if (mode == "eshop")
		g_browseState->playBGMeShop();
	else if (mode == "random")
		selectRandomMusicTrack();
	else // selected
		playSelectedMusic();
}

void Settings::musicFileChanged(Gwen::Controls::Base *base)
{
	std::string mode = m_comboMusicMode->GetSelectedItem()->GetName();
	if (mode == "selected" || mode == "random")
		playSelectedMusic();
}

void Settings::selectRandomMusicTrack()
{
	auto& musicRows = m_listboxMusicFiles->GetChildren().front()->GetChildren();
	if (musicRows.size() > 0)
	{
		size_t randIndex = rand() % musicRows.size();
		int i = 0;
		for (auto it = musicRows.begin(); it != musicRows.end(); ++it, ++i)
			if (i == randIndex)
			{
				m_listboxMusicFiles->SetSelectedRow(*it, true);
				break;
			}
	}
}

void Settings::playSelectedMusic()
{
	if (!m_listboxMusicFiles->GetSelectedRow())
		return;
	std::string filename = m_listboxMusicFiles->GetSelectedRow()->GetText(0).c_str();
	std::string path = FREESHOP_DIR "/music/" + filename;
	g_browseState->playBGM(path);
}

void Settings::playMusic()
{
	musicComboChanged(nullptr);
}

void Settings::showNews(Gwen::Controls::Base *base)
{
	Config::set(Config::ShowNews, true);
}

void Settings::languageChange(Gwen::Controls::Base *base)
{
	std::string langCode = m_listboxLanguages->GetSelectedRowName();
	if (!langCode.empty() && langCode != Config::get(Config::Language).GetString())
	{
		askUserToRestartApp();
	}
}

void Settings::keyboardChange(Gwen::Controls::Base *base)
{
	std::string keyCode = m_listboxKeyboards->GetSelectedRowName();
	if (!keyCode.empty() && keyCode != Config::get(Config::Keyboard).GetString())
	{
		Config::set(Config::Keyboard, m_listboxKeyboards->GetSelectedRow()->GetName().c_str());
		g_browseState->reloadKeyboard();
		Notification::spawn(_("Keyboard changed to: %s", m_listboxKeyboards->GetSelectedRow()->GetText(0).c_str()));
	}
}

void Settings::systemKeyboardChange(Gwen::Controls::Base* base)
{
	Config::set(Config::SystemKeyboard, m_checkboxSystemKeyboard->Checkbox()->IsChecked());
}

void Settings::fillThemeInfoPage(Gwen::Controls::Base *page)
{
	m_labelThemeName = new Label(page);
	m_labelThemeName->SetBounds(0, 0, 150, 20);
	m_labelThemeName->SetText(Theme::themeName);

	m_labelThemeVersion = new Label(page);
	m_labelThemeVersion->SetBounds(270, 0, 40, 20);
	m_labelThemeVersion->SetText(Theme::themeVersion);

	m_labelThemeDescription = new Label(page);
	m_labelThemeDescription->SetText(Theme::themeDesc);
	m_labelThemeDescription->SetTextColor(Gwen::Color(Theme::themeDescColor.r, Theme::themeDescColor.g, Theme::themeDescColor.b, Theme::themeDescColor.a));
	m_labelThemeDescription->SetBounds(0, 20, 320, 210);
}

void Settings::fillThemeSettingsPage(Gwen::Controls::Base *page)
{
	m_checkboxDarkTheme = new CheckBoxWithLabel(page);
	m_checkboxDarkTheme->SetBounds(0, 0, 320, 20);
	m_checkboxDarkTheme->Label()->SetText(_("Enable Dark Theme").toAnsiString());

	m_checkboxRestartFix = new CheckBoxWithLabel(page);
	m_checkboxRestartFix->SetBounds(0, 25, 320, 20);
	m_checkboxRestartFix->Label()->SetText(_("Restart Fix").toAnsiString());
}

void Settings::ifThemeSettingsIsPressed(Gwen::Controls::Base *page)
{
		m_checkboxDarkTheme->Checkbox()->onCheckChanged.Add(this, &Settings::darkThemeRestart);
}

void Settings::darkThemeRestart()
{
		askUserToRestartApp();
}

void Settings::fillNotifiersPage(Gwen::Controls::Base *page)
{
	m_checkboxLEDStartup = new CheckBoxWithLabel(page);
	m_checkboxLEDStartup->SetBounds(0, 0, 320, 20);
	m_checkboxLEDStartup->Label()->SetText(_("Blink the LED on freeShop startup").toAnsiString());

	m_checkboxLEDDownloadFinished = new CheckBoxWithLabel(page);
	m_checkboxLEDDownloadFinished->SetBounds(0, 30, 320, 20);
	m_checkboxLEDDownloadFinished->Label()->SetText(_("Blink the LED when a download finished").toAnsiString());

	m_checkboxLEDDownloadErrored = new CheckBoxWithLabel(page);
	m_checkboxLEDDownloadErrored->SetBounds(0, 50, 320, 20);
	m_checkboxLEDDownloadErrored->Label()->SetText(_("Blink the LED when a download failed").toAnsiString());

	m_checkboxNEWSDownloadFinished = new CheckBoxWithLabel(page);
	m_checkboxNEWSDownloadFinished->SetBounds(0, 80, 320, 20);
	m_checkboxNEWSDownloadFinished->Label()->SetText(_("Send a notification when a download finished").toAnsiString());

	m_checkboxNEWSNoLED = new CheckBoxWithLabel(page);
	m_checkboxNEWSNoLED->SetBounds(0, 100, 320, 20);
	m_checkboxNEWSNoLED->Label()->SetText(_("Disable the LED when the notification is sent").toAnsiString());
}

void Settings::fillInactivityPage(Gwen::Controls::Base *page)
{
	m_checkboxSleep = new CheckBoxWithLabel(page);
	m_checkboxSleep->SetBounds(0, 0, 320, 20);
	m_checkboxSleep->Label()->SetText(_("Enable top screen sleep after inactivity").toAnsiString());

	m_checkboxSleepBottom = new CheckBoxWithLabel(page);
	m_checkboxSleepBottom->SetBounds(0, 20, 320, 20);
	m_checkboxSleepBottom->Label()->SetText(_("Enable bottom screen sleep after inactivity").toAnsiString());

#ifndef EMULATION
	if (!envIsHomebrew()) {
#endif
	m_checkboxDimLEDs = new CheckBoxWithLabel(page);
	m_checkboxDimLEDs->SetBounds(0, 50, 320, 20);
	m_checkboxDimLEDs->Label()->SetText(_("Dim the LEDs after inactivity").toAnsiString());
#ifndef EMULATION
	}
#endif

	m_checkboxInactivitySoundAllowed = new CheckBoxWithLabel(page);
	m_checkboxInactivitySoundAllowed->SetBounds(0, 80, 320, 20);
	m_checkboxInactivitySoundAllowed->Label()->SetText(_("Allow sounds on inactivity mode").toAnsiString());

	m_checkboxInactivityMusicAllowed = new CheckBoxWithLabel(page);
	m_checkboxInactivityMusicAllowed->SetBounds(0, 100, 320, 20);
	m_checkboxInactivityMusicAllowed->Label()->SetText(_("Allow music on inactivity mode").toAnsiString());

	m_labelInactivityTime = new Label(page);
	m_labelInactivityTime->SetBounds(0, 125, 305, 20);
	m_sliderInactivityTime = new HorizontalSlider(page);
	m_sliderInactivityTime->SetBounds(0, 140, 305, 20);
	m_sliderInactivityTime->SetRange(10, 60);
	m_sliderInactivityTime->SetNotchCount(50);
	m_sliderInactivityTime->SetClampToNotches(true);
	m_sliderInactivityTime->onValueChanged.Add(this, &Settings::inactivityTimeChanged);
}

void Settings::inactivityTimeChanged(Gwen::Controls::Base* base)
{
	auto slider = gwen_cast<HorizontalSlider>(base);
	m_labelInactivityTime->SetText(_("Go into inactivity mode after %.0fs", slider->GetFloatValue()).toAnsiString());
}

void Settings::resetEshopMusicClicked(Gwen::Controls::Base *button)
{
#ifndef EMULATION
	Config::set(Config::ResetEshopMusic, true);
	askUserToRestartApp();
#endif
}

void Settings::askUserToRestartApp()
{
	g_browseState->requestStackPush(States::Dialog, false, [=](void *data) mutable {
		auto event = reinterpret_cast<DialogState::Event*>(data);
		if (event->type == DialogState::GetText)
		{
			auto str = reinterpret_cast<cpp3ds::String*>(event->data);
			*str = _("You need to restart freeShop\n\nWould you like to do this now?");
			return true;
		}
		else if (event->type == DialogState::GetTitle) {
			auto str = reinterpret_cast<cpp3ds::String *>(event->data);
			*str = _("Restart freeShop");
			return true;
		}
		else if (event->type == DialogState::Response)
		{
			bool *accepted = reinterpret_cast<bool*>(event->data);
			if (*accepted)
			{
				// Restart freeShop
				Config::saveToFile();
				g_requestJump = 0x400000F12EE00;
			}
			return true;
		}
		return false;
	});
}

} // namespace GUI
} // namespace FreeShop
