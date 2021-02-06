#include <cpp3ds/System/Err.hpp>
#include <cpp3ds/Window/Window.hpp>
#include <iostream>
#include <cpp3ds/System/I18n.hpp>
#include "AssetManager.hpp"
#include "AppInfo.hpp"
#include "Download.hpp"
#include "Util.hpp"
#include "DownloadQueue.hpp"
#include "Notification.hpp"
#include "InstalledList.hpp"
#include "FreeShop.hpp"
#include "Theme.hpp"
#include "Config.hpp"
#include "LoadInformations.hpp"
#include "States/DialogState.hpp"
#include <sstream>
#include <TweenEngine/Tween.h>
#include <cpp3ds/System/FileInputStream.hpp>
#include <cpp3ds/System/FileSystem.hpp>
#include <sys/stat.h>
#include <cmath>


namespace FreeShop
{

AppInfo::AppInfo()
: m_appItem(nullptr)
, m_currentScreenshot(0)
, m_descriptionVelocity(0.f)
, m_isDemoInstalled(false)
, m_isBannerLoaded(false)
, m_showAppStats(false)
, m_threadUninstallGame(&AppInfo::uninstallGame, this)
, m_threadUninstallDemo(&AppInfo::uninstallDemo, this)
, m_canDraw(true)
{
	m_textDownload.setFillColor(cpp3ds::Color::White);
	m_textDownload.setOutlineColor(cpp3ds::Color(0, 0, 0, 200));
	m_textDownload.setOutlineThickness(2.f);
	m_textDownload.setFont(AssetManager<cpp3ds::Font>::get("fonts/fontawesome.ttf"));
	m_textDownload.setString("\uf019");
	m_textDownload.setCharacterSize(30);
	m_textDownload.setPosition(285.f, 200.f);
	m_textSleepDownload = m_textDownload;
	m_textSleepDownload.setFillColor(cpp3ds::Color::White);
	m_textSleepDownload.setString("\uf186");
	m_textSleepDownload.setPosition(192.f, 198.f);
	m_textDelete = m_textDownload;
	m_textDelete.setString("\uf1f8");
	m_textDelete.setPosition(285.f, 200.f);
	m_textExecute = m_textDownload;
	m_textExecute.setString("\uf01d");
	m_textExecute.setPosition(192.f, 200.f);
	m_arrowLeft.setFont(AssetManager<cpp3ds::Font>::get("fonts/fontawesome.ttf"));
	m_arrowLeft.setCharacterSize(24);
	m_arrowLeft.setFillColor(cpp3ds::Color(255, 255, 255, 150));
	m_arrowLeft.setOutlineColor(cpp3ds::Color(0, 0, 0, 100));
	m_arrowLeft.setOutlineThickness(1.f);
	m_arrowLeft.setPosition(4.f, 100.f);
	m_arrowLeft.setString("\uf053");
	m_arrowRight = m_arrowLeft;
	m_arrowRight.setPosition(298.f, 100.f);
	m_arrowRight.setString("\uf054");
	m_close = m_arrowLeft;
	m_close.setCharacterSize(30);
	m_close.setPosition(285.f, 4.f);
	m_close.setString("\uf00d");

	if (Theme::isTextThemed)
		m_screenshotsBackground.setOutlineColor(Theme::boxOutlineColor);
	else
		m_screenshotsBackground.setOutlineColor(cpp3ds::Color(158, 158, 158, 255));
	m_screenshotsBackground.setOutlineThickness(1);
	if (Theme::isTextThemed)
		m_screenshotsBackground.setFillColor(Theme::boxColor);
	else
		m_screenshotsBackground.setFillColor(cpp3ds::Color(245, 245, 245));
	m_screenshotsBackground.setSize(cpp3ds::Vector2f(320.f, 74.f));
	m_screenshotsBackground.setPosition(0.f, 166.f);

	if (Theme::isTextThemed)
		m_separator.setFillColor(Theme::boxOutlineColor);
	else
		m_separator.setFillColor(cpp3ds::Color(158, 158, 158, 255));
	m_separator.setSize(cpp3ds::Vector2f(1.f, 74.f));
	m_separator.setPosition(184.f, 166.f);

	m_textScreenshotsEmpty.setCharacterSize(12);
	if (Theme::isTextThemed)
		m_textScreenshotsEmpty.setFillColor(Theme::secondaryTextColor);
	else
		m_textScreenshotsEmpty.setFillColor(cpp3ds::Color(100, 100, 100, 255));
	m_textScreenshotsEmpty.useSystemFont();
	m_textScreenshotsEmpty.setString(_("No Screenshots"));
	cpp3ds::FloatRect textRect = m_textScreenshotsEmpty.getLocalBounds();
	m_textScreenshotsEmpty.setOrigin(m_textScreenshotsEmpty.getLocalBounds().width / 2, m_textScreenshotsEmpty.getLocalBounds().height / 2);
	m_textScreenshotsEmpty.setPosition(92.f, 199.f);

	m_textNothingSelected.setString(_("Select a game to start"));
	m_textNothingSelected.setCharacterSize(16);
	if (Theme::isTextThemed)
		m_textNothingSelected.setFillColor(Theme::secondaryTextColor);
	else
		m_textNothingSelected.setFillColor(cpp3ds::Color(0, 0, 0, 120));
	m_textNothingSelected.useSystemFont();
	m_textNothingSelected.setPosition(160.f, 105.f);
	m_textNothingSelected.setOrigin(m_textNothingSelected.getLocalBounds().width / 2, m_textNothingSelected.getLocalBounds().height / 2);
	TweenEngine::Tween::to(m_textNothingSelected, m_textNothingSelected.SCALE_XY, 4.f)
		.target(0.9f, 0.9f).repeatYoyo(-1, 0.f).start(m_tweenManager);

	m_textTitle.setCharacterSize(15);
	if (Theme::isTextThemed)
		m_textTitle.setFillColor(Theme::primaryTextColor);
	else
		m_textTitle.setFillColor(cpp3ds::Color::Black);
	m_textTitle.setPosition(28.f, 27.f);
	m_textTitle.useSystemFont();

	m_textDescription.setCharacterSize(12);
	m_textDescriptionDrawn.setCharacterSize(12);
	if (Theme::isTextThemed)
		m_textDescriptionDrawn << Theme::secondaryTextColor;
	else
		m_textDescriptionDrawn << cpp3ds::Color(100, 100, 100, 255);
	m_textDescription.useSystemFont();
	m_textDescriptionDrawn.useSystemFont();

	m_textLittleDescription.setCharacterSize(10);
	if (Theme::isTextThemed)
		m_textLittleDescription.setFillColor(Theme::secondaryTextColor);
	else
		m_textLittleDescription.setFillColor(cpp3ds::Color(100, 100, 100, 255));
	m_textLittleDescription.useSystemFont();
	m_textLittleDescription.setPosition(28.f, 44.f);

	m_textTitleId.setCharacterSize(11);
	if (Theme::isTextThemed)
		m_textTitleId.setFillColor(Theme::secondaryTextColor);
	else
		m_textTitleId.setFillColor(cpp3ds::Color(80, 80, 80, 255));
	m_textTitleId.setPosition(192.f, 185.f);

	m_textDemo.setString(_("Demo"));
	if (Theme::isTextThemed)
		m_textDemo.setFillColor(Theme::secondaryTextColor);
	else
		m_textDemo.setFillColor(cpp3ds::Color(100, 100, 100));
	m_textDemo.setCharacterSize(10);
	m_textDemo.setPosition(214.f, 171.f);
	m_textDemo.useSystemFont();
	m_textIconDemo.setFont(AssetManager<cpp3ds::Font>::get("fonts/fontawesome.ttf"));
	m_textIconDemo.setFillColor(cpp3ds::Color(50, 100, 50));
	m_textIconDemo.setCharacterSize(18);
	m_textIconDemo.setPosition(192.f, 166.f);

	if (fopen(FREESHOP_DIR "/theme/images/fade.png", "rb"))
		m_fadeTextRect.setTexture(&AssetManager<cpp3ds::Texture>::get(FREESHOP_DIR "/theme/images/fade.png"));
	else
		m_fadeTextRect.setTexture(&AssetManager<cpp3ds::Texture>::get("images/fade.png"));

	m_fadeTextRect.setSize(cpp3ds::Vector2f(250.f, 8.f));
	m_fadeTextRect.setOrigin(m_fadeTextRect.getSize());
	m_fadeTextRect.setRotation(180.f);
	m_fadeTextRect.setPosition(102.f, 46.f);

	m_icon.setPosition(2.f, 30.f);
	m_icon.setScale(.5f, .5f);

	m_fadeRect.setPosition(0.f, 30.f);
	m_fadeRect.setSize(cpp3ds::Vector2f(320.f, 210.f));
	m_fadeRect.setFillColor(cpp3ds::Color::White);

	m_overlay.setSize(cpp3ds::Vector2f(400.f, 240.f));
	m_overlay.setFillColor(cpp3ds::Color::Transparent);

	m_topIcon.setPosition(5.f, 36.f);
}

AppInfo::~AppInfo()
{

}

AppInfo &AppInfo::getInstance()
{
	static AppInfo appInfo;
	return appInfo;
}

void AppInfo::drawTop(cpp3ds::Window &window)
{
	if (m_currentScreenshot)
		window.draw(m_screenshotTopTop);

	if (m_isBannerLoaded) {
		window.draw(m_overlay);
		window.draw(m_gameBanner);
		window.draw(m_topIcon);
	}

	m_appStats.drawTop(window);
}

void AppInfo::draw(cpp3ds::RenderTarget &target, cpp3ds::RenderStates states) const
{
	if (!m_canDraw)
		return;

	states.transform *= getTransform();

	if (m_appItem)
	{
		target.draw(m_screenshotsBackground, states);
		target.draw(m_separator, states);
		if (m_screenshotTops.empty())
			target.draw(m_textScreenshotsEmpty, states);

		target.draw(m_icon, states);
		target.draw(m_textTitle, states);
		target.draw(m_textLittleDescription, states);

		if (Config::get(Config::TitleID).GetBool())
			target.draw(m_textTitleId, states);

		states.scissor = cpp3ds::UintRect(0, 55, 320, 109);
		target.draw(m_textDescriptionDrawn, states);
		target.draw(m_scrollbar);
		states.scissor = cpp3ds::UintRect();

		if (m_appItem->isInstalled())
		{
			target.draw(m_textExecute, states);
			target.draw(m_textDelete, states);
		}
		else if (!DownloadQueue::getInstance().isDownloading(m_appItem))
		{
			target.draw(m_textDownload, states);
			target.draw(m_textSleepDownload, states);
		}

		if (!m_appItem->getDemos().empty() && !DownloadQueue::getInstance().isDownloading(m_appItem->getDemos()[0]))
		{
			target.draw(m_textDemo, states);
			target.draw(m_textIconDemo, states);
		}

		int i = 1;
		for (auto& screenshot : m_screenshotTops) {
			if (m_currentScreenshot != i)
				target.draw(*screenshot, states);

			i++;
		}

		i = 1;
		for (auto& screenshot : m_screenshotBottoms) {
			if (m_currentScreenshot != i)
				target.draw(*screenshot, states);

			i++;
		}

		target.draw(m_fadeRect, states);

		target.draw(m_appStats, states);
	}
	else
	{
		target.draw(m_textNothingSelected, states);
	}

	if (m_currentScreenshot)
	{
		target.draw(m_screenshotTopBottom);
		target.draw(m_screenshotBottom);
		if (m_currentScreenshot > 1)
			target.draw(m_arrowLeft);
		if (m_currentScreenshot < m_screenshotTops.size())
			target.draw(m_arrowRight);
		target.draw(m_close);
	}
}

void AppInfo::setValues(int tweenType, float *newValues)
{
	switch (tweenType) {
		case ALPHA: {
			cpp3ds::Color color = m_fadeRect.getFillColor();
			color.a = 255.f - std::max(std::min(newValues[0], 255.f), 0.f);
			m_fadeRect.setFillColor(color);
			break;
		}
		default:
			TweenTransformable::setValues(tweenType, newValues);
	}
}

int AppInfo::getValues(int tweenType, float *returnValues)
{
	switch (tweenType) {
		case ALPHA:
			returnValues[0] = 255.f - m_fadeRect.getFillColor().a;
			return 1;
		default:
			return TweenTransformable::getValues(tweenType, returnValues);
	}
}

void AppInfo::loadApp(std::shared_ptr<AppItem> appItem)
{
	if (m_appItem == appItem)
		return;

	m_appItem = appItem;
	m_appStats.setVisible(false);

	if (appItem)
	{
		std::string countryCode = getCountryCode(appItem->getRegions());
		std::string jsonFilename = appItem->getJsonFilename();
		std::string urlTitleData = "https://samurai.ctr.shop.nintendo.net/samurai/ws/" + countryCode + "/title/" + appItem->getContentId() + "/?shop_id=1";
		std::string dir = cpp3ds::FileSystem::getFilePath(FREESHOP_DIR "/tmp/" + appItem->getTitleIdStr());
		if (!pathExists(dir.c_str(), false))
			mkdir(dir.c_str(), 0777);

		if (!appItem->isCached())
		{
			Download download(urlTitleData, jsonFilename);
			download.setField("Accept", "application/json");
			download.run();

			auto status = download.getLastResponse().getStatus();
			if (status != cpp3ds::Http::Response::Ok && status != cpp3ds::Http::Response::PartialContent)
			{
				urlTitleData = "https://samurai.ctr.shop.nintendo.net/samurai/ws/" + appItem->getUriRegion() + "/title/" + appItem->getContentId() + "/?shop_id=1";
				download.setUrl(urlTitleData);
				download.run();
			}
		}

		updateInfo();

		m_textTitle.setString("");
		m_textLittleDescription.setString("");
		m_textDescriptionDrawn.clear();
		m_textTitleId.setString(appItem->getTitleIdStr());

		// Transform the game's release date to human readable time
		time_t t = m_appItem->getReleaseDate();
		struct tm * timeinfo;
		timeinfo = localtime(&t);

		char timeTextFmt[12];
		strftime(timeTextFmt, 12, "%d/%m/%Y", timeinfo);

		// Transform the game's size to human readable size
		cpp3ds::String tempSizeHolder;
		if (m_appItem->getFilesize() > 1024 * 1024 * 1024)
			tempSizeHolder = _("%.1f GB", static_cast<float>(m_appItem->getFilesize()) / 1024.f / 1024.f / 1024.f);
		else if (m_appItem->getFilesize() > 1024 * 1024)
			tempSizeHolder = _("%.1f MB", static_cast<float>(m_appItem->getFilesize()) / 1024.f / 1024.f);
		else
			tempSizeHolder = _("%d KB", m_appItem->getFilesize() / 1024);

		cpp3ds::IntRect textureRect;
		m_icon.setTexture(*appItem->getIcon(textureRect), true);
		m_icon.setTextureRect(textureRect);

		m_screenshotTops.clear();
		m_screenshotTopTextures.clear();
		m_screenshotBottoms.clear();
		m_screenshotBottomTextures.clear();

		rapidjson::Document doc;
		std::string json;
		cpp3ds::FileInputStream file;
		if (file.open(jsonFilename))
		{
			json.resize(file.getSize());
			file.read(&json[0], json.size());
			doc.Parse(json.c_str());
			if (!doc.HasParseError())
			{
				if (doc["title"].HasMember("banner_url"))
					setBanner(doc["title"]["banner_url"]);

				if (doc["title"].HasMember("screenshots"))
					setScreenshots(doc["title"]["screenshots"]["screenshot"]);

				if (doc["title"].HasMember("description"))
					setDescription(doc["title"]["description"]);
				m_textDescription.setPosition(2.f, 55.f);
				m_textDescriptionDrawn.setPosition(2.f, 55.f);

				if (doc["title"].HasMember("star_rating_info"))
					m_appStats.loadApp(appItem, doc["title"]["star_rating_info"]);

				if (doc["title"].HasMember("preference"))
					m_appStats.loadPreferences(doc["title"]["preference"]);

				m_textTitle.setString(appItem->getTitle());

				if (doc["title"].HasMember("copyright")) {
					cpp3ds::String copyright = doc["title"]["copyright"]["text"].GetString();
					copyright.replace("<br>", "");
					copyright.replace("<BR>", "");
					copyright.replace("<br/>", "");

					m_textDescriptionDrawn << "\n\n";
					m_textDescriptionDrawn << calculateWordWrapping(copyright);
				}

				if (Config::get(Config::ShowGameDescription).GetBool())
					addInfoToDescription(doc["title"]);

				// Set little description text
				cpp3ds::String textLittleDescription;
				if (doc["title"].HasMember("new") && doc["title"]["new"].GetBool())
					textLittleDescription.insert(textLittleDescription.getSize(), _("NEW - "));

				textLittleDescription.insert(textLittleDescription.getSize(), _("%s - %s", tempSizeHolder.toAnsiString().c_str(), timeTextFmt));

				if (doc["title"].HasMember("rating_info")) {
					if (doc["title"]["rating_info"].HasMember("rating_system") && doc["title"]["rating_info"].HasMember("rating")) {
						if (doc["title"]["rating_info"]["rating_system"].HasMember("name") && doc["title"]["rating_info"]["rating"].HasMember("name")) {
							textLittleDescription.insert(textLittleDescription.getSize(), _(" - %s %s", doc["title"]["rating_info"]["rating_system"]["name"].GetString(), doc["title"]["rating_info"]["rating"]["name"].GetString()));
						}
					}
				}

				m_textLittleDescription.setString(_("%s", textLittleDescription.toAnsiString().c_str()));

				// Shorten the app name if it's out of the screen
				int maxSize = 290;
				if (m_textTitle.getLocalBounds().width > maxSize) {
					cpp3ds::Text tmpText = m_textTitle;
					tmpText.setString("");
					auto s = m_textTitle.getString().toUtf8();

					for (int i = 0; i <= s.size(); ++i) {
						tmpText.setString(cpp3ds::String::fromUtf8(s.begin(), s.begin() + i));

						if (tmpText.getLocalBounds().width > maxSize) {
							cpp3ds::String titleTxt = tmpText.getString();
							titleTxt.erase(titleTxt.getSize() - 3, 3);
							titleTxt.insert(titleTxt.getSize(), "...");

							m_textTitle.setString(titleTxt);
							break;
						}
					}
				}

				m_textDownload.setFillColor(cpp3ds::Color::White);
				m_textSleepDownload.setFillColor(cpp3ds::Color::White);

				m_scrollbar.setSize(cpp3ds::Vector2u(2, 109));
				m_scrollbar.setScrollAreaSize(cpp3ds::Vector2u(320, 109));
				m_scrollbar.setDragRect(cpp3ds::IntRect(0, 20, 320, 146));
				m_scrollbar.setColor(cpp3ds::Color(0, 0, 0, 128));
				m_scrollbar.setPosition(312.f, 56.f);
				m_scrollbar.setAutoHide(false);
				m_scrollbar.attachObject(this);
				m_scrollbar.setScroll(0.f);
				m_scrollbar.markDirty();

				m_scrollSize = cpp3ds::Vector2f(320.f, m_textDescriptionDrawn.getLocalBounds().top + m_textDescriptionDrawn.getLocalBounds().height + 5.f);

				closeBanner();
			}
		}
	}
}

const std::shared_ptr<AppItem> AppInfo::getAppItem() const
{
	return m_appItem;
}

#define SCREENSHOT_BUTTON_FADEIN(object) \
	object.setFillColor(cpp3ds::Color(255,255,255,190)); \
	object.setOutlineColor(cpp3ds::Color(0,0,0,100)); \
	TweenEngine::Tween::from(object, util3ds::TweenText::FILL_COLOR_ALPHA, 4.f) \
		.target(0).start(m_tweenManager); \
	TweenEngine::Tween::from(object, util3ds::TweenText::OUTLINE_COLOR_ALPHA, 4.f) \
		.target(0).start(m_tweenManager);
#define SCREENSHOT_BUTTON_FADEOUT(object) \
	TweenEngine::Tween::to(object, util3ds::TweenText::FILL_COLOR_ALPHA, 0.5f) \
		.target(0).start(m_tweenManager); \
	TweenEngine::Tween::to(object, util3ds::TweenText::OUTLINE_COLOR_ALPHA, 0.5f) \
		.target(0).start(m_tweenManager);

void AppInfo::setCurrentScreenshot(int screenshotIndex)
{
	if (screenshotIndex != 0)
	{
		cpp3ds::Sprite* screenshotTop = m_screenshotTops[screenshotIndex-1].get();
		cpp3ds::Sprite* screenshotBottom = m_screenshotBottoms[screenshotIndex-1].get();

		m_screenshotTopTop.setTexture(*screenshotTop->getTexture(), true);
		m_screenshotTopBottom.setTexture(*screenshotTop->getTexture(), true);
		m_screenshotBottom.setTexture(*screenshotBottom->getTexture(), true);

		// Calculate scale for screenshots that are not 400x240 (top screen) / 320x240 (bottom screen)
		float scaleTopX = 400.f / m_screenshotTopBottom.getTexture()->getSize().x;
		float scaleBotX = 320.f / m_screenshotBottom.getTexture()->getSize().x;
		float scaleY = 240.f / m_screenshotTopBottom.getTexture()->getSize().y;

		// If another screenshot not already showing
		if (m_currentScreenshot == 0)
		{
			SCREENSHOT_BUTTON_FADEIN(m_close);
			SCREENSHOT_BUTTON_FADEIN(m_arrowLeft);
			SCREENSHOT_BUTTON_FADEIN(m_arrowRight);

			m_screenshotTopTop.setPosition(screenshotTop->getPosition() + cpp3ds::Vector2f(0.f, 270.f));
			m_screenshotTopTop.setScale(screenshotTop->getScale());
			m_screenshotTopBottom.setPosition(screenshotTop->getPosition());
			m_screenshotTopBottom.setScale(screenshotTop->getScale());
			m_screenshotBottom.setPosition(screenshotBottom->getPosition());
			m_screenshotBottom.setScale(screenshotBottom->getScale());

			TweenEngine::Tween::to(m_screenshotTopBottom, util3ds::TweenSprite::SCALE_XY, 0.5f)
					.target(0.18f * scaleTopX / 1.f, 0.18f * scaleY / 1.f)
					.ease(TweenEngine::TweenEquations::easeOutQuart)
					.start(m_tweenManager);
			TweenEngine::Tween::to(m_screenshotTopBottom, util3ds::TweenSprite::POSITION_XY, 0.5f)
					.targetRelative(-6.f, -6.f)
					.ease(TweenEngine::TweenEquations::easeOutQuart)
					.start(m_tweenManager);
			TweenEngine::Tween::to(m_screenshotBottom, util3ds::TweenSprite::SCALE_XY, 0.5f)
					.target(0.18f * scaleBotX / 1.f, 0.18f * scaleY / 1.f)
					.ease(TweenEngine::TweenEquations::easeOutQuart)
					.start(m_tweenManager);
			TweenEngine::Tween::to(m_screenshotBottom, util3ds::TweenSprite::POSITION_XY, 0.5f)
					.targetRelative(-6.f, 0.f)
					.ease(TweenEngine::TweenEquations::easeOutQuart)
					.start(m_tweenManager);

			TweenEngine::Tween::to(m_screenshotTopTop, util3ds::TweenSprite::SCALE_XY, 0.7f)
					.target(1.f * scaleTopX / 1.f, 1.f * scaleY / 1.f)
					.delay(0.5f)
					.start(m_tweenManager);
			TweenEngine::Tween::to(m_screenshotTopTop, util3ds::TweenSprite::POSITION_XY, 0.7f)
					.target(0.f, 0.f)
					.delay(0.5f)
					.start(m_tweenManager);
			TweenEngine::Tween::to(m_screenshotTopBottom, util3ds::TweenSprite::SCALE_XY, 0.7f)
					.target(1.f * scaleTopX / 1.f, 1.f * scaleY / 1.f)
					.delay(0.5f)
					.start(m_tweenManager);
			TweenEngine::Tween::to(m_screenshotTopBottom, util3ds::TweenSprite::POSITION_XY, 0.7f)
					.target(-40.f, -240.f)
					.delay(0.5f)
					.start(m_tweenManager);
			TweenEngine::Tween::to(m_screenshotBottom, util3ds::TweenSprite::SCALE_XY, 0.7f)
					.target(1.f * scaleBotX / 1.f, 1.f * scaleY / 1.f)
					.delay(0.5f)
					.start(m_tweenManager);
			TweenEngine::Tween::to(m_screenshotBottom, util3ds::TweenSprite::POSITION_XY, 0.7f)
					.target(0.f, 0.f)
					.delay(0.5f)
					.start(m_tweenManager);
		}
	}
	else if (m_currentScreenshot != 0)
	{
		// Close screenshot
		cpp3ds::Sprite* screenshotTop = m_screenshotTops[m_currentScreenshot-1].get();
		cpp3ds::Sprite* screenshotBottom = m_screenshotBottoms[m_currentScreenshot-1].get();

		SCREENSHOT_BUTTON_FADEOUT(m_close);
		SCREENSHOT_BUTTON_FADEOUT(m_arrowLeft);
		SCREENSHOT_BUTTON_FADEOUT(m_arrowRight);

		// Calculate scale for screenshots that are not 400x240 (top screen) / 320x240 (bottom screen)
		float scaleTopX = 400.f / m_screenshotTopBottom.getTexture()->getSize().x;
		float scaleBotX = 320.f / m_screenshotBottom.getTexture()->getSize().x;
		float scaleY = 240.f / m_screenshotTopBottom.getTexture()->getSize().y;

		TweenEngine::Tween::to(m_screenshotTopTop, util3ds::TweenSprite::SCALE_XY, 0.7f)
				.target(0.15f * scaleTopX / 1.f, 0.15f * scaleY / 1.f)
				.start(m_tweenManager);
		TweenEngine::Tween::to(m_screenshotTopTop, util3ds::TweenSprite::POSITION_XY, 0.7f)
				.target(screenshotTop->getPosition().x, screenshotTop->getPosition().y + 270.f)
				.start(m_tweenManager);
		TweenEngine::Tween::to(m_screenshotTopBottom, util3ds::TweenSprite::SCALE_XY, 0.7f)
				.target(0.15f * scaleTopX / 1.f, 0.15f * scaleY / 1.f)
				.start(m_tweenManager);
		TweenEngine::Tween::to(m_screenshotTopBottom, util3ds::TweenSprite::POSITION_XY, 0.7f)
				.target(screenshotTop->getPosition().x, screenshotTop->getPosition().y)
				.start(m_tweenManager);
		TweenEngine::Tween::to(m_screenshotBottom, util3ds::TweenSprite::SCALE_XY, 0.7f)
				.target(0.15f * scaleBotX / 1.f, 0.15f * scaleY / 1.f)
				.start(m_tweenManager);
		TweenEngine::Tween::to(m_screenshotBottom, util3ds::TweenSprite::POSITION_XY, 0.7f)
				.target(screenshotBottom->getPosition().x, screenshotBottom->getPosition().y)
				.setCallback(TweenEngine::TweenCallback::COMPLETE, [=](TweenEngine::BaseTween* source) {
					m_currentScreenshot = screenshotIndex;
				})
				.start(m_tweenManager);
		return;
	}

	m_currentScreenshot = screenshotIndex;
}

bool AppInfo::processEvent(const cpp3ds::Event &event)
{
	if (!m_appItem)
		return true;

	m_scrollbar.processEvent(event);

	if (!m_currentScreenshot)
		m_appStats.processEvent(event);

	if (m_appStats.isVisible())
		return false;

	if (m_currentScreenshot)
	{
		if (event.type == cpp3ds::Event::KeyPressed)
		{
			switch (event.key.code)
			{
				case cpp3ds::Keyboard::B:
					setCurrentScreenshot(0);
					break;
				case cpp3ds::Keyboard::DPadRight:
					if (m_currentScreenshot < m_screenshotTops.size())
						setCurrentScreenshot(m_currentScreenshot + 1);
					break;
				case cpp3ds::Keyboard::DPadLeft:
					if (m_currentScreenshot > 1)
						setCurrentScreenshot(m_currentScreenshot - 1);
					break;
				default:
					break;
			}
		}
		else if (event.type == cpp3ds::Event::TouchBegan)
		{
			if (m_close.getGlobalBounds().contains(event.touch.x, event.touch.y))
			{
				setCurrentScreenshot(0);
			}
			else if (m_currentScreenshot > 1 && event.touch.x < 140)
			{
				setCurrentScreenshot(m_currentScreenshot - 1);
			}
			else if (m_currentScreenshot < m_screenshotTops.size() && event.touch.x > 180)
			{
				setCurrentScreenshot(m_currentScreenshot + 1);
			}
		}
		return false;
	}

	if (event.type == cpp3ds::Event::TouchBegan)
	{
		if (m_textIconDemo.getGlobalBounds().contains(event.touch.x, event.touch.y))
		{
			if (!m_appItem->getDemos().empty())
			{
				cpp3ds::Uint64 demoTitleId = m_appItem->getDemos()[0];
				cpp3ds::String appTitle = m_appItem->getTitle();

				if (InstalledList::isInstalled(demoTitleId))
				{
					g_browseState->requestStackPush(States::Dialog, false, [=](void *data) mutable {
						auto event = reinterpret_cast<DialogState::Event*>(data);
						if (event->type == DialogState::GetText)
						{
							auto str = reinterpret_cast<cpp3ds::String*>(event->data);
							*str = _("You are deleting the demo for this title:\n\n%s", appTitle.toAnsiString().c_str());
							return true;
						}
						else if (event->type == DialogState::GetTitle) {
							auto str = reinterpret_cast<cpp3ds::String *>(event->data);
							*str = _("Delete a demo");
							return true;
						}
						else if (event->type == DialogState::Response)
						{
							bool *accepted = reinterpret_cast<bool*>(event->data);
							if (*accepted)
							{
								m_threadUninstallDemo.launch();
							}
							return true;
						}
						return false;
					});
				}
				else
				{
					DownloadQueue::getInstance().addDownload(m_appItem, demoTitleId, [this](bool succeeded){
						updateInfo();
					});
					Notification::spawn(_("Queued demo: %s", appTitle.toAnsiString().c_str()));
				}
			}
		}
		else if (m_appItem->isInstalled())
		{
			if (m_textExecute.getGlobalBounds().contains(event.touch.x, event.touch.y))
			{
				g_requestJump = m_appItem->getTitleId();
			}
			else if (m_textDelete.getGlobalBounds().contains(event.touch.x, event.touch.y))
			{
				cpp3ds::String appTitle = m_appItem->getTitle();
				g_browseState->requestStackPush(States::Dialog, false, [=](void *data) mutable {
					auto event = reinterpret_cast<DialogState::Event*>(data);
					if (event->type == DialogState::GetText)
					{
						auto str = reinterpret_cast<cpp3ds::String*>(event->data);
						*str = _("You are deleting this game, including all save data:\n\n%s", appTitle.toAnsiString().c_str());
						return true;
					}
					else if (event->type == DialogState::GetTitle) {
						auto str = reinterpret_cast<cpp3ds::String *>(event->data);
						*str = _("Uninstall a game");
						return true;
					}
					else if (event->type == DialogState::Response)
					{
						bool *accepted = reinterpret_cast<bool*>(event->data);
						if (*accepted)
						{
							m_threadUninstallGame.launch();
						}
						return true;
					}
					return false;
				});
			}
		}
		else if (m_textDownload.getGlobalBounds().contains(event.touch.x, event.touch.y))
		{
			if (!DownloadQueue::getInstance().isDownloading(m_appItem))
			{
				DownloadQueue::getInstance().addDownload(m_appItem);
				TweenEngine::Tween::to(m_textDownload, util3ds::TweenText::FILL_COLOR_RGB, 0.5f)
						.target(230, 20, 20)
						.start(m_tweenManager);

				Notification::spawn(_("Queued install: %s", m_appItem->getTitle().toAnsiString().c_str()));
			}
		}
		else if (m_textSleepDownload.getGlobalBounds().contains(event.touch.x, event.touch.y))
		{
			if (!DownloadQueue::getInstance().isDownloading(m_appItem))
			{
				m_appItem->queueForSleepInstall(false);
				TweenEngine::Tween::to(m_textSleepDownload, util3ds::TweenText::FILL_COLOR_RGB, 0.5f)
						.target(230, 20, 20)
						.start(m_tweenManager);
			}
		}
	}

	for (int i = 0; i < m_screenshotTops.size(); ++i)
	{
		if (m_screenshotTops[i]->getGlobalBounds().contains(event.touch.x, event.touch.y))
			setCurrentScreenshot(i+1);
		else if (m_screenshotBottoms[i]->getGlobalBounds().contains(event.touch.x, event.touch.y))
			setCurrentScreenshot(i+1);
	}

	if (event.type == cpp3ds::Event::JoystickMoved)
	{
		m_descriptionVelocity = event.joystickMove.y * 2.f;
	}

	return true;
}

void AppInfo::update(float delta)
{
//	if (!m_downloads.isDownloading(m_currentApp) && !m_downloads.isInstalling(m_currentApp))
//	{
//		m_textDownload.setFillColor(cpp3ds::Color::White);
//	}

	/*m_textDescription.move(0.f, m_descriptionVelocity * delta);
	if (m_textDescription.getPosition().y < 49.f - m_textDescription.getLocalBounds().height + 110.f)
		m_textDescription.setPosition(102.f, 49.f - m_textDescription.getLocalBounds().height + 110.f);
	if (m_textDescription.getPosition().y > 49.f)
		m_textDescription.setPosition(102.f, 49.f);*/

	m_scrollbar.update(delta);
	m_tweenManager.update(delta);

	m_appStats.update(delta);
}

void AppInfo::setDescription(const rapidjson::Value &jsonDescription)
{
	// Decode description, strip newlines
	const char *dd = jsonDescription.GetString();
	cpp3ds::String description = cpp3ds::String::fromUtf8(dd, dd + jsonDescription.GetStringLength());
	description.replace("\n", " ");
	description.replace("<br>", "\n");
	description.replace("<BR>", "\n");
	description.replace("<br/>", "\n");

	cpp3ds::String wrappedDesc = calculateWordWrapping(description);

	if (Theme::isTextThemed)
		m_textDescriptionDrawn << Theme::secondaryTextColor;
	else
		m_textDescriptionDrawn << cpp3ds::Color(100, 100, 100, 255);
	m_textDescription.setString(wrappedDesc);
	m_textDescriptionDrawn << wrappedDesc;
}

void AppInfo::setScreenshots(const rapidjson::Value &jsonScreenshots)
{
	if (jsonScreenshots.IsArray())
		for (int i = 0; i < jsonScreenshots.Size(); ++i)
		{
			LoadInformations::getInstance().setStatus(_("Loading screenshot #%i", i + 1));

			if (jsonScreenshots[i]["image_url"].Size() == 2) {
				addScreenshot(i, jsonScreenshots[i]["image_url"][0]);
				addScreenshot(i, jsonScreenshots[i]["image_url"][1]);
			} else {
				addScreenshot(i, jsonScreenshots[i]["image_url"][0]);

				addEmptyScreenshot(i, jsonScreenshots[i]["image_url"][0]["type"] == "lower");
			}
		}

	float startX = std::round((184.f - 61.f * m_screenshotTops.size()) / 2.f);
	for (int i = 0; i < m_screenshotTops.size(); ++i)
	{
		m_screenshotTops[i]->setPosition(startX + i * 61.f, 167.f);
		m_screenshotBottoms[i]->setPosition(startX + 6.f + i * 61.f, 203.f);
	}
}

void AppInfo::addScreenshot(int index, const rapidjson::Value &jsonScreenshot)
{
	std::string url = jsonScreenshot["value"].GetString();
	std::string type = jsonScreenshot["type"].GetString();
	std::string filename = _(FREESHOP_DIR "/tmp/%s/screen%d%s.jpg", m_appItem->getTitleIdStr().c_str(), index, type.c_str());
	bool isUpper = type == "upper";

	if (!pathExists(filename.c_str()))
	{
		Download download(url, filename);
		download.run();
	}

	std::unique_ptr<cpp3ds::Texture> texture(new cpp3ds::Texture());
	std::unique_ptr<cpp3ds::Sprite> sprite(new cpp3ds::Sprite());

	texture->loadFromFile(filename);
	texture->setSmooth(true);
	sprite->setTexture(*texture, true);
	//sprite->setScale(0.15f, 0.15f);
	sprite->setPosition(400.f, 0.f); // Keep offscreen

	// Calculate scale for screenshots that are not 400x240 (top screen) / 320x240 (bottom screen)
	float scaleX = isUpper ? 400.f / texture->getSize().x : 320.f / texture->getSize().x;
	float scaleY = 240.f / texture->getSize().y;

	sprite->setScale(0.15f * scaleX / 1.f, 0.15f * scaleY / 1.f);

	if (isUpper) {
		m_screenshotTops.emplace_back(std::move(sprite));
		m_screenshotTopTextures.emplace_back(std::move(texture));
	} else {
		m_screenshotBottoms.emplace_back(std::move(sprite));
		m_screenshotBottomTextures.emplace_back(std::move(texture));
	}
}

void AppInfo::addEmptyScreenshot(int index, bool isUpper)
{
	std::unique_ptr<cpp3ds::Texture> texture(new cpp3ds::Texture());
	std::unique_ptr<cpp3ds::Sprite> sprite(new cpp3ds::Sprite());

	cpp3ds::Image img;
	if (isUpper)
		img.create(400.f, 240.f, cpp3ds::Color(0, 0, 0, 128));
	else
		img.create(320.f, 240.f, cpp3ds::Color(0, 0, 0, 128));

	texture->loadFromImage(img);
	texture->setSmooth(true);
	sprite->setTexture(*texture, true);
	sprite->setScale(0.15f, 0.15f);
	sprite->setPosition(400.f, 0.f); // Keep offscreen

	if (isUpper) {
		m_screenshotTops.emplace_back(std::move(sprite));
		m_screenshotTopTextures.emplace_back(std::move(texture));
	} else {
		m_screenshotBottoms.emplace_back(std::move(sprite));
		m_screenshotBottomTextures.emplace_back(std::move(texture));
	}
}

void AppInfo::updateInfo()
{
	if (m_appItem->getDemos().size() > 0)
	{
		if (InstalledList::isInstalled(m_appItem->getDemos()[0])) {
			m_textIconDemo.setString(L"\uf1f8");
		} else {
			m_textIconDemo.setString(L"\uf019");
		}
	}
}

void AppInfo::setScroll(float position)
{
	m_scrollPos = position;
	m_textDescriptionDrawn.setPosition(2.f, std::round(position + 55.f));
}

float AppInfo::getScroll()
{
	return m_scrollPos;
}

const cpp3ds::Vector2f &AppInfo::getScrollSize()
{
	return m_scrollSize;
}

void AppInfo::addInfoToDescription(const rapidjson::Value &jsonTitle)
{
	// Get publisher from game's data and put it in description
	if (jsonTitle.HasMember("publisher")) {
		m_textDescriptionDrawn << "\n\n";
		if (Theme::isTextThemed)
  		m_textDescriptionDrawn << Theme::primaryTextColor;
		else
  		m_textDescriptionDrawn << cpp3ds::Color::Black;
		m_textDescriptionDrawn << _("Publisher").toAnsiString() << "\n";
		if (Theme::isTextThemed)
			m_textDescriptionDrawn << Theme::secondaryTextColor;
			else
			m_textDescriptionDrawn << cpp3ds::Color(100, 100, 100, 255);

		m_textDescriptionDrawn << jsonTitle["publisher"]["name"].GetString();
	}

	// Get number of player(s) from game's data and put it in description
	if (jsonTitle.HasMember("number_of_players")) {
		m_textDescriptionDrawn << "\n\n";
		if (Theme::isTextThemed)
  		m_textDescriptionDrawn << Theme::primaryTextColor;
		else
  		m_textDescriptionDrawn << cpp3ds::Color::Black;
		m_textDescriptionDrawn << _("Players").toAnsiString() << "\n";
		if (Theme::isTextThemed)
			m_textDescriptionDrawn << Theme::secondaryTextColor;
			else
			m_textDescriptionDrawn << cpp3ds::Color(100, 100, 100, 255);

		cpp3ds::String nbOfPlayers = jsonTitle["number_of_players"].GetString();
		nbOfPlayers.replace("\n", " ");
		nbOfPlayers.replace("<br>", "\n");
		nbOfPlayers.replace("<BR>", "\n");
		nbOfPlayers.replace("<br/>", "\n");

		m_textDescriptionDrawn << nbOfPlayers;
	}

	// Init var for genre(s)
	std::vector<std::string> vectorAppGenres;
	cpp3ds::String tempGenresStorage;

	// Get genre(s) from game's data
	if (jsonTitle.HasMember("genres")) {
		for (int i = 0; i < jsonTitle["genres"]["genre"].Size(); ++i) {
			vectorAppGenres.push_back(jsonTitle["genres"]["genre"][i]["name"].GetString());
		}
	}

	// Sort alphabetically the genre(s)
	std::sort(vectorAppGenres.begin(), vectorAppGenres.end(), [=](std::string& a, std::string& b) {
		return a < b;
	});

	// Put genre(s) in the string
	for (int i = 0; i < vectorAppGenres.size(); ++i)
		tempGenresStorage.insert(tempGenresStorage.getSize(), _("%s, ", vectorAppGenres[i]));

	// Remove the last comma
	if (tempGenresStorage.getSize() > 2)
		tempGenresStorage.erase(tempGenresStorage.getSize() - 2, 2);

	// Put genre(s) in description
	if (tempGenresStorage.getSize() > 0) {
		m_textDescriptionDrawn << "\n\n";
		if (Theme::isTextThemed)
  		m_textDescriptionDrawn << Theme::primaryTextColor;
		else
  		m_textDescriptionDrawn << cpp3ds::Color::Black;
		m_textDescriptionDrawn << _("Genre").toAnsiString() << "\n";
		if (Theme::isTextThemed)
			m_textDescriptionDrawn << Theme::secondaryTextColor;
		else
			m_textDescriptionDrawn << cpp3ds::Color(100, 100, 100, 255);

		m_textDescriptionDrawn << calculateWordWrapping(tempGenresStorage);
	}

	// Init var for language(s)
	std::vector<std::string> vectorAppLanguage;
	cpp3ds::String tempLanguageStorage;

	// Get language(s) from game's data
	if (jsonTitle.HasMember("languages")) {
		for (int i = 0; i < jsonTitle["languages"]["language"].Size(); ++i) {
			vectorAppLanguage.push_back(jsonTitle["languages"]["language"][i]["name"].GetString());
		}
	}

	// Sort alphabetically the language(s)
	std::sort(vectorAppLanguage.begin(), vectorAppLanguage.end(), [=](std::string& a, std::string& b) {
		return a < b;
	});

	// Put language(s) in the string
	for (int i = 0; i < vectorAppLanguage.size(); ++i)
		tempLanguageStorage.insert(tempLanguageStorage.getSize(), _("%s, ", vectorAppLanguage[i]));

	// Remove the last comma
	if (tempLanguageStorage.getSize() > 2)
		tempLanguageStorage.erase(tempLanguageStorage.getSize() - 2, 2);

	// Put language(s) in description
	if (tempLanguageStorage.getSize() > 0) {
		m_textDescriptionDrawn << "\n\n";
		if (Theme::isTextThemed)
  		m_textDescriptionDrawn << Theme::primaryTextColor;
		else
  		m_textDescriptionDrawn << cpp3ds::Color::Black;
		m_textDescriptionDrawn << _("Languages").toAnsiString() << "\n";
		if (Theme::isTextThemed)
			m_textDescriptionDrawn << Theme::secondaryTextColor;
		else
			m_textDescriptionDrawn << cpp3ds::Color(100, 100, 100, 255);

		m_textDescriptionDrawn << calculateWordWrapping(tempLanguageStorage);
	}

	// Init var for feature(s)
	std::vector<std::string> vectorAppFeatures;
	cpp3ds::String tempFeaturesStorage;

	// Get feature(s) from game's data
	if (jsonTitle.HasMember("features")) {
		for (int i = 0; i < jsonTitle["features"]["feature"].Size(); ++i) {
			vectorAppFeatures.push_back(jsonTitle["features"]["feature"][i]["name"].GetString());
		}
	}

	// Sort alphabetically the feature(s)
	std::sort(vectorAppFeatures.begin(), vectorAppFeatures.end(), [=](std::string& a, std::string& b) {
		return a < b;
	});

	// Put feature(s) in the string
	for (int i = 0; i < vectorAppFeatures.size(); ++i)
		tempFeaturesStorage.insert(tempFeaturesStorage.getSize(), _("%s, ", vectorAppFeatures[i]));

	// Remove the last comma
	if (tempFeaturesStorage.getSize() > 2)
		tempFeaturesStorage.erase(tempFeaturesStorage.getSize() - 2, 2);

	// Put feature(s) in description
	if (tempFeaturesStorage.getSize() > 0) {
		m_textDescriptionDrawn << "\n\n";
		if (Theme::isTextThemed)
  		m_textDescriptionDrawn << Theme::primaryTextColor;
		else
  		m_textDescriptionDrawn << cpp3ds::Color::Black;
		m_textDescriptionDrawn << _("Compatible Features/Accessories").toAnsiString() << "\n";
		if (Theme::isTextThemed)
			m_textDescriptionDrawn << Theme::secondaryTextColor;
		else
			m_textDescriptionDrawn << cpp3ds::Color(100, 100, 100, 255);

		m_textDescriptionDrawn << calculateWordWrapping(tempFeaturesStorage);
	}

	// Put indication to open AppStats in description
	m_textDescriptionDrawn << "\n\n";
	if (Theme::isTextThemed)
		m_textDescriptionDrawn << Theme::primaryTextColor;
	else
		m_textDescriptionDrawn << cpp3ds::Color::Black;
	m_textDescriptionDrawn << _("Press Select to open the Ratings screen").toAnsiString();
}

cpp3ds::String AppInfo::calculateWordWrapping(cpp3ds::String sentence)
{
	// Calculate word-wrapping
	int startPos = 0;
	int lastSpace = 0;
	auto s = sentence.toUtf8();
	cpp3ds::Text tmpText = m_textDescription;
	for (int i = 0; i < s.size(); ++i)
	{
		if (s[i] == ' ')
			lastSpace = i;
		tmpText.setString(cpp3ds::String::fromUtf8(s.begin() + startPos, s.begin() + i));
		if (tmpText.getLocalBounds().width > 300)
		{
			if (lastSpace != 0)
			{
				s[lastSpace] = '\n';
				i = startPos = lastSpace + 1;
				lastSpace = 0;
			}
			else
			{
				s.insert(s.begin() + i, '\n');
				startPos = ++i;
			}
		}
	}

	return cpp3ds::String::fromUtf8(s.begin(), s.end());
}

void AppInfo::setBanner(const rapidjson::Value &jsonBanner)
{
	std::string bannerURL = jsonBanner.GetString();
	std::string bannerPath = _(FREESHOP_DIR "/tmp/%s/banner.jpg", m_appItem->getTitleIdStr().c_str());

	Download download(bannerURL, bannerPath);
	download.setField("Accept", "application/json");
	download.run();

	m_isBannerLoaded = true;

	m_gameBannerTexture.loadFromFile(bannerPath);
	m_gameBanner.setPosition(200.f, 120.f);
	m_gameBanner.setOrigin(m_gameBannerTexture.getSize().x / 2, m_gameBannerTexture.getSize().y / 2);
	m_gameBanner.setColor(cpp3ds::Color(255, 255, 255, 0));
	m_gameBanner.setScale(1.5f, 1.5f);
	m_gameBanner.setTexture(m_gameBannerTexture, true);

	cpp3ds::IntRect textureRect;
	m_topIcon.setColor(cpp3ds::Color(255, 255, 255, 0));
	m_topIcon.setTexture(*m_appItem->getIcon(textureRect), true);
	m_topIcon.setTextureRect(textureRect);
	m_topIcon.setScale(.5f, .5f);
	m_topIcon.setOrigin(0.f, m_topIcon.getGlobalBounds().height / 2);

	TweenEngine::Tween::to(m_gameBanner, util3ds::TweenSprite::COLOR_ALPHA, 0.2f)
		.target(255.f)
		.start(m_tweenManager);
	TweenEngine::Tween::to(m_gameBanner, util3ds::TweenSprite::SCALE_XY, 0.2f)
		.target(1.f, 1.f)
		.start(m_tweenManager);

	TweenEngine::Tween::to(m_overlay, util3ds::TweenRectangleShape::FILL_COLOR_ALPHA, 0.2f)
		.target(200.f)
		.start(m_tweenManager);

	TweenEngine::Tween::to(m_topIcon, util3ds::TweenSprite::COLOR_ALPHA, 0.2f)
		.target(255.f)
		.delay(0.2f)
		.start(m_tweenManager);
}

void AppInfo::closeBanner()
{
	m_tweenManager.killAll();

	TweenEngine::Tween::to(m_gameBanner, util3ds::TweenSprite::COLOR_ALPHA, 0.2f)
		.target(0.f)
		.setCallback(TweenEngine::TweenCallback::COMPLETE, [&](TweenEngine::BaseTween* source) {
			m_isBannerLoaded = false;
		})
		.start(m_tweenManager);
	TweenEngine::Tween::to(m_gameBanner, util3ds::TweenSprite::SCALE_XY, 0.2f)
		.target(.8f, .8f)
		.start(m_tweenManager);

	TweenEngine::Tween::to(m_overlay, util3ds::TweenRectangleShape::FILL_COLOR_ALPHA, 0.2f)
		.target(0.f)
		.start(m_tweenManager);

	TweenEngine::Tween::to(m_topIcon, util3ds::TweenSprite::COLOR_ALPHA, 0.2f)
		.target(0.f)
		.start(m_tweenManager);
}

void AppInfo::uninstallGame()
{
	cpp3ds::String appTitle = m_appItem->getTitle();

	g_browseState->blockControls(true);
	setCanDraw(false);
	g_browseState->requestStackPush(States::Loading);

#ifdef _3DS
	FS_MediaType mediaType = ((m_appItem->getTitleId() >> 32) == TitleKeys::DSiWare) ? MEDIATYPE_NAND : MEDIATYPE_SD;
	AM_DeleteTitle(mediaType, m_appItem->getTitleId());
#endif
	m_appItem->setInstalled(false);
	Notification::spawn(_("Deleted: %s", appTitle.toAnsiString().c_str()));

	g_browseState->requestStackPop();
	g_browseState->blockControls(false);
	setCanDraw(true);

	InstalledList::getInstance().refresh();
	updateInfo();
}

void AppInfo::uninstallDemo()
{
	cpp3ds::Uint64 demoTitleId = m_appItem->getDemos()[0];
	cpp3ds::String appTitle = m_appItem->getTitle();

	g_browseState->blockControls(true);
	setCanDraw(false);
	g_browseState->requestStackPush(States::Loading);

#ifdef _3DS
	AM_DeleteTitle(MEDIATYPE_SD, demoTitleId);
#endif
	Notification::spawn(_("Deleted demo: %s", appTitle.toAnsiString().c_str()));

	g_browseState->requestStackPop();
	g_browseState->blockControls(false);
	setCanDraw(true);

	InstalledList::getInstance().refresh();
	updateInfo();
}

void AppInfo::setCanDraw(bool canDraw)
{
	m_canDraw = canDraw;
}

bool AppInfo::canDraw()
{
	return m_canDraw;
}

} // namespace FreeShop
