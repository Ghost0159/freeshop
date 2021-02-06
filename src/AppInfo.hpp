#ifndef FREESHOP_APPINFO_HPP
#define FREESHOP_APPINFO_HPP

#include <cpp3ds/Graphics/Drawable.hpp>
#include <cpp3ds/Graphics/Texture.hpp>
#include <cpp3ds/Graphics/Sprite.hpp>
#include <TweenEngine/TweenManager.h>
#include <cpp3ds/Window/Event.hpp>
#include <cpp3ds/System/Thread.hpp>
#include "TweenObjects.hpp"
#include "AppItem.hpp"
#include "RichText.hpp"
#include "AppStats.hpp"
#include <cpp3ds/Window/Window.hpp>
#include "States/State.hpp"
#include "TweenObjects.hpp"
#include "GUI/Scrollable.hpp"
#include "GUI/ScrollBar.hpp"

namespace FreeShop {

class AppInfo : public cpp3ds::Drawable, public util3ds::TweenTransformable<cpp3ds::Transformable>, public Scrollable {
public:
	static const int ALPHA = 11;

	AppInfo();
	~AppInfo();

	static AppInfo &getInstance();

	void drawTop(cpp3ds::Window &window);

	bool processEvent(const cpp3ds::Event& event);
	void update(float delta);

	void loadApp(std::shared_ptr<AppItem> appItem);
	const std::shared_ptr<AppItem> getAppItem() const;

	void setCurrentScreenshot(int screenshotIndex);

	virtual void setScroll(float position);
	virtual float getScroll();
	virtual const cpp3ds::Vector2f &getScrollSize();

	void setCanDraw(bool canDraw);
	bool canDraw();

protected:
	virtual void draw(cpp3ds::RenderTarget &target, cpp3ds::RenderStates states) const;

	virtual void setValues(int tweenType, float *newValues);
	virtual int getValues(int tweenType, float *returnValues);

	void updateInfo();

private:
	void setDescription(const rapidjson::Value &jsonDescription);
	void setScreenshots(const rapidjson::Value &jsonScreenshots);
	void addScreenshot(int index, const rapidjson::Value &jsonScreenshot);
	void addEmptyScreenshot(int index, bool isUpper);
	void addInfoToDescription(const rapidjson::Value &jsonTitle);
	cpp3ds::String calculateWordWrapping(cpp3ds::String sentence);

	void setBanner(const rapidjson::Value &jsonBanner);
	void closeBanner();

	cpp3ds::Sprite m_icon;
	util3ds::TweenText m_textTitle;
	util3ds::RichText m_textDescriptionDrawn;
	util3ds::TweenText m_textDescription;
	util3ds::TweenText m_textLittleDescription;
	cpp3ds::Text m_textTitleId;
	float m_descriptionVelocity;
	cpp3ds::RectangleShape m_fadeTextRect;

	bool m_isDemoInstalled;
	cpp3ds::Text m_textDemo;
	cpp3ds::Text m_textIconDemo;

	util3ds::TweenText m_textDownload;
	util3ds::TweenText m_textDelete;
	util3ds::TweenText m_textExecute;
	util3ds::TweenText m_textSleepDownload;

	std::vector<std::unique_ptr<cpp3ds::Texture>> m_screenshotTopTextures;
	std::vector<std::unique_ptr<cpp3ds::Texture>> m_screenshotBottomTextures;
	std::vector<std::unique_ptr<cpp3ds::Sprite>> m_screenshotTops;
	std::vector<std::unique_ptr<cpp3ds::Sprite>> m_screenshotBottoms;

	int m_currentScreenshot;
	util3ds::TweenSprite m_screenshotTopTop;
	util3ds::TweenSprite m_screenshotTopBottom;
	util3ds::TweenSprite m_screenshotBottom;
	util3ds::TweenText m_arrowLeft;
	util3ds::TweenText m_arrowRight;
	util3ds::TweenText m_close;

	cpp3ds::RectangleShape m_screenshotsBackground;
	cpp3ds::RectangleShape m_separator;
	cpp3ds::Text m_textScreenshotsEmpty;

	util3ds::TweenText m_textNothingSelected;
	cpp3ds::RectangleShape m_fadeRect;

	bool m_isBannerLoaded;
	cpp3ds::Texture m_gameBannerTexture;
	util3ds::TweenSprite m_gameBanner;
	util3ds::TweenRectangleShape m_overlay;
	util3ds::TweenSprite m_topIcon;

	std::shared_ptr<AppItem> m_appItem;

	AppStats m_appStats;
	bool m_showAppStats;

	cpp3ds::Thread m_threadUninstallGame;
	cpp3ds::Thread m_threadUninstallDemo;
	void uninstallGame();
	void uninstallDemo();

	bool m_canDraw;

	TweenEngine::TweenManager m_tweenManager;

	ScrollBar m_scrollbar;
	cpp3ds::Vector2f m_scrollSize;
	float m_scrollPos;
};

} // namepace FreeShop


#endif // FREESHOP_APPINFO_HPP
