#include "BrowseState.hpp"
#include "SyncState.hpp"
#include "../Notification.hpp"
#include "../AssetManager.hpp"
#include "../Util.hpp"
#include "../Installer.hpp"
#include "SleepState.hpp"
#include "../InstalledList.hpp"
#include "../Config.hpp"
#include "../TitleKeys.hpp"
#include "../FreeShop.hpp"
#include "../Theme.hpp"
#include "../LoadInformations.hpp"
#ifndef EMULATION
#include "../KeyboardApplet.hpp"
#endif
#include <TweenEngine/Tween.h>
#include <cpp3ds/Window/Window.hpp>
#include <sstream>
#include <cpp3ds/System/I18n.hpp>
#include <cpp3ds/System/FileSystem.hpp>
#include <time.h>
#include <cpp3ds/Window/Window.hpp>
#include <cpp3ds/System/FileSystem.hpp>
#include <cpp3ds/System/Sleep.hpp>
#include <cpp3ds/Audio/Music.hpp>

namespace FreeShop {

BrowseState *g_browseState = nullptr;
bool g_isLatestFirmwareVersion = true;
cpp3ds::Clock BrowseState::clockDownloadInactivity;

BrowseState::BrowseState(StateStack& stack, Context& context, StateCallback callback)
: State(stack, context, callback)
, m_appListPositionX(0.f)
, m_threadInitialize(&BrowseState::initialize, this)
, m_threadLoadApp(&BrowseState::loadApp, this)
, m_threadBusy(false)
, m_activeDownloadCount(0)
, m_mode(Info)
, m_gwenRenderer(nullptr)
, m_gwenSkin(nullptr)
, m_settingsGUI(nullptr)
, m_isTransitioning(false)
, m_isJapKeyboard(false)
, m_isTIDKeyboard(false)
, m_musicMode(0)
, m_isSliderOff(false)
, m_counter(1)
, m_isControlsBlocked(false)
{
	g_browseState = this;
	m_musicLoop.setLoop(true);

#ifdef EMULATION
	g_syncComplete = true;
	initialize();
#else
	m_threadInitialize.setRelativePriority(1);
	m_threadInitialize.launch();
#endif
}

BrowseState::~BrowseState()
{
	settingsSaveToConfig();

	Config::saveToFile();

	if (m_settingsGUI)
		delete m_settingsGUI;
	if (m_gwenSkin)
		delete m_gwenSkin;
	if (m_gwenRenderer)
		delete m_gwenRenderer;
}

void BrowseState::initialize()
{
	// Initialize AppList singleton first
	LoadInformations::getInstance().setStatus(_("Loading game list..."));
	AppList::getInstance().refresh();

	LoadInformations::getInstance().setStatus(_("Loading installed game list..."));
	LoadInformations::getInstance().updateLoadingPercentage(-1);
	InstalledList::getInstance().refresh();

	LoadInformations::getInstance().setStatus(_("Welcome, %s", getUsername().toAnsiString().c_str()));

	//Var init
	m_ctrSdPath = "";
	m_keyHistory = {};
	m_musicFileName = "";

	m_iconSet.addIcon(L"\uf0ae");
	m_iconSet.addIcon(L"\uf290");
	m_iconSet.addIcon(L"\uf019");
	m_iconSet.addIcon(L"\uf11b");
	m_iconSet.addIcon(L"\uf013");
	m_iconSet.addIcon(L"\uf002");
	m_iconSet.setPosition(60.f, 13.f);
	m_iconSet.setSelectedIndex(m_mode);

	m_textActiveDownloads.setCharacterSize(8);
	m_textActiveDownloads.setFillColor(cpp3ds::Color::Black);
	m_textActiveDownloads.setOutlineColor(cpp3ds::Color::White);
	m_textActiveDownloads.setOutlineThickness(1.f);
	m_textActiveDownloads.setPosition(128.f, 3.f);

	m_textInstalledCount = m_textActiveDownloads;
	m_textInstalledCount.setPosition(162.f, 3.f);


	//If there's no title key available, or no cache, one of these messages will appear
	m_textListEmpty.setFillColor(cpp3ds::Color::Red);
	if (TitleKeys::getIds().empty())
		m_textListEmpty.setString(_("No title keys found.\nMake sure you have keys in\n%s\n\nManually copy keys to the directory\nor check settings to enter a URL\nfor downloading title keys.", FREESHOP_DIR "/keys/"));
	else
		m_textListEmpty.setString(_("No cache entries found\nfor your title keys.\n\nTry refreshing cache in settings.\nIf that doesn't work, then your\ntitles simply won't work with\nfreeShop currently."));
	m_textListEmpty.useSystemFont();
	m_textListEmpty.setCharacterSize(16);
	m_textListEmpty.setFillColor(cpp3ds::Color(80, 80, 80, 255));
	m_textListEmpty.setPosition(200.f, 140.f);
	m_textListEmpty.setOrigin(m_textListEmpty.getLocalBounds().width / 2, m_textListEmpty.getLocalBounds().height / 2);

	//Load keyboard file
	reloadKeyboard();

	m_textMatches.resize(4);
	for (auto& text : m_textMatches)
	{
		text.setCharacterSize(13);
		text.useSystemFont();
	}

	m_AP_Sprite.setTexture(m_AP_Texture, true);
	m_AP_Sprite.setPosition(200.f, 120.f);
	m_AP_Sprite.setOrigin(m_AP_Sprite.getGlobalBounds().width / 2, m_AP_Sprite.getLocalBounds().height / 2);

	m_scrollbarInstalledList.setPosition(314.f, 50.f);
	m_scrollbarInstalledList.setDragRect(cpp3ds::IntRect(0, 50, 320, 190));
	m_scrollbarInstalledList.setScrollAreaSize(cpp3ds::Vector2u(320, 210));
	m_scrollbarInstalledList.setSize(cpp3ds::Vector2u(8, 190));
	m_scrollbarInstalledList.setColor(cpp3ds::Color(150, 150, 150, 150));

	m_scrollbarDownloadQueue = m_scrollbarInstalledList;
	m_scrollbarDownloadQueue.setDragRect(cpp3ds::IntRect(0, 30, 320, 210));
	m_scrollbarDownloadQueue.setScrollAreaSize(cpp3ds::Vector2u(320, 210));
	m_scrollbarDownloadQueue.setSize(cpp3ds::Vector2u(8, 210));

	m_scrollbarInstalledList.attachObject(&InstalledList::getInstance());
	m_scrollbarDownloadQueue.attachObject(&DownloadQueue::getInstance());

	m_textSearchInstalledList.setString(_("Search..."));
	m_textSearchInstalledList.setPosition(160.f, 32.f);
	if (Theme::isTextThemed)
		m_textSearchInstalledList.setFillColor(Theme::primaryTextColor);
	else if (Config::get(Config::DarkTheme).GetBool())
		m_textSearchInstalledList.setFillColor(cpp3ds::Color(248, 248, 248));
	else
		m_textSearchInstalledList.setFillColor(cpp3ds::Color::Black);
	m_textSearchInstalledList.setCharacterSize(12.f);
	m_textSearchInstalledList.setStyle(cpp3ds::Text::Italic);
	m_textSearchInstalledList.setOrigin(m_textSearchInstalledList.getGlobalBounds().width / 2, 0);

	if (Theme::isTextThemed)
		m_textBoxInstalledList.setOutlineColor(Theme::boxOutlineColor);
	else if (Config::get(Config::DarkTheme).GetBool())
		m_textBoxInstalledList.setOutlineColor(cpp3ds::Color(165, 44, 44));
	else
		m_textBoxInstalledList.setOutlineColor(cpp3ds::Color(158, 158, 158, 255));
	m_textBoxInstalledList.setOutlineThickness(1);
	if (Theme::isTextThemed)
		m_textBoxInstalledList.setFillColor(Theme::boxColor);
	else if (Config::get(Config::DarkTheme).GetBool())
		m_textBoxInstalledList.setFillColor(cpp3ds::Color(201, 28, 28));
	else
		m_textBoxInstalledList.setFillColor(cpp3ds::Color(245, 245, 245));
	m_textBoxInstalledList.setSize(cpp3ds::Vector2f(320.f, 16.f));
	m_textBoxInstalledList.setPosition(0.f, 32.f);

	setMode(Info);

	m_bottomView.setCenter(cpp3ds::Vector2f(160.f, 120.f));
	m_bottomView.setSize(cpp3ds::Vector2f(320.f, 240.f));

#ifdef _3DS
	while (!m_gwenRenderer)
		cpp3ds::sleep(cpp3ds::milliseconds(10));
	m_gwenSkin = new Gwen::Skin::TexturedBase(m_gwenRenderer);

	if (pathExists(cpp3ds::FileSystem::getFilePath(FREESHOP_DIR "/theme/images/DefaultSkin.png").c_str(), true))
		m_gwenSkin->Init(cpp3ds::FileSystem::getFilePath(FREESHOP_DIR "/theme/images/DefaultSkin.png"));
	else if (Config::get(Config::DarkTheme).GetBool())
		m_gwenSkin->Init("darkimages/DefaultSkin.png");
	else
		m_gwenSkin->Init("images/DefaultSkin.png");

	m_gwenSkin->SetDefaultFont(L"", 11);

	//Check if the system firmware is the latest for sleep download
	NIMS_WantUpdate(&g_isLatestFirmwareVersion);
	g_isLatestFirmwareVersion = !g_isLatestFirmwareVersion;

	// Need to wait until title screen is done to prevent music from
	// settings starting prematurely.
	while(!g_syncComplete)
		cpp3ds::sleep(cpp3ds::milliseconds(10));

	//White screen used for transitions
	m_whiteScreen.setPosition(0.f, 30.f);
	m_whiteScreen.setSize(cpp3ds::Vector2f(320.f, 210.f));
	if (Theme::isTextThemed)
		m_whiteScreen.setFillColor(Theme::transitionScreenColor);
	else if (Config::get(Config::DarkTheme).GetBool())
		m_whiteScreen.setFillColor(cpp3ds::Color(98, 98, 98));
	else
		m_whiteScreen.setFillColor(cpp3ds::Color::White);
	m_whiteScreen.setFillColor(cpp3ds::Color(m_whiteScreen.getFillColor().r, m_whiteScreen.getFillColor().g, m_whiteScreen.getFillColor().b, 0));

	m_settingsGUI = new GUI::Settings(m_gwenSkin, this);
#endif

#ifndef EMULATION
	//Get the /Nintendo 3DS/<id0>/<id1> path
	fsInit();
	u8 * outdata = static_cast<u8 *>(malloc(1024));
	FSUSER_GetSdmcCtrRootPath(outdata, 1024);
	char* charOut;
	std::string ctrPath;

	for (size_t i = 0; i < 158; ++i) {
  		if (i % 2 == 0) {
			charOut = (char*)outdata + i;
			ctrPath += charOut;
		}
	}

	fsExit();

	m_ctrSdPath = ctrPath;
	free(outdata);
#endif

	g_browserLoaded = true;

	m_topInfos.resetModeTimer();
	m_topInfos.setModeChangeEnabled(true);

	SleepState::clock.restart();
	clockDownloadInactivity.restart();
	requestStackClearUnder();
}

void BrowseState::renderTopScreen(cpp3ds::Window& window)
{
	if (!g_syncComplete || !g_browserLoaded)
		return;

	if (AppList::getInstance().getList().size() == 0) {
		window.draw(m_textListEmpty);
	} else {
		window.draw(AppList::getInstance());
	}

	//if (Config::get(Config::DownloadTitleKeys).GetBool()) {
		window.draw(m_AP_Sprite);
	//}

	// Special draw method to draw top screenshot if selected
	m_appInfo.drawTop(window);
}

void BrowseState::renderBottomScreen(cpp3ds::Window& window)
{
	if (!m_gwenRenderer)
	{
		m_gwenRenderer = new Gwen::Renderer::cpp3dsRenderer(window);

#ifdef EMULATION
		m_gwenSkin = new Gwen::Skin::TexturedBase(m_gwenRenderer);

		std::cout << cpp3ds::FileSystem::getFilePath(FREESHOP_DIR "/theme/images/DefaultSkin.png").c_str() << std::endl;

		if (pathExists(cpp3ds::FileSystem::getFilePath(FREESHOP_DIR "/theme/images/DefaultSkin.png").c_str(), true))
			m_gwenSkin->Init(cpp3ds::FileSystem::getFilePath(FREESHOP_DIR "/theme/images/DefaultSkin.png"));
		else
			m_gwenSkin->Init("images/DefaultSkin.png");

		m_gwenSkin->SetDefaultFont(L"", 11);
		m_settingsGUI = new GUI::Settings(m_gwenSkin, this);
#endif
	}
	if (!g_syncComplete || !g_browserLoaded)
		return;

	window.draw(m_topInfos);
	window.draw(m_iconSet);

	if (m_activeDownloadCount > 0)
		window.draw(m_textActiveDownloads);

	if (InstalledList::getInstance().getGameCount() > 0 && Config::get(Config::ShowGameCounter).GetBool())
		window.draw(m_textInstalledCount);

	window.setView(m_bottomView);

	if (m_mode == App)
		window.draw(m_appInfo);
	if (m_mode == Downloads)
	{
		window.draw(DownloadQueue::getInstance());
		window.draw(m_scrollbarDownloadQueue);
	}
	if (m_mode == Installed)
	{
		window.draw(InstalledList::getInstance());
		window.draw(m_scrollbarInstalledList);

		window.draw(m_textBoxInstalledList);
		window.draw(m_textSearchInstalledList);
	}
	if (m_mode == Search)
	{
		window.draw(m_keyboard);
		for (auto& textMatch : m_textMatches)
			window.draw(textMatch);
	}
	if (m_mode == Info)
		window.draw(m_botInfos);
	if (m_mode == Settings)
	{
		m_settingsGUI->render();
	}

	window.setView(window.getDefaultView());

	if (m_isTransitioning)
		window.draw(m_whiteScreen);
}

bool BrowseState::update(float delta)
{
	if (!g_syncComplete || !g_browserLoaded)
		return true;

	if (m_threadBusy)
	{
		clockDownloadInactivity.restart();
		SleepState::clock.restart();
	}

	// Show latest news if requested
	if (Config::get(Config::ShowNews).GetBool())
	{
		Config::set(Config::ShowNews, false);
		requestStackPush(States::News);
	}

	// Go into sleep state after inactivity
	if (!SleepState::isSleeping && (Config::get(Config::SleepMode).GetBool() || Config::get(Config::SleepModeBottom).GetBool() || Config::get(Config::DimLEDs).GetBool()) && SleepState::clock.getElapsedTime() > cpp3ds::seconds(Config::get(Config::InactivitySeconds).GetFloat()))
	{
		if (!Config::get(Config::MusicOnInactivity).GetBool())
			stopBGM();

		requestStackPush(States::Sleep);
		return false;
	}

	// Power off after sufficient download inactivity
	if (m_activeDownloadCount == 0
		&& Config::get(Config::PowerOffAfterDownload).GetBool()
		&& clockDownloadInactivity.getElapsedTime() > cpp3ds::seconds(Config::get(Config::PowerOffTime).GetInt()))
	{
		g_requestShutdown = true;
		return false;
	}

	// If selected icon changed, change mode accordingly
	// If the selected mode is Search and the "Use system keyboard" option is enabled, show the System keyboard
	int iconIndex = m_iconSet.getSelectedIndex();
	if (iconIndex == Search && Config::get(Config::SystemKeyboard).GetBool()) {
		m_iconSet.setSelectedIndex(m_mode);
#ifndef EMULATION
		//Check if the keyboard mode is Title ID
		if (m_isTIDKeyboard) {
			KeyboardApplet kb(KeyboardApplet::TitleID);
			swkbdSetHintText(kb, _("Type a game Title ID...").toAnsiString().c_str());
			cpp3ds::String input = kb.getInput();
			if (!input.isEmpty())
				AppList::getInstance().filterBySearch(input.toAnsiString(), m_textMatches);
		} else {
			KeyboardApplet kb(KeyboardApplet::Text);
			swkbdSetHintText(kb, _("Type a game name...").toAnsiString().c_str());

			cpp3ds::String input = kb.getInput();
			if (!input.isEmpty())
				AppList::getInstance().filterBySearch(input.toAnsiString(), m_textMatches);
		}
#else
		std::cout << "System keyboard." << std::endl;
#endif
	}
	else if (m_mode != iconIndex && iconIndex >= 0)
		setMode(static_cast<Mode>(iconIndex));

	// Update the active mode
	if (m_mode == App)
	{
		m_appInfo.update(delta);
	}
	else if (m_mode == Downloads)
	{
		DownloadQueue::getInstance().update(delta);
		m_scrollbarDownloadQueue.update(delta);
	}
	else if (m_mode == Installed)
	{
		InstalledList::getInstance().update(delta);
		m_scrollbarInstalledList.update(delta);
	}
	else if (m_mode == Search)
	{
		m_keyboard.update(delta);
	}
	else if (m_mode == Info)
	{
		m_botInfos.update(delta);
	}

	if (m_activeDownloadCount != DownloadQueue::getInstance().getActiveCount())
	{
		clockDownloadInactivity.restart();
		m_activeDownloadCount = DownloadQueue::getInstance().getActiveCount();
		m_textActiveDownloads.setString(_("%u", m_activeDownloadCount));
	}

	m_textInstalledCount.setString(_("%u", InstalledList::getInstance().getGameCount()));

	m_iconSet.update(delta);
	m_topInfos.update(delta);
	AppList::getInstance().update(delta);
	m_tweenManager.update(delta);

	return true;
}

bool BrowseState::processEvent(const cpp3ds::Event& event)
{
	if (SleepState::isSleeping)
	{
		if (!Config::get(Config::MusicOnInactivity).GetBool())
			m_settingsGUI->playMusic();

		return true;
	}


	SleepState::clock.restart();
	clockDownloadInactivity.restart();

	if (m_threadBusy || !g_syncComplete || !g_browserLoaded || m_isControlsBlocked)
		return false;

	if (m_mode == App) {
		if (!m_appInfo.processEvent(event))
			return false;
	}
	else if (m_mode == Downloads) {
		if (!m_scrollbarDownloadQueue.processEvent(event))
			DownloadQueue::getInstance().processEvent(event);
	} else if (m_mode == Installed) {
		if (!m_scrollbarInstalledList.processEvent(event))
			InstalledList::getInstance().processEvent(event);
	} else if (m_mode == Settings) {
		m_settingsGUI->processEvent(event);
	}

	m_iconSet.processEvent(event);

	if (m_mode == Search)
	{
		if (!m_keyboard.processEvents(event))
			return true;

		cpp3ds::String currentInput;
		if (m_keyboard.popString(currentInput))
		{
			// Enter was pressed, so close keyboard
			m_iconSet.setSelectedIndex(App);
			m_lastKeyboardInput.clear();
		}
		else
		{
			currentInput = m_keyboard.getCurrentInput();
			if (m_lastKeyboardInput != currentInput)
			{
				m_lastKeyboardInput = currentInput;
				AppList::getInstance().filterBySearch(currentInput, m_textMatches);
				TweenEngine::Tween::to(AppList::getInstance(), AppList::POSITION_XY, 0.3f)
					.target(0.f, 0.f)
					.start(m_tweenManager);
			}
		}
	}
	else
	{
		// Events for all modes except Search
		AppList::getInstance().processEvent(event);
	}

	if (event.type == cpp3ds::Event::KeyPressed)
	{
		int index = AppList::getInstance().getSelectedIndex();

		//Dat secret block of code
		m_keyHistory.push_back(event.key.code);
		if (m_keyHistory.size() > 10) {
			m_keyHistory.erase(m_keyHistory.begin());
		}
		if (m_keyHistory.size() >= 10) {
			if (m_keyHistory[0] == cpp3ds::Keyboard::DPadUp && m_keyHistory[1] == cpp3ds::Keyboard::DPadUp && m_keyHistory[2] == cpp3ds::Keyboard::DPadDown && m_keyHistory[3] == cpp3ds::Keyboard::DPadDown && m_keyHistory[4] == cpp3ds::Keyboard::DPadLeft && m_keyHistory[5] == cpp3ds::Keyboard::DPadRight && m_keyHistory[6] == cpp3ds::Keyboard::DPadLeft && m_keyHistory[7] == cpp3ds::Keyboard::DPadRight && m_keyHistory[8] == cpp3ds::Keyboard::B && m_keyHistory[9] == cpp3ds::Keyboard::A) {
				switch (m_counter) {
					case 1: Notification::spawn(_("There are no Easter Eggs in this program.")); break;
					case 2: Notification::spawn(_("There really are no Easter Eggs in this program.")); break;
					case 3: Notification::spawn(_("Didn't I already tell you that\nthere are no Easter Eggs in this program?")); break;
					case 4: Notification::spawn(_("All right, you win.\n\n                                    /----\\\n                           -------/      \\\n                          /                 \\\n                         /                   |\n   -----------------/                     --------\\\n   ----------------------------------------------")); break;
					default: Notification::spawn(_("What is it? It's an elephant being eaten by a snake, of course.")); break;
				}

				m_counter++;
			}
		}

		switch (event.key.code)
		{
			case cpp3ds::Keyboard::Start:
#ifndef EMULATION
				// If the user launched the freeShop with the 3dsx version, don't allow it to close via Start to prevent crash
				if (!envIsHomebrew()) {
					FreeShop::prepareToCloseApp();
					if (Config::get(Config::RestartFix).GetBool())
					{
						printf("Rebooting...\n");
						g_requestReboot = true;
					} else {
						requestStackClear();
						return true;
					}
				}
#endif
			case cpp3ds::Keyboard::A:
			{
				// Don't load if game info is already loaded or nothing is selected
				if (!AppList::getInstance().getSelected())
					break;
				if (m_appInfo.getAppItem() == AppList::getInstance().getSelected()->getAppItem())
					break;

				m_threadBusy = true;
				// Only fade out if a game is loaded already
				if (!m_appInfo.getAppItem())
					m_threadLoadApp.launch();
				else
					TweenEngine::Tween::to(m_appInfo, AppInfo::ALPHA, 0.3f)
						.target(0.f)
						.setCallback(TweenEngine::TweenCallback::COMPLETE, [this](TweenEngine::BaseTween* source) {
							m_threadLoadApp.launch();
						})
						.start(m_tweenManager);
				break;
			}
			case cpp3ds::Keyboard::B:
				AppList::getInstance().filterBySearch("", m_textMatches);
				break;
			case cpp3ds::Keyboard::X: {
				if (!AppList::getInstance().getSelected())
					break;
				auto app = AppList::getInstance().getSelected()->getAppItem();
				if (app && !DownloadQueue::getInstance().isDownloading(app))
				{
					if (!app->isInstalled())
					{
						app->queueForInstall();
						Notification::spawn(_("Queued install: %s", app->getTitle().toAnsiString().c_str()));
					}
					else
						Notification::spawn(_("Already installed: %s", app->getTitle().toAnsiString().c_str()));
				} else {
					Notification::spawn(_("Already in downloading: \n%s", app->getTitle().toAnsiString().c_str()));
				}
				break;
			}
			case cpp3ds::Keyboard::Y: {
				if (!AppList::getInstance().getSelected())
					break;
				auto app = AppList::getInstance().getSelected()->getAppItem();
				bool isSleepDownloading = false;
#ifndef EMULATION
				NIMS_IsTaskRegistered(app->getTitleId(), &isSleepDownloading);
#endif
				if (app && !DownloadQueue::getInstance().isDownloading(app) && !isSleepDownloading)
				{
					if (!app->isInstalled())
					{
						app->queueForSleepInstall();
					}
					else
						Notification::spawn(_("Already installed: %s", app->getTitle().toAnsiString().c_str()));
				} else if (isSleepDownloading) {
					app->removeSleepInstall();
				} else {
					Notification::spawn(_("Already in downloading: \n%s", app->getTitle().toAnsiString().c_str()));
				}
				break;
			}
			case cpp3ds::Keyboard::CStickRight: {
				if (getMode() < 5) {
					int newMode = getMode() + 1;
					setMode(static_cast<Mode>(newMode));
					m_iconSet.setSelectedIndex(newMode);
				}
				break;
			}
			case cpp3ds::Keyboard::CStickLeft: {
				if (getMode() > 0) {
					int newMode = getMode() - 1;
					setMode(static_cast<Mode>(newMode));
					m_iconSet.setSelectedIndex(newMode);
				}
				break;
			}
			default:
				break;
		}
	} else if (event.type == cpp3ds::Event::SliderVolumeChanged) {
		if (event.slider.value < 0.1 && !m_isSliderOff && Config::get(Config::MusicTurnOffSlider).GetBool()) {
			// The volume slider is under 10%, stop the music
			m_isSliderOff = true;
			stopBGM();
		} else if (event.slider.value >= 0.1 && m_isSliderOff && Config::get(Config::MusicTurnOffSlider).GetBool()) {
			// The volume slider is above or equals 10%, re-launch the music
			m_isSliderOff = false;
			m_settingsGUI->playMusic();
		}
	}

	return true;
}


void BrowseState::loadApp()
{
	auto item = AppList::getInstance().getSelected()->getAppItem();
	if (!item)
		return;

	// TODO: Don't show loading when game is cached?
	bool showLoading = g_browserLoaded && m_appInfo.getAppItem() != item;

	m_iconSet.setSelectedIndex(App);
	if (m_appInfo.getAppItem() != item)
	{
		setMode(App);

		if (showLoading)
			requestStackPush(States::Loading);

		m_appInfo.loadApp(item);
	}

	TweenEngine::Tween::to(m_appInfo, AppInfo::ALPHA, 0.5f)
		.target(255.f)
		.start(m_tweenManager);

	if (showLoading)
		requestStackPop();

	m_threadBusy = false;
}


void BrowseState::setMode(BrowseState::Mode mode)
{
	if (m_mode == mode || m_isTransitioning)
		return;

	// Transition / end current mode
	if (m_mode == Search)
	{
		float delay = 0.f;
		for (auto& text : m_textMatches)
		{
			TweenEngine::Tween::to(text, util3ds::RichText::POSITION_X, 0.2f)
				.target(-text.getLocalBounds().width)
				.delay(delay)
				.start(m_tweenManager);
			delay += 0.05f;
		}

		AppList::getInstance().setCollapsed(false);
		m_topInfos.setCollapsed(false);

		TweenEngine::Tween::to(m_iconSet, IconSet::POSITION_X, 0.3f)
					.target(60.f)
					.start(m_tweenManager);

		TweenEngine::Tween::to(m_textActiveDownloads, util3ds::TweenText::POSITION_X, 0.3f)
					.target(128.f)
					.start(m_tweenManager);

		TweenEngine::Tween::to(m_textInstalledCount, util3ds::TweenText::POSITION_X, 0.3f)
					.target(162.f)
					.start(m_tweenManager);
	}
	else if (m_mode == Settings)
	{
		m_settingsGUI->saveToConfig();
	}

	// Transition / start new mode
	if (mode == Search)
	{
		float posY = 1.f;
		for (auto& text : m_textMatches)
		{
			text.clear();
			text.setPosition(5.f, posY);
			posY += 13.f;
		}
		AppList::getInstance().setCollapsed(true);
		AppList::getInstance().setIndexDelta(0);
		m_topInfos.setCollapsed(true);

		TweenEngine::Tween::to(m_iconSet, IconSet::POSITION_X, 0.3f)
					.target(155.f)
					.start(m_tweenManager);

		TweenEngine::Tween::to(m_textActiveDownloads, util3ds::TweenText::POSITION_X, 0.3f)
					.target(223.f)
					.start(m_tweenManager);

		TweenEngine::Tween::to(m_textInstalledCount, util3ds::TweenText::POSITION_X, 0.3f)
					.target(257.f)
					.start(m_tweenManager);

		m_lastKeyboardInput = "";
		m_keyboard.setCurrentInput(m_lastKeyboardInput);
	}

	TweenEngine::Tween::to(m_whiteScreen, m_whiteScreen.FILL_COLOR_ALPHA, 0.4f)
		.target(255.f)
		.setCallback(TweenEngine::TweenCallback::COMPLETE, [=](TweenEngine::BaseTween* source) {
			m_mode = mode;
		})
		.start(m_tweenManager);
	TweenEngine::Tween::to(m_whiteScreen, m_whiteScreen.FILL_COLOR_ALPHA, 0.4f)
		.target(0.f)
		.setCallback(TweenEngine::TweenCallback::COMPLETE, [=](TweenEngine::BaseTween* source) {
			m_isTransitioning = false;
		})
		.delay(0.4f)
		.start(m_tweenManager);

	if (mode > m_mode) {
		TweenEngine::Tween::to(m_bottomView, m_bottomView.CENTER_XY, 0.4f)
			.target(180.f, 120.f)
			.setCallback(TweenEngine::TweenCallback::COMPLETE, [=](TweenEngine::BaseTween* source) {
				m_bottomView.setCenter(cpp3ds::Vector2f(140.f, 120.f));
			})
			.start(m_tweenManager);

		if (mode == Settings || m_mode == Settings) {
			TweenEngine::Tween::to(*m_settingsGUI, m_settingsGUI->POSITION_XY, 0.4f)
				.target(-20.f, 0.f)
				.setCallback(TweenEngine::TweenCallback::COMPLETE, [=](TweenEngine::BaseTween* source) {
					m_settingsGUI->setPosition(cpp3ds::Vector2f(20.f, 0.f));
				})
				.start(m_tweenManager);

			TweenEngine::Tween::to(*m_settingsGUI, m_settingsGUI->POSITION_XY, 0.4f)
				.target(0.f, 0.f)
				.delay(0.4f)
				.start(m_tweenManager);
		}
	} else {
		TweenEngine::Tween::to(m_bottomView, m_bottomView.CENTER_XY, 0.4f)
			.target(140.f, 120.f)
			.setCallback(TweenEngine::TweenCallback::COMPLETE, [=](TweenEngine::BaseTween* source) {
				m_bottomView.setCenter(cpp3ds::Vector2f(180.f, 120.f));
			})
			.start(m_tweenManager);

		if (mode == Settings || m_mode == Settings) {
			TweenEngine::Tween::to(*m_settingsGUI, m_settingsGUI->POSITION_XY, 0.4f)
				.target(20.f, 0.f)
				.setCallback(TweenEngine::TweenCallback::COMPLETE, [=](TweenEngine::BaseTween* source) {
					m_settingsGUI->setPosition(cpp3ds::Vector2f(-20.f, 0.f));
				})
				.start(m_tweenManager);

			TweenEngine::Tween::to(*m_settingsGUI, m_settingsGUI->POSITION_XY, 0.4f)
				.target(0.f, 0.f)
				.delay(0.4f)
				.start(m_tweenManager);
		}
	}

	TweenEngine::Tween::to(m_bottomView, m_bottomView.CENTER_XY, 0.4f)
		.target(160.f, 120.f)
		.delay(0.4f)
		.start(m_tweenManager);

	if (mode == Settings || m_mode == Settings) {
		TweenEngine::Tween::to(*m_settingsGUI, m_settingsGUI->POSITION_XY, 0.4f)
			.target(0.f, 0.f)
			.delay(0.4f)
			.start(m_tweenManager);
	}

	m_isTransitioning = true;
}

bool BrowseState::playBGMeShop()
{
	stopBGM();

	int bgmCount = 2;
	int randIndex = (std::rand() % bgmCount) + 1;
	int m_musicMode = 0;

	// In case it doesn't find it, loop down until it hopefully does
	for (int i = randIndex; i > 0; --i)
	{
		std::string filePath = fmt::sprintf(FREESHOP_DIR "/music/eshop/boss_bgm%d", i);
		if (m_musicLoop.openFromFile(filePath))
		{
			m_musicLoop.play();
			break;
		}
	}
}

bool BrowseState::playBGM(const std::string &filename)
{
	stopBGM();

	/*if (m_musicLoopBCSTM.openFromFile(filename)) {
		m_musicLoopBCSTM.play();
		m_musicMode = 1;
	} else if (m_musicLoopMP3.openFromFile(filename)) {
		m_musicLoopMP3.play();
		m_musicMode = 2;
	} else if (m_musicLoop.openFromFile(filename)) {
		m_musicLoop.play();
		m_musicMode = 3;
	} else {
		Notification::spawn(_("This song file type isn't supported."));
		return false;
	}

	m_musicFileName = filename;

	return true;*/
}

void BrowseState::stopBGM()
{
	m_musicLoopBCSTM.stop();
	m_musicLoopMP3.stop();
	m_musicLoop.stop();

	m_musicMode = 0;
}

void BrowseState::reloadKeyboard()
{
	m_isJapKeyboard = false;
	m_isTIDKeyboard = false;

	//Loading the keyboard locale file
	if (std::string(Config::get(Config::Keyboard).GetString()) == "azerty")
		m_keyboard.loadFromFile("kb/keyboard.azerty.xml");
	else if (std::string(Config::get(Config::Keyboard).GetString()) == "qwertz")
		m_keyboard.loadFromFile("kb/keyboard.qwertz.xml");
	else if (std::string(Config::get(Config::Keyboard).GetString()) == "jap") {
		m_keyboard.loadFromFile("kb/keyboard.jap.xml");
		m_isJapKeyboard = true;
	}
	else if (std::string(Config::get(Config::Keyboard).GetString()) == "tid") {
		m_keyboard.loadFromFile("kb/keyboard.titleid.xml");
		m_isTIDKeyboard = true;
	}
	else
		m_keyboard.loadFromFile("kb/keyboard.qwerty.xml");
}

int BrowseState::getMode()
{
	return m_mode;
}

bool BrowseState::isAppInfoLoaded()
{
	if (!m_appInfo.getAppItem())
		return false;
	else
		return true;
}

bool BrowseState::getJapKeyboard()
{
	return m_isJapKeyboard;
}

bool BrowseState::getTIDKeyboard()
{
	return m_isTIDKeyboard;
}

std::string BrowseState::getCtrSdPath()
{
	return m_ctrSdPath;
}

void BrowseState::settingsSaveToConfig()
{
	if (m_settingsGUI)
		m_settingsGUI->saveToConfig();
}

void BrowseState::setInstalledListSearchText(std::string text)
{
	if (text.empty())
		m_textSearchInstalledList.setString(_("Search..."));
	else
		m_textSearchInstalledList.setString(text);

	m_textSearchInstalledList.setOrigin(m_textSearchInstalledList.getGlobalBounds().width / 2, 0);
}

void BrowseState::wokeUp()
{
	m_topInfos.wokeUp();
}

void BrowseState::blockControls(bool isControlsBlocked)
{
	m_isControlsBlocked = isControlsBlocked;
}

bool BrowseState::isControlsBlocked()
{
	return m_isControlsBlocked;
}

} // namespace FreeShop
