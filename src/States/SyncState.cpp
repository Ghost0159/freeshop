#include "SyncState.hpp"
#include "DialogState.hpp"
#include "../Download.hpp"
#include "../Installer.hpp"
#include "../Util.hpp"
#include "../AssetManager.hpp"
#include "../Config.hpp"
#include "../TitleKeys.hpp"
#include "../FreeShop.hpp"
#include "../Theme.hpp"
#include "../Notification.hpp"
#include "../LoadInformations.hpp"
#include <TweenEngine/Tween.h>
#include <cpp3ds/Window/Window.hpp>
#include <cpp3ds/System/I18n.hpp>
#include <cpp3ds/System/FileSystem.hpp>
#include <cpp3ds/System/Sleep.hpp>
#include <sys/stat.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdio.h>
#include <cpp3ds/System/Service.hpp>
#include <archive.h>
#include <archive_entry.h>
#include <dirent.h>
#include <cpp3ds/System/FileInputStream.hpp>
#ifndef EMULATION
#include <3ds.h>
#include "../MCU/Mcu.hpp"
#endif

namespace {

int copy_data(struct archive *ar, struct archive *aw)
{
	int r;
	const void *buff;
	size_t size;
	int64_t offset;

	while (1)
	{
		r = archive_read_data_block(ar, &buff, &size, &offset);
		if (r == ARCHIVE_EOF)
			return (ARCHIVE_OK);
		if (r < ARCHIVE_OK)
			return (r);
		r = archive_write_data_block(aw, buff, size, offset);
		if (r < ARCHIVE_OK)
		{
			fprintf(stderr, "%s\n", archive_error_string(aw));
			return (r);
		}
	}
}

bool extract(const std::string &filename, const std::string &destDir)
{
	struct archive *a;
	struct archive *ext;
	struct archive_entry *entry;
	int r = ARCHIVE_FAILED;

	a = archive_read_new();
	archive_read_support_format_tar(a);
	archive_read_support_compression_gzip(a);
	ext = archive_write_disk_new();
	if ((r = archive_read_open_filename(a, cpp3ds::FileSystem::getFilePath(filename).c_str(), 32*1024))) {
		std::cout << "failure! " << r << std::endl;
		return r;
	}

	while(1)
	{
		r = archive_read_next_header(a, &entry);

		if (r == ARCHIVE_EOF)
			break;
		if (r < ARCHIVE_OK)
			fprintf(stderr, "%s\n", archive_error_string(a));
		// TODO: handle these fatal error
		std::string path = cpp3ds::FileSystem::getFilePath(destDir + archive_entry_pathname(entry));

		if (FreeShop::pathExists(path.c_str(), false))
			unlink(path.c_str());

		archive_entry_set_pathname(entry, path.c_str());
		r = archive_write_header(ext, entry);
		if (r < ARCHIVE_OK)
			fprintf(stderr, "%s\n", archive_error_string(ext));
		else if (archive_entry_size(entry) > 0)
		{
			r = copy_data(a, ext);

			if (r < ARCHIVE_OK)
				fprintf(stderr, "%s\n", archive_error_string(ext));
		}
		r = archive_write_finish_entry(ext);
		if (r < ARCHIVE_OK)
			fprintf(stderr, "%s\n", archive_error_string(ext));
	}

	archive_read_close(a);
	archive_read_free(a);
	archive_write_close(ext);
	archive_write_free(ext);

	return r == ARCHIVE_EOF;
}

}

namespace FreeShop {

bool g_syncComplete = false;
bool g_browserLoaded = false;
bool SyncState::exitRequired = false;

SyncState::SyncState(StateStack& stack, Context& context, StateCallback callback)
: State(stack, context, callback)
, m_threadSync(&SyncState::sync, this)
, m_threadStartupSound(&SyncState::startupSound, this)
{
	g_syncComplete = false;
	g_browserLoaded = false;

	std::string path = cpp3ds::FileSystem::getFilePath(FREESHOP_DIR "/theme/sounds/startup.ogg");
	if (pathExists(path.c_str(), false))
		Theme::isSoundStartupThemed = true;

	if (Theme::isSoundStartupThemed)
		m_soundStartup.setBuffer(AssetManager<cpp3ds::SoundBuffer>::get(FREESHOP_DIR "/theme/sounds/startup.ogg"));
	else
		m_soundStartup.setBuffer(AssetManager<cpp3ds::SoundBuffer>::get("sounds/startup.ogg"));

	m_soundLoading.setBuffer(AssetManager<cpp3ds::SoundBuffer>::get("sounds/loading.ogg"));
	m_soundLoading.setLoop(true);

	std::string pathTexts = cpp3ds::FileSystem::getFilePath(FREESHOP_DIR "/theme/texts.json");
	if (pathExists(pathTexts.c_str(), false)) {
		if (Theme::loadFromFile()) {
			Theme::isTextThemed = true;

			//Load differents colors
			std::string loadingTextValue = Theme::get("loadingText").GetString();

			//Set the colors
			int R, G, B;

			hexToRGB(loadingTextValue, &R, &G, &B);
			Theme::loadingText = cpp3ds::Color(R, G, B);
		}
	}

	m_textStatus.setCharacterSize(14);
	if (Theme::isTextThemed)
		m_textStatus.setFillColor(Theme::loadingText);
	else if (Config::get(Config::DarkTheme).GetBool())
		m_textStatus.setFillColor(cpp3ds::Color(243, 243, 243));
	else
		m_textStatus.setFillColor(cpp3ds::Color::Black);

	m_textStatus.setOutlineColor(cpp3ds::Color(0, 0, 0, 70));
	m_textStatus.setOutlineThickness(2.f);
	m_textStatus.setPosition(160.f, 155.f);
	m_textStatus.useSystemFont();
	TweenEngine::Tween::to(m_textStatus, util3ds::TweenText::FILL_COLOR_ALPHA, 0.3f)
			.target(180)
			.repeatYoyo(-1, 0)
			.start(m_tweenManager);

	m_threadSync.launch();
	m_threadStartupSound.launch();
}


SyncState::~SyncState()
{
	m_threadSync.wait();
	m_threadStartupSound.wait();
}


void SyncState::renderTopScreen(cpp3ds::Window& window)
{
	// Nothing
}

void SyncState::renderBottomScreen(cpp3ds::Window& window)
{
	window.draw(m_textStatus);
}

bool SyncState::update(float delta)
{
	m_tweenManager.update(delta);
	return true;
}

bool SyncState::processEvent(const cpp3ds::Event& event)
{
	if (event.type == cpp3ds::Event::KeyPressed) {
		switch (event.key.code)	{
			case cpp3ds::Keyboard::A: {
#ifndef EMULATION
				if (!cpp3ds::Service::isEnabled(cpp3ds::Httpc)) {
					if (R_SUCCEEDED(nwmExtInit())) {
						NWMEXT_ControlWirelessEnabled(true);
						nwmExtExit();
					}
				}
				break;
#endif
			}
			case cpp3ds::Keyboard::B: {
#ifndef EMULATION
				if (!cpp3ds::Service::isEnabled(cpp3ds::Httpc)) {
					u8 region;
					u64 settingsID;

					Result res = 0;
					u8 hmac[0x20];
					memset(hmac, 0, sizeof(hmac));

					if (R_SUCCEEDED(CFGU_SecureInfoGetRegion(&region))) {
						switch (region) {
							case CFG_REGION_JPN: settingsID = 0x0004001000020000; break;
							case CFG_REGION_USA: settingsID = 0x0004001000021000; break;
							case CFG_REGION_EUR: settingsID = 0x0004001000022000; break;
							case CFG_REGION_AUS: settingsID = 0x0004001000022000; break;
							case CFG_REGION_CHN: settingsID = 0x0004001000026000; break;
							case CFG_REGION_KOR: settingsID = 0x0004001000027000; break;
							case CFG_REGION_TWN: settingsID = 0x0004001000028000; break;
							default: break;
						}
					}

					if (R_SUCCEEDED(res = APT_PrepareToDoApplicationJump(0, settingsID, MEDIATYPE_NAND)))
						res = APT_DoApplicationJump(0, 0, hmac);
				}
				break;
#endif
			}
		}
	}

	return true;
}

void SyncState::sync()
{
	m_timer.restart();

	//No internet connection
	if (!cpp3ds::Service::isEnabled(cpp3ds::Httpc))
	{
		bool isMessagesPrinted = false;

		while (!cpp3ds::Service::isEnabled(cpp3ds::Httpc) && m_timer.getElapsedTime() < cpp3ds::seconds(30.f) && !exitRequired)
		{
			setStatus(_("Waiting for internet connection... %.0fs", 30.f - m_timer.getElapsedTime().asSeconds()));
			cpp3ds::sleep(cpp3ds::milliseconds(200));

			if (!isMessagesPrinted && m_timer.getElapsedTime() >= cpp3ds::seconds(1.f)) {
				Notification::spawn(_("Press the A button to enable WiFi"));
				Notification::spawn(_("Press the B button to open System Settings"));
				isMessagesPrinted = true;
			}
		}
		if (!cpp3ds::Service::isEnabled(cpp3ds::Httpc))
		{
			requestStackClear();
			return;
		}
	}

	// If auto-dated, boot into launch newest freeShop
#ifndef EMULATION
	if (!envIsHomebrew()) {
		if (updateFreeShop())
		{
			Config::set(Config::ShowNews, true);
			g_requestJump = 0x400000F12EE00;
			return;
		}
	} else {
		if (updateFreeShop3DSX())
		{
			// It crashes here :/
		}
	}
#endif


	if (!pathExists(FREESHOP_DIR "/news/" FREESHOP_VERSION ".txt") || std::string(FREESHOP_VERSION) != Config::get(Config::Version).GetString())
	{
		Config::set(Config::Version, FREESHOP_VERSION);
		Config::set(Config::ShowNews, true);
	}

	// Check if the last session exited correctly
	if (!Config::get(Config::CleanExit).GetBool()) {
		Notification::spawn(_("The freeShop wasn't closed correctly the last time.\nDelete the %s folder\nand/or reinstall freeShop if the problem persists.", FREESHOP_DIR));
	}
	Config::set(Config::CleanExit, false);
	Config::saveToFile();

	loadServices();
	loadThemeManagement();
	updateCache();
	updateTitleKeys();
	updateEshopMusic();

	if (Config::get(Config::ShowNews).GetBool())
	{
		setStatus(_("Fetching news for %s ...", FREESHOP_VERSION));
		std::string url = _("https://notabug.org/evi/freeShop/raw/master/news/%s.txt", FREESHOP_VERSION).toAnsiString();
		Download download(url, FREESHOP_DIR "/news/" FREESHOP_VERSION ".txt");
		download.run();
	}

	// TODO: Figure out why browse state won't load without this sleep
	cpp3ds::sleep(cpp3ds::milliseconds(100));
	requestStackPush(States::Browse);

	Config::saveToFile();
	while (m_timer.getElapsedTime() < cpp3ds::seconds(7.f))
		cpp3ds::sleep(cpp3ds::milliseconds(50));

	g_syncComplete = true;
}


bool SyncState::updateFreeShop()
{
	if (!Config::get(Config::AutoUpdate).GetBool() && !Config::get(Config::TriggerUpdateFlag).GetBool())
		return false;

	setStatus(_("Looking for freeShop update..."));
	const char *url = "https://pastebin.com/raw/L3ET5F1G";
	const char *latestFilename = FREESHOP_DIR "/tmp/latest.txt";
	Download download(url, latestFilename);
	download.run();

	cpp3ds::FileInputStream latestFile;
	if (latestFile.open(latestFilename))
	{
		std::string tag;
		rapidjson::Document doc;
		int size = latestFile.getSize();
		tag.resize(size);
		latestFile.read(&tag[0], size);
		if (tag.empty())
			return false;

		Config::set(Config::LastUpdatedTime, static_cast<int>(time(nullptr)));
		Config::saveToFile();

		std::string tagWithoutPoints = ReplaceAll(tag, ".", "");
		std::cout << "Latest version: " << tagWithoutPoints << std::endl;

		std::string actualVersionWithoutPoints = ReplaceAll(FREESHOP_VERSION, ".", "");
		std::cout << "Actual version: " << actualVersionWithoutPoints << std::endl;

		if (std::stoi(tagWithoutPoints) > std::stoi(actualVersionWithoutPoints))
		{
#ifndef EMULATION
			Result ret;
			Handle cia;
			bool suceeded = false;
			size_t total = 0;
			std::string freeShopFile = FREESHOP_DIR "/tmp/freeShop.cia";
			std::string freeShopUrl = _("https://notabug.org/evi/freeshop-versions/raw/master/%s", tag.c_str());
			setStatus(_("Installing freeShop %s ...", tag.c_str()));
			Download freeShopDownload(freeShopUrl, freeShopFile);
			AM_QueryAvailableExternalTitleDatabase(nullptr);

			freeShopDownload.run();
			cpp3ds::Int64 bytesRead;
			cpp3ds::FileInputStream f;
			if (!f.open(freeShopFile))
				return false;
			char *buf = new char[128*1024];

			AM_StartCiaInstall(MEDIATYPE_SD, &cia);
			while (bytesRead = f.read(buf, 128*1024))
			{
				if (R_FAILED(ret = FSFILE_Write(cia, nullptr, total, buf, bytesRead, 0)))
					break;
				total += bytesRead;
			}
			delete[] buf;

			if (R_FAILED(ret))
				setStatus(_("Failed to install update: 0x%08lX", ret));
			suceeded = R_SUCCEEDED(ret = AM_FinishCiaInstall(cia));
			if (!suceeded)
				setStatus(_("Failed to install update: 0x%08lX", ret));
			return suceeded;
#endif
		}
	}
	return false;
}

bool SyncState::updateFreeShop3DSX()
{
	if (!Config::get(Config::AutoUpdate).GetBool() && !Config::get(Config::TriggerUpdateFlag).GetBool())
		return false;

	setStatus(_("Looking for freeShop update..."));
	const char *url = "https://pastebin.com/raw/jzzG1KgG";
	const char *latestFilename = FREESHOP_DIR "/tmp/latest.txt";
	Download download(url, latestFilename);
	download.run();

	cpp3ds::FileInputStream latestFile;
	if (latestFile.open(latestFilename))
	{
		std::string tag;
		rapidjson::Document doc;
		int size = latestFile.getSize();
		tag.resize(size);
		latestFile.read(&tag[0], size);
		if (tag.empty())
			return false;

		Config::set(Config::LastUpdatedTime, static_cast<int>(time(nullptr)));
		Config::saveToFile();

		std::string tagWithoutPoints = ReplaceAll(tag, ".", "");
		std::cout << "Latest version (3DSX): " << tagWithoutPoints << std::endl;

		std::string actualVersionWithoutPoints = ReplaceAll(FREESHOP_VERSION, ".", "");
		std::cout << "Actual version (3DSX): " << actualVersionWithoutPoints << std::endl;

		if (std::stoi(tagWithoutPoints) > std::stoi(actualVersionWithoutPoints))
		{
			std::string freeShopFile = "sdmc:/3ds/freeShop/freeshop.3dsx";
			std::string freeShopUrl = _("https://notabug.org/evi/freeshop-versions/raw/master/%s.3dsx", tag.c_str());
			setStatus(_("Installing freeShop %s ...", tag.c_str()));
			Download freeShopDownload(freeShopUrl, freeShopFile);

			freeShopDownload.run();
			if (freeShopDownload.getLastResponse().getStatus() != cpp3ds::Http::Response::Ok)
				return false;

			return true;
		}
	}
	return false;
}

bool SyncState::loadThemeManagement()
{
	setStatus(_("Loading theme manager..."));
	std::string path;

	Theme::loadNameDesc();

	path = cpp3ds::FileSystem::getFilePath(FREESHOP_DIR "/theme/images/flags.png");
	if (pathExists(path.c_str(), false))
		Theme::isFlagsThemed = true;

	path = cpp3ds::FileSystem::getFilePath(FREESHOP_DIR "/theme/images/itembg.9.png");
	if (pathExists(path.c_str(), false))
		Theme::isItemBG9Themed = true;

	path = cpp3ds::FileSystem::getFilePath(FREESHOP_DIR "/theme/images/button-radius.9.png");
	if (pathExists(path.c_str(), false))
		Theme::isButtonRadius9Themed = true;

	path = cpp3ds::FileSystem::getFilePath(FREESHOP_DIR "/theme/images/fsbgsd.9.png");
	if (pathExists(path.c_str(), false))
		Theme::isFSBGSD9Themed = true;

	path = cpp3ds::FileSystem::getFilePath(FREESHOP_DIR "/theme/images/fsbgnand.9.png");
	if (pathExists(path.c_str(), false))
		Theme::isFSBGNAND9Themed = true;

	path = cpp3ds::FileSystem::getFilePath(FREESHOP_DIR "/theme/images/installed_item_bg.9.png");
	if (pathExists(path.c_str(), false))
		Theme::isInstalledItemBG9Themed = true;

	path = cpp3ds::FileSystem::getFilePath(FREESHOP_DIR "/theme/images/itembg-selected.9.png");
	if (pathExists(path.c_str(), false))
		Theme::isItemBGSelected9Themed = true;

	path = cpp3ds::FileSystem::getFilePath(FREESHOP_DIR "/theme/images/listitembg.9.png");
	if (pathExists(path.c_str(), false))
		Theme::isListItemBG9Themed = true;

	path = cpp3ds::FileSystem::getFilePath(FREESHOP_DIR "/theme/images/missing-icon.png");
	if (pathExists(path.c_str(), false))
		Theme::isMissingIconThemed = true;

	path = cpp3ds::FileSystem::getFilePath(FREESHOP_DIR "/theme/images/notification.9.png");
	if (pathExists(path.c_str(), false))
		Theme::isNotification9Themed = true;

	path = cpp3ds::FileSystem::getFilePath(FREESHOP_DIR "/theme/images/qr_selector.9.png");
	if (pathExists(path.c_str(), false))
		Theme::isQrSelector9Themed = true;

	path = cpp3ds::FileSystem::getFilePath(FREESHOP_DIR "/theme/images/scrollbar.9.png");
	if (pathExists(path.c_str(), false))
		Theme::isScrollbar9Themed = true;

	path = cpp3ds::FileSystem::getFilePath(FREESHOP_DIR "/theme/sounds/blip.ogg");
	if (pathExists(path.c_str(), false))
		Theme::isSoundBlipThemed = true;

	path = cpp3ds::FileSystem::getFilePath(FREESHOP_DIR "/theme/sounds/chime.ogg");
	if (pathExists(path.c_str(), false))
		Theme::isSoundChimeThemed = true;

	/*path = cpp3ds::FileSystem::getFilePath("darktheme/texts.json");
	if (pathExists(path.c_str(), false))
	printf("Hi lul\n");*/

	path = cpp3ds::FileSystem::getFilePath(FREESHOP_DIR "/theme/texts.json");
	if (pathExists(path.c_str(), false)) {
		if (Theme::loadFromFile()) {
			Theme::isTextThemed = true;


			//Load differents colors
			std::string primaryTextValue      = Theme::get("primaryText").GetString();
			std::string secondaryTextValue    = Theme::get("secondaryText").GetString();
			std::string iconSetValue          = Theme::get("iconSet").GetString();
			std::string iconSetActiveValue    = Theme::get("iconSetActive").GetString();
			std::string transitionScreenValue = Theme::get("transitionScreen").GetString();
			std::string boxColorValue         = Theme::get("boxColor").GetString();
			std::string boxOutlineColorValue  = Theme::get("boxOutlineColor").GetString();
			std::string dialogBackgroundValue = Theme::get("dialogBackground").GetString();
			std::string dialogButtonValue     = Theme::get("dialogButton").GetString();
			std::string dialogButtonTextValue = Theme::get("dialogButtonText").GetString();
			std::string themeDescColor				= Theme::get("themeDescColor").GetString();

			/*//Load built-in DarkTheme colors
			std::string darkPrimaryTextValue  		= Theme::get("darkPrimaryText").GetString();
			std::string darkSecondaryTextValue		= Theme::get("darkSecondaryText").GetString();
			std::string darkIconSetValue					= Theme::get("darkIconSet").GetString();
			std::string darkIconSetActiveValue		= Theme::get("darkIconSetActive").GetString();
			std::string darkTransitionScreenValue = Theme::get("darkTransitionScreen").GetString();
			std::string darkBoxColorValue					= Theme::get("darkBoxColor").GetString();
			std::string darkBoxOutlineColorValue  = Theme::get("darkBoxOutlineColor").GetString();
			std::string darkDialogBackgroundValue = Theme::get("darkDialogBackground").GetString();
			std::string darkDialogButtonValue 		= Theme::get("darkDialogButton").GetString();
			std::string darkDialogButtonTextValue = Theme::get("darkDialogButtonText").GetString();
			std::string darkThemeDescColor				= Theme::get("darkThemeDescColor").GetString();*/

			//Load the theme information
			std::string themeNameValue = Theme::get("themeName").GetString();
			std::string themeVerValue  = Theme::get("themeVer").GetString();
			std::string themeDescValue = Theme::get("themeDesc").GetString();

			/*//Load built-in DarkTheme information
			std::string darkThemeNameValue = Theme::get("darkThemeName").GetString();
			std::string darkThemeVerValue  = Theme::get("darkThemeVer").GetString();
			std::string darkThemeDescValue = Theme::get("darkThemeDesc").GetString();*/

			//Set the colors
			int R, G, B;

			hexToRGB(primaryTextValue, &R, &G, &B);
			Theme::primaryTextColor = cpp3ds::Color(R, G, B);

			hexToRGB(secondaryTextValue, &R, &G, &B);
			Theme::secondaryTextColor = cpp3ds::Color(R, G, B);

			hexToRGB(iconSetValue, &R, &G, &B);
			Theme::iconSetColor = cpp3ds::Color(R, G, B);

			hexToRGB(iconSetActiveValue, &R, &G, &B);
			Theme::iconSetColorActive = cpp3ds::Color(R, G, B);

			hexToRGB(transitionScreenValue, &R, &G, &B);
			Theme::transitionScreenColor = cpp3ds::Color(R, G, B);

			hexToRGB(boxColorValue, &R, &G, &B);
			Theme::boxColor = cpp3ds::Color(R, G, B);

			hexToRGB(boxOutlineColorValue, &R, &G, &B);
			Theme::boxOutlineColor = cpp3ds::Color(R, G, B);

			hexToRGB(dialogBackgroundValue, &R, &G, &B);
			Theme::dialogBackground = cpp3ds::Color(R, G, B);

			hexToRGB(dialogButtonValue, &R, &G, &B);
			Theme::dialogButton = cpp3ds::Color(R, G, B);

			hexToRGB(dialogButtonTextValue, &R, &G, &B);
			Theme::dialogButtonText = cpp3ds::Color(R, G, B);

			hexToRGB(themeDescColor, &R, &G, &B);
			Theme::themeDescColor = cpp3ds::Color(R, G, B);

			/*hexToRGB(darkPrimaryTextValue, &R, &G, &B);
			Theme::darkPrimaryTextColor = cpp3ds::Color(R, G, B);

			hexToRGB(darkSecondaryTextValue, &R, &G, &B);
			Theme::darkSecondaryTextColor = cpp3ds::Color(R, G, B);

			hexToRGB(darkIconSetValue, &R, &G, &B);
			Theme::darkIconSetColor = cpp3ds::Color(R, G, B);

			hexToRGB(darkIconSetActiveValue, &R, &G, &B);
			Theme::darkIconSetColorActive = cpp3ds::Color(R, G, B);

			hexToRGB(darkTransitionScreenValue, &R, &G, &B);
			Theme::darkTransitionScreenColor = cpp3ds::Color(R, G, B);

			hexToRGB(darkBoxColorValue, &R, &G, &B);
			Theme::darkBoxColor = cpp3ds::Color(R, G, B);

			hexToRGB(darkBoxOutlineColorValue, &R, &G, &B);
			Theme::darkBoxOutlineColor = cpp3ds::Color(R, G, B);

			hexToRGB(darkDialogBackgroundValue, &R, &G, &B);
			Theme::darkDialogBackground = cpp3ds::Color(R, G, B);

			hexToRGB(darkDialogButtonValue, &R, &G, &B);
			Theme::darkDialogButton = cpp3ds::Color(R, G, B);

			hexToRGB(darkDialogButtonTextValue, &R, &G, &B);
			Theme::darkDialogButtonText = cpp3ds::Color(R, G, B);

			hexToRGB(darkThemeDescColor, &R, &G, &B);
			Theme::darkThemeDescColor = cpp3ds::Color(R, G, B);*/

			//Set the theme informations
			Theme::themeName = themeNameValue;
			Theme::themeVersion = themeVerValue;
			Theme::themeDesc = themeDescValue;

			/*//Set the DarkTheme informations
			Theme::darkThemeName = darkThemeNameValue;
			Theme::darkThemeVersion = darkThemeVerValue;
			Theme::darkThemeDesc = darkThemeDescValue;*/
		}
	}
}

bool SyncState::loadServices()
{
	setStatus(_("Loading services..."));

	#ifndef EMULATION
		Result res;

		//NIMS service init for sleep download
		static u8 nimBuf[0x20000];
		if (R_FAILED(res = nimsInit(nimBuf, 0x20000)))
			Notification::spawn(_("Unable to start NIMS: \n0x%08lX (%d)", res, R_DESCRIPTION(res)));

		//PTMU service init for battery indicator
		if (R_FAILED(res = ptmuInit()))
			Notification::spawn(_("Unable to start PTMU: \n0x%08lX (%d)", res, R_DESCRIPTION(res)));

		//AC service init for signal indicator
		if (R_FAILED(res = acInit()))
			Notification::spawn(_("Unable to start AC: \n0x%08lX (%d)", res, R_DESCRIPTION(res)));

		//PTM:SYSM service init for shutdown
		if (R_FAILED(res = ptmSysmInit()))
			Notification::spawn(_("Unable to start PTMSYSM: \n0x%08lX (%d)", res, R_DESCRIPTION(res)));

		//NEWS service init for notifications
		if (R_FAILED(res = newsInit()))
			Notification::spawn(_("Unable to start NEWS: \n0x%08lX (%d)", res, R_DESCRIPTION(res)));

		//NS service to launch Sapphire
		if (R_FAILED(res = nsInit())) {
			Notification::spawn(_("Unable to start NS: \n0x%08lX (%d)", res, R_DESCRIPTION(res)));
		} else {
			u32 pid;
      Result ret = NS_LaunchTitle(0x000401300F13EE02ULL, 0, &pid);

			if (R_FAILED(ret) && ret != 0xC8804478 && ret != 0xD9001413)
				Notification::spawn(_("Unable to start Sapphire: \n0x%08lX (%d)", ret, R_DESCRIPTION(ret)));
		}

		std::cout << "isHb: " << envIsHomebrew() << std::endl;
	#endif
}


bool SyncState::updateCache()
{
	// Fetch cache if it doesn't exist, regardless of settings
	bool cacheExists = pathExists(FREESHOP_DIR "/cache/data.json");

	if (cacheExists && Config::get(Config::CacheVersion).GetStringLength() > 0)
		if (!Config::get(Config::AutoUpdate).GetBool() && !Config::get(Config::TriggerUpdateFlag).GetBool())
			return false;

	// In case this flag triggered the update, reset it
	Config::set(Config::TriggerUpdateFlag, false);

	setStatus(_("Checking latest cache..."));
	const char *url = "https://gitlab.com/altCache/altCache-v2/raw/master/cache_version.txt";
	const char *latestJsonFilename = FREESHOP_DIR "/tmp/latest.json";
	Download cache(url, latestJsonFilename);
	cache.run();

	cpp3ds::FileInputStream jsonFile;
	if (jsonFile.open(latestJsonFilename))
	{
		std::string json;
		rapidjson::Document doc;
		int size = jsonFile.getSize();
		json.resize(size);
		jsonFile.read(&json[0], size);
		doc.Parse(json.c_str());

		std::string tag = doc["tag_name"].GetString();
		if (!cacheExists || (!tag.empty() && tag.compare(Config::get(Config::CacheVersion).GetString()) != 0))
		{
			std::string cacheFile = FREESHOP_DIR "/tmp/cache.tar.gz";
#ifdef _3DS
			std::string cacheUrl = _("https://gitlab.com/altCache/altCache-v2/raw/master/%s/cache-%s-etc1.tar.gz", tag.c_str(), tag.c_str());
#else
			std::string cacheUrl = _("https://gitlab.com/altCache/altCache-v2/raw/master/%s/cache-%s-jpg.tar.gz", tag.c_str(), tag.c_str());
#endif
			setStatus(_("Downloading latest cache: %s...", tag.c_str()));
			Download cacheDownload(cacheUrl, cacheFile);
			cacheDownload.run();

			setStatus(_("Extracting latest cache..."));
			if (extract(cacheFile, FREESHOP_DIR "/cache/"))
			{
				Config::set(Config::CacheVersion, tag.c_str());
				Config::saveToFile();
				return true;
			}
		}
	}

	return false;
}


bool SyncState::updateTitleKeys()
{
	auto urls = Config::get(Config::KeyURLs).GetArray();
	if (!Config::get(Config::DownloadTitleKeys).GetBool() || urls.Empty())
		return false;

	for (int i = 0; i < urls.Size(); ++i) {
		if (urls.Size() > 1 && Config::get(Config::DownloadFromMultipleURLs).GetBool())
			setStatus(_("Downloading title keys...") + _(" %i/%i", i + 1, urls.Size()));
		else
			setStatus(_("Downloading title keys..."));

		const char *url = urls[i].GetString();
		std::string tmpFilename = cpp3ds::FileSystem::getFilePath(FREESHOP_DIR "/tmp/keys.bin");
		std::string keysFilename = cpp3ds::FileSystem::getFilePath(_(FREESHOP_DIR "/keys/download.%i.bin", i));
		Download download(url, tmpFilename);
		download.run();

		if (!TitleKeys::isValidFile(tmpFilename))
			return false;

		if (pathExists(keysFilename.c_str()))
			unlink(keysFilename.c_str());
		rename(tmpFilename.c_str(), keysFilename.c_str());

		if (!Config::get(Config::DownloadFromMultipleURLs).GetBool())
			break;
	}

	return true;
}


bool SyncState::updateEshopMusic()
{
#ifdef EMULATION
	return true;
#else
	u32 id;
	u8 consoleRegion;
	CFGU_SecureInfoGetRegion(&consoleRegion);

	switch (consoleRegion)
	{
		case CFG_REGION_USA: id = 0x219; break;
		case CFG_REGION_JPN: id = 0x209; break;
		case CFG_REGION_AUS: // ?
		case CFG_REGION_EUR: id = 0x229; break;
		case CFG_REGION_KOR: id = 0x279; break;
		case CFG_REGION_CHN: // TODO: Figure out eShop id
		case CFG_REGION_TWN: id = 0x289; break;
		default:
			return false;
	}

	Handle handle;
	Result res;
	std::vector<char> fileData;
	u64 fileSize;

	u32 archivePathData[3] = {MEDIATYPE_SD, id, 0};
	FS_Path archivePath = {PATH_BINARY, sizeof(archivePathData), archivePathData};

	int bgmCount = 2;
	for (int i = 1; i <= bgmCount; ++i)
	{
		std::string fileName = fmt::sprintf("/boss_bgm%d", i);
		std::string destPath = FREESHOP_DIR "/music/eshop" + fileName;
		FS_Path filePath = {PATH_ASCII, fileName.size()+1, fileName.c_str()};

		if (pathExists(destPath.c_str()) && !Config::get(Config::ResetEshopMusic).GetBool())
			continue;
		std::cout << "getting shit!" << std::endl;
		if (R_SUCCEEDED(res = FSUSER_OpenFileDirectly(&handle, ARCHIVE_EXTDATA, archivePath, filePath, FS_OPEN_READ, FS_ATTRIBUTE_READ_ONLY)))
		{
			if (R_SUCCEEDED(res = FSFILE_GetSize(handle, &fileSize)))
			{
				fileData.resize(fileSize);
				if (R_SUCCEEDED(res = FSFILE_Read(handle, nullptr, 0, fileData.data(), fileSize)))
				{
					std::ofstream file(destPath);
					if (file.is_open())
						file.write(fileData.data(), fileData.size());
				}
			}
			FSFILE_Close(handle);
		}
	}

	Config::set(Config::ResetEshopMusic, false);
	return true;
#endif
}


void SyncState::setStatus(const std::string &message)
{
	LoadInformations::getInstance().setStatus(message);
}

void SyncState::startupSound()
{
	cpp3ds::Clock clock;

#ifndef EMULATION
	Result res;

	//AM service init for app lists and check Sapphire
	if (R_FAILED(res = amInit()))
		Notification::spawn(_("Unable to start AM: \n0x%08lX (%d)", res, R_DESCRIPTION(res)));

	u32 nandTitleCount;
	AM_GetTitleCount(MEDIATYPE_NAND, &nandTitleCount);

	std::vector<cpp3ds::Uint64> titleIds;
	titleIds.resize(nandTitleCount);
	AM_GetTitleList(nullptr, MEDIATYPE_NAND, nandTitleCount, &titleIds[0]);

	bool isSapphirePresent = false;
	if (std::find(titleIds.begin(), titleIds.end(), 0x000401300F13EE02ULL) != titleIds.end())
		isSapphirePresent = true;
#endif

	while (clock.getElapsedTime() < cpp3ds::seconds(3.5f))
		cpp3ds::sleep(cpp3ds::milliseconds(50));

	m_soundStartup.play();

#ifndef EMULATION
	if (Config::get(Config::LEDStartup).GetBool() && !isSapphirePresent) {
		cpp3ds::sleep(cpp3ds::milliseconds(100));

		MCU::getInstance().ledBlinkOnce(0x19A4FF);
	}
#endif
//	while (clock.getElapsedTime() < cpp3ds::seconds(7.f))
//		cpp3ds::sleep(cpp3ds::milliseconds(50));
//	m_soundLoading.play(0);
}

} // namespace FreeShop
