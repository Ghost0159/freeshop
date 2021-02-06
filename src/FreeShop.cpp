#include <sys/stat.h>
#include "FreeShop.hpp"
#include "States/TitleState.hpp"
#include "Notification.hpp"
#include "States/DialogState.hpp"
#include "States/LoadingState.hpp"
#include "States/SyncState.hpp"
#include "States/BrowseState.hpp"
#include "States/NewsState.hpp"
#include "TitleKeys.hpp"
#include "Util.hpp"
#include "States/SleepState.hpp"
#include "States/QrScannerState.hpp"
#include "GUI/Settings.hpp"
#ifndef EMULATION
#include <3ds.h>
#endif

using namespace cpp3ds;
using namespace TweenEngine;

namespace FreeShop {

cpp3ds::Uint64 g_requestJump = 0;
bool g_requestShutdown = false;
bool g_requestReboot = false;

FreeShop::FreeShop()
: Game(0x100000)
{
	m_stateStack = new StateStack(State::Context(m_text, m_data));
	m_stateStack->registerState<TitleState>(States::Title);
	m_stateStack->registerState<LoadingState>(States::Loading);
	m_stateStack->registerState<SyncState>(States::Sync);
	m_stateStack->registerState<BrowseState>(States::Browse);
	m_stateStack->registerState<SleepState>(States::Sleep);
	m_stateStack->registerState<QrScannerState>(States::QrScanner);
	m_stateStack->registerState<DialogState>(States::Dialog);
	m_stateStack->registerState<NewsState>(States::News);

#ifdef EMULATION
	m_stateStack->pushState(States::Browse);
#else
	m_stateStack->pushState(States::Loading);
	m_stateStack->pushState(States::Sync);
	m_stateStack->pushState(States::Title);
#endif

	textFPS.setFillColor(cpp3ds::Color::Red);
	textFPS.setCharacterSize(20);

	// Set up directory structure, if necessary
	std::string path = cpp3ds::FileSystem::getFilePath(FREESHOP_DIR);
	if (!pathExists(path.c_str(), false))
		makeDirectory(path.c_str());

	path = cpp3ds::FileSystem::getFilePath(FREESHOP_DIR "/tmp");
	if (pathExists(path.c_str(), false))
		removeDirectory(path.c_str());
	mkdir(path.c_str(), 0777);

	path = cpp3ds::FileSystem::getFilePath(FREESHOP_DIR "/cache");
	if (!pathExists(path.c_str(), false))
		mkdir(path.c_str(), 0777);

	path = cpp3ds::FileSystem::getFilePath(FREESHOP_DIR "/keys");
	if (!pathExists(path.c_str(), false))
		mkdir(path.c_str(), 0777);

	path = cpp3ds::FileSystem::getFilePath(FREESHOP_DIR "/news");
	if (!pathExists(path.c_str(), false))
		mkdir(path.c_str(), 0777);

	path = cpp3ds::FileSystem::getFilePath(FREESHOP_DIR "/music");
	if (!pathExists(path.c_str(), false))
		mkdir(path.c_str(), 0777);

	path = cpp3ds::FileSystem::getFilePath(FREESHOP_DIR "/music/eshop");
	if (!pathExists(path.c_str(), false))
		mkdir(path.c_str(), 0777);

	#ifndef NDEBUG
	path = cpp3ds::FileSystem::getFilePath(FREESHOP_DIR "/debug/dev/");
	if (!pathExists(path.c_str(), false))
		mkdir(path.c_str(), 0777);
	#endif

	Config::loadFromFile();

	// Load chosen language, correct auto-detect with separate Spanish/Portuguese
	std::string langCode = Config::get(Config::Language).GetString();
#ifdef _3DS
	u8 region;
	CFGU_SecureInfoGetRegion(&region);
	if (langCode == "auto")
	{
		if (cpp3ds::I18n::getLanguage() == cpp3ds::Language::Spanish)
			langCode = (region == CFG_REGION_USA) ? "es_US" : "es_ES";
		else if (cpp3ds::I18n::getLanguage() == cpp3ds::Language::Portuguese)
			langCode = (region == CFG_REGION_USA) ? "pt_BR" : "pt_PT";
	}
#endif

	if (langCode != "auto")
		cpp3ds::I18n::loadLanguageFile(_("lang/%s.lang", langCode.c_str()));

	// If override.lang exists, use it instead of chosen language
	std::string testLangFilename(FREESHOP_DIR "/override.lang");
	if (pathExists(testLangFilename.c_str()))
		cpp3ds::I18n::loadLanguageFile(testLangFilename);

	// Used to theme the loading background
	if (pathExists(FREESHOP_DIR "/theme/images/topBG.png", true)) {
		m_rectTopBG.setTexture(&AssetManager<cpp3ds::Texture>::get(FREESHOP_DIR "/theme/images/topBG.png"));
		m_rectTopBG.setPosition(0.f, 0.f);
		m_topBG = true;
	} else if (Config::get(Config::DarkTheme).GetBool()) {
		m_rectTopBG.setTexture(&AssetManager<cpp3ds::Texture>::get("darkimages/topBG.png"));
		m_rectTopBG.setPosition(0.f, 0.f);
		m_topBG = true;
	}

	if (pathExists(FREESHOP_DIR "/theme/images/botBG.png", true)) {
		m_rectBotBG.setTexture(&AssetManager<cpp3ds::Texture>::get(FREESHOP_DIR "/theme/images/botBG.png"));
		m_rectBotBG.setPosition(0.f, 0.f);
		m_botBG = true;
	} else if (Config::get(Config::DarkTheme).GetBool()) {
		m_rectBotBG.setTexture(&AssetManager<cpp3ds::Texture>::get("darkimages/botBG.png"));
		m_rectBotBG.setPosition(0.f, 0.f);
		m_botBG = true;
	}
}

FreeShop::~FreeShop()
{
	delete m_stateStack;
}

void FreeShop::update(float delta)
{
#ifndef EMULATION
	// 3DSX version need to do this here...
	if (g_requestJump != 0 || g_requestShutdown || g_requestReboot) {
		prepareToCloseApp();

		if (g_requestShutdown && envIsHomebrew()) {
			//Init the services and turn off the console
			ptmSysmInit();
			PTMSYSM_ShutdownAsync(0);
			ptmSysmExit();
		} else if (g_requestJump != 0) {
			//Var initialization
			Result res = 0;
			u8 hmac[0x20];
			memset(hmac, 0, sizeof(hmac));

			//Check on which media launch the title
			FS_MediaType mediaType = ((g_requestJump >> 32) == TitleKeys::DSiWare) ? MEDIATYPE_NAND : MEDIATYPE_SD;

			FS_CardType type;
			bool cardInserted;
			cardInserted = (R_SUCCEEDED(FSUSER_CardSlotIsInserted(&cardInserted)) && cardInserted && R_SUCCEEDED(FSUSER_GetCardType(&type)) && type == CARD_CTR);
			if (cardInserted)
			{
				// Retry a bunch of times. When the card is newly inserted,
				// it sometimes takes a short while before title can be read.
				int retryCount = 100;
				u64 cardTitleId;
				while (retryCount-- > 0)
					if (R_SUCCEEDED(AM_GetTitleList(nullptr, MEDIATYPE_GAME_CARD, 1, &cardTitleId)))
					{
						try
						{
							if (cardTitleId == g_requestJump)
								mediaType = MEDIATYPE_GAME_CARD;
						}
						catch (int e)
						{
							//
						}
						break;
					}
					else
						cpp3ds::sleep(cpp3ds::milliseconds(5));
			}

			//Do the application jump
			if (R_SUCCEEDED(res = APT_PrepareToDoApplicationJump(0, g_requestJump, mediaType)))
				res = APT_DoApplicationJump(0, 0, hmac);
		}
	}
#endif
	// Need to update before checking if empty
	m_stateStack->update(delta);
	if (m_stateStack->isEmpty() || g_requestJump != 0 || g_requestShutdown || g_requestReboot) {
		prepareToCloseApp();

		exit();
	}

	Notification::update(delta);

#ifndef NDEBUG
	static int i;
	if (i++ % 10 == 0) {
		textFPS.setString(_("%.1f fps", 1.f / delta));
		textFPS.setPosition(395 - textFPS.getGlobalBounds().width, 2.f);
	}
#endif
}

void FreeShop::processEvent(Event& event)
{
	m_stateStack->processEvent(event);
}

void FreeShop::renderTopScreen(Window& window)
{
	if (m_topBG)
		window.draw(m_rectTopBG);
	else
		window.clear(Color::White);

	m_stateStack->renderTopScreen(window);
	for (auto& notification : Notification::notifications)
		window.draw(*notification);

#ifndef NDEBUG
	window.draw(textFPS);
#endif
}

void FreeShop::renderBottomScreen(Window& window)
{
	if (m_botBG)
		window.draw(m_rectBotBG);
	else
		window.clear(Color::White);

	m_stateStack->renderBottomScreen(window);
		//window.draw(m_AP_Sprite);
}

void FreeShop::prepareToCloseApp()
{
	DownloadQueue::getInstance().suspend();
	DownloadQueue::getInstance().save();

	if (g_browseState)
		g_browseState->settingsSaveToConfig();

	Config::set(Config::CleanExit, true);
	Config::saveToFile();
}

} // namespace FreeShop
