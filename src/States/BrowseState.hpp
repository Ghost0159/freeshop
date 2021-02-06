#ifndef FREESHOP_BROWSESTATE_HPP
#define FREESHOP_BROWSESTATE_HPP

#include "State.hpp"
#include "../AppList.hpp"
#include "../AppInfo.hpp"
#include "../DownloadQueue.hpp"
#include "../Keyboard/Keyboard.hpp"
#include "../RichText.hpp"
#include "../IconSet.hpp"
#include "../BotInformations.hpp"
#include "../TopInformations.hpp"
#include "../GUI/Settings.hpp"
#include "../GUI/ScrollBar.hpp"
#include "../MusicBCSTM.hpp"
#include "../MusicMP3.hpp"
#include <cpp3ds/Graphics/Sprite.hpp>
#include <cpp3ds/Graphics/Texture.hpp>
#include <cpp3ds/System/Clock.hpp>
#include <cpp3ds/Graphics/RectangleShape.hpp>
#include <TweenEngine/TweenManager.h>
#include <cpp3ds/Audio/SoundBuffer.hpp>
#include <cpp3ds/Audio/Sound.hpp>
#include <cpp3ds/Audio/Music.hpp>

#include <Gwen/Renderers/cpp3ds.h>
#include <Gwen/Input/cpp3ds.h>
#include <Gwen/Skins/Simple.h>
#include <Gwen/Skins/TexturedBase.h>

namespace FreeShop {

class BrowseState : public State
{
public:
	BrowseState(StateStack& stack, Context& context, StateCallback callback);
	~BrowseState();

	virtual void renderTopScreen(cpp3ds::Window& window);
	virtual void renderBottomScreen(cpp3ds::Window& window);
	virtual bool update(float delta);
	virtual bool processEvent(const cpp3ds::Event& event);

	bool playBGM(const std::string &filename);
	bool playBGMeShop();
	void stopBGM();

	void reloadKeyboard();
	int getMode();
	bool isAppInfoLoaded();
	bool getJapKeyboard();
	bool getTIDKeyboard();
	void setInstalledListSearchText(std::string text);

	static cpp3ds::Clock clockDownloadInactivity;
	void wokeUp();

	std::string getCtrSdPath();

	void blockControls(bool isControlsBlocked);
	bool isControlsBlocked();

	void settingsSaveToConfig();

private:
	enum Mode {
		Info = 0,
		App,
		Downloads,
		Installed,
		Settings,
		Search,
	};

	void initialize();
	void setMode(Mode mode);
	void loadApp();

private:
	AppInfo m_appInfo;
	util3ds::TweenText m_textListEmpty;

	TweenEngine::TweenManager m_tweenManager;

	float m_appListPositionX;

	cpp3ds::Thread m_threadInitialize;
	cpp3ds::Thread m_threadLoadApp;
	bool m_threadBusy;

	IconSet m_iconSet;
	IconSet m_iconInfo;
	BotInformations m_botInfos;
	TopInformations m_topInfos;

	cpp3ds::Sprite m_AP_Sprite;
	cpp3ds::Texture m_AP_Texture;

	size_t m_activeDownloadCount;
	util3ds::TweenText m_textActiveDownloads;
	util3ds::TweenText m_textInstalledCount;

	ScrollBar m_scrollbarInstalledList;
	ScrollBar m_scrollbarDownloadQueue;

	int m_counter;
	bool m_isControlsBlocked;

	// Sounds
	cpp3ds::Sound  m_soundClick;
	cpp3ds::Sound  m_soundLoading;

	MusicMP3 m_musicLoopMP3;
	MusicBCSTM m_musicLoopBCSTM;
	cpp3ds::Music m_musicLoop;

	int m_musicMode;
	std::string m_musicFileName;

	bool m_isSliderOff;

	// Keyboard
	util3ds::Keyboard m_keyboard;
	cpp3ds::String m_lastKeyboardInput;

	std::vector<util3ds::RichText> m_textMatches;
	std::vector<int> m_keyHistory;

	bool m_isJapKeyboard;
	bool m_isTIDKeyboard;

	// Transitioning
	bool m_isTransitioning;
	Mode m_mode;
	util3ds::TweenRectangleShape m_whiteScreen;
	util3ds::TweenableView m_bottomView;

	// GWEN
	Gwen::Renderer::cpp3dsRenderer *m_gwenRenderer;
	Gwen::Skin::TexturedBase *m_gwenSkin;
	GUI::Settings *m_settingsGUI;

	//Nintendo 3DS SD Folder
	std::string m_ctrSdPath;

	// Installed list search box
	util3ds::TweenText m_textSearchInstalledList;
	util3ds::TweenRectangleShape m_textBoxInstalledList;
};

extern BrowseState *g_browseState;
extern bool g_isLatestFirmwareVersion;

} // namespace FreeShop

#endif // FREESHOP_BROWSESTATE_HPP
