#include "AppStats.hpp"
#include "AssetManager.hpp"
#include "Theme.hpp"
#include "Util.hpp"
#include "Config.hpp"
#include "AppInfo.hpp"
#include "States/StateIdentifiers.hpp"
#include "States/DialogState.hpp"
#include "States/SyncState.hpp"
#include <cpp3ds/System/I18n.hpp>
#include <cpp3ds/System/FileSystem.hpp>
#include <TweenEngine/Tween.h>

namespace FreeShop {

AppStats::AppStats()
: m_appItem(nullptr)
, m_isVisible(false)
, m_votes(0)
, m_finalScore(0.f)
, m_percentageGamers(0)
, m_percentageEveryone(0)
, m_percentageCasual(0)
, m_percentageIntense(0)
{
	m_overlay.setSize(cpp3ds::Vector2f(400.f, 240.f));
	m_overlay.setFillColor(cpp3ds::Color(0, 0, 0, 0));

	if (Theme::isTextThemed)
		m_topBackground.setOutlineColor(Theme::boxOutlineColor);
	else
		m_topBackground.setOutlineColor(cpp3ds::Color(158, 158, 158, 255));
	m_topBackground.setOutlineThickness(1);
	if (Theme::isTextThemed)
		m_topBackground.setFillColor(Theme::boxColor);
	else
		m_topBackground.setFillColor(cpp3ds::Color(245, 245, 245));
	m_topBackground.setSize(cpp3ds::Vector2f(400.f, 116.f));
	m_topBackground.setPosition(0.f, 124.f);

	m_botBackground = m_topBackground;
	m_botBackground.setSize(cpp3ds::Vector2f(320.f, 165.f));
	m_botBackground.setPosition(0.f, 0.f);

	m_statText.setPosition(200.f, 136.f);
	if (Theme::isTextThemed)
		m_statText.setFillColor(Theme::primaryTextColor);
	else
		m_statText.setFillColor(cpp3ds::Color::Black);
	m_statText.setCharacterSize(16);
	m_statText.setString(_("Ratings"));
	m_statText.setOrigin(std::round(m_statText.getLocalBounds().width / 2), std::round(m_statText.getLocalBounds().height / 2));

	m_preferenceText = m_statText;
	m_preferenceText.setString(_("Preferences"));
	m_preferenceText.setOrigin(std::round(m_preferenceText.getLocalBounds().width / 2), std::round(m_preferenceText.getLocalBounds().height / 2));
	m_preferenceText.setPosition(160.f, 15.f);

	m_stars.resize(5);

	m_starsText.resize(5);
	m_ratings.resize(5);
	m_progressRatings.resize(5);
	m_progressRatingsBackground.resize(5);

	for (int i = 0; i < 5; ++i) {
		util3ds::TweenText starText;
		starText.setFillColor(cpp3ds::Color(66, 66, 66));
		starText.setCharacterSize(12);
		starText.setFont(AssetManager<cpp3ds::Font>::get("fonts/fontawesome.ttf"));
		starText.setPosition(5.f, 220.f - 16.f * i);

		std::string starTextString;
		for (int j = 0; j <= i; ++j)
			starTextString += "\uf005";

		for (int j = i; j < 4; ++j)
			starTextString += "\uf006";

		starText.setString(starTextString);

		m_starsText[i] = starText;

		util3ds::TweenRectangleShape barRatingBackground;
		barRatingBackground.setFillColor(cpp3ds::Color(158, 158, 158, 150));
		barRatingBackground.setPosition(70.f, 226.f - 16.f * i);
		barRatingBackground.setSize(cpp3ds::Vector2f(115.f, 5.f));
		m_progressRatingsBackground[i] = barRatingBackground;
	}

	m_finalScoreText.setCharacterSize(32);
	m_finalScoreText.setPosition(395.f, 191.f);
	if (Theme::isTextThemed)
		m_finalScoreText.setFillColor(Theme::primaryTextColor);
	else
		m_finalScoreText.setFillColor(cpp3ds::Color::Black);

	m_voteCountUserIcon.setCharacterSize(11);
	m_voteCountUserIcon.setFont(AssetManager<cpp3ds::Font>::get("fonts/fontawesome.ttf"));
	m_voteCountUserIcon.setString("\uf007");
	m_voteCountUserIcon.setOrigin(m_voteCountUserIcon.getLocalBounds().width, 0);
	m_voteCountUserIcon.setPosition(395.f, 131.f);
	if (Theme::isTextThemed)
		m_voteCountUserIcon.setFillColor(Theme::secondaryTextColor);
	else
		m_voteCountUserIcon.setFillColor(cpp3ds::Color(66, 66, 66));

	m_voteCount.setCharacterSize(12);
	m_voteCount.setPosition(m_voteCountUserIcon.getPosition().x - m_voteCountUserIcon.getLocalBounds().width - 5.f, m_voteCountUserIcon.getPosition().y);
	if (Theme::isTextThemed)
		m_voteCount.setFillColor(Theme::secondaryTextColor);
	else
		m_voteCount.setFillColor(cpp3ds::Color(66, 66, 66));

	m_textAverageScore.setCharacterSize(12);
	m_textAverageScore.setString(_("Average score"));
	m_textAverageScore.setPosition(395.f, 191.f - m_textAverageScore.getLocalBounds().height - 5.f);
	m_textAverageScore.setOrigin(m_textAverageScore.getLocalBounds().width, 0.f);
	if (Theme::isTextThemed)
		m_textAverageScore.setFillColor(Theme::secondaryTextColor);
	else
		m_textAverageScore.setFillColor(cpp3ds::Color(66, 66, 66));

	m_textPlayerType.setCharacterSize(10);
	m_textPlayerType.setString(_("• Who do you think this title appeals to?"));
	m_textPlayerType.setPosition(5.f, 40.f);
	if (Theme::isTextThemed)
		m_textPlayerType.setFillColor(Theme::secondaryTextColor);
	else
		m_textPlayerType.setFillColor(cpp3ds::Color(66, 66, 66));

	m_textPlayStyle = m_textPlayerType;
	m_textPlayStyle.setString(_("• What kind of play style is this title more suited for?"));
	m_textPlayStyle.setPosition(5.f, 99.f);

	m_barEveryone.setFillColor(cpp3ds::Color(3, 169, 244));
	m_barEveryone.setPosition(20.f, 84.f);

	m_barGamers.setFillColor(cpp3ds::Color(255, 152, 0));
	m_barGamers.setPosition(20.f, 84.f);
	m_barGamers.setSize(cpp3ds::Vector2f(280.f, 5.f));

	m_barCasual.setFillColor(cpp3ds::Color(3, 169, 244));
	m_barCasual.setPosition(20.f, 143.f);

	m_barIntense.setFillColor(cpp3ds::Color(255, 152, 0));
	m_barIntense.setPosition(20.f, 143.f);
	m_barIntense.setSize(cpp3ds::Vector2f(280.f, 5.f));

	m_textEveryone.setCharacterSize(12);
	m_textEveryone.setPosition(20.f, 64.f);
	m_textEveryone.setFillColor(cpp3ds::Color(3, 169, 244));

	m_textGamers.setCharacterSize(12);
	m_textGamers.setPosition(300.f, 64.f);
	m_textGamers.setFillColor(cpp3ds::Color(255, 152, 0));

	m_textCasual = m_textEveryone;
	m_textCasual.setPosition(20.f, 123.f);

	m_textIntense = m_textGamers;
	m_textIntense.setPosition(300.f, 123.f);

	m_topView.setCenter(cpp3ds::Vector2f(200.f, 3.f));
	m_topView.setSize(cpp3ds::Vector2f(400.f, 240.f));

	m_bottomView.setCenter(cpp3ds::Vector2f(160.f, 286.f));
	m_bottomView.setSize(cpp3ds::Vector2f(320.f, 240.f));
}

AppStats::~AppStats()
{

}

void AppStats::draw(cpp3ds::RenderTarget &target, cpp3ds::RenderStates states) const
{
	if (!m_isVisible)
		return;

	target.draw(m_overlay);

	target.setView(m_bottomView);

	target.draw(m_botBackground);
	target.draw(m_preferenceText);

	// Type of player and play style stuff
	target.draw(m_textPlayerType);
	target.draw(m_textPlayStyle);

	target.draw(m_barGamers);
	target.draw(m_barEveryone);
	target.draw(m_barIntense);
	target.draw(m_barCasual);

	target.draw(m_textEveryone);
	target.draw(m_textGamers);
	target.draw(m_textCasual);
	target.draw(m_textIntense);

	target.setView(target.getDefaultView());
}

void AppStats::drawTop(cpp3ds::Window &window)
{
	if (!m_isVisible)
		return;

	window.draw(m_overlay);

	window.setView(m_topView);

	window.draw(m_topBackground);
	window.draw(m_statText);

	window.draw(m_textAverageScore);

	// Rating related stuff
	for (int i = 0; i < 5; ++i) {
		window.draw(m_ratings.at(i));
		window.draw(m_starsText.at(i));
		window.draw(m_progressRatingsBackground.at(i));
		window.draw(m_progressRatings.at(i));
	}

	window.draw(m_finalScoreText);
	window.draw(m_voteCount);
	window.draw(m_voteCountUserIcon);

	window.setView(window.getDefaultView());
}

void AppStats::update(float delta)
{
	m_tweenManager.update(delta);
}

bool AppStats::processEvent(const cpp3ds::Event &event)
{
	if (isVisible()) {
		if (event.type == cpp3ds::Event::KeyPressed && (event.key.code == cpp3ds::Keyboard::Select || event.key.code == cpp3ds::Keyboard::B))
			setVisible(false);
		else if (event.type == cpp3ds::Event::TouchEnded)
			setVisible(false);
	} else {
		if (event.type == cpp3ds::Event::KeyReleased && event.key.code == cpp3ds::Keyboard::Select)
			setVisible(true);
	}

#ifdef EMULATION
	if (event.type == cpp3ds::Event::KeyPressed && event.key.code == cpp3ds::Keyboard::Y)
		setVisible(!isVisible());
#endif
}

void AppStats::loadApp(std::shared_ptr<AppItem> appItem, const rapidjson::Value &jsonStarRating)
{
	m_appItem = appItem;

	// Rating related stuff
	if (jsonStarRating.HasMember("votes"))
		m_votes = jsonStarRating["votes"].GetInt();
	else
		m_votes = 0;

	m_voteCount.setString(_("%i", m_votes));
	m_voteCount.setOrigin(std::round(m_voteCount.getLocalBounds().width / 2), 0);

	if (jsonStarRating.HasMember("score"))
		m_finalScore = jsonStarRating["score"].GetDouble();
	else
		m_finalScore = 0;

	m_finalScoreText.setString(_("%.2f", m_finalScore));
	m_finalScoreText.setOrigin(m_finalScoreText.getLocalBounds().width, 0.f);

	for (int i = 0; i < 5; ++i) {
		util3ds::TweenText ratingText;
		ratingText.setCharacterSize(12);
		ratingText.setPosition(200.f, 221.f - 16.f * i);
		if (Theme::isTextThemed)
			ratingText.setFillColor(Theme::secondaryTextColor);
		else
			ratingText.setFillColor(cpp3ds::Color(66, 66, 66));

		util3ds::TweenRectangleShape barRating;
		barRating.setFillColor(cpp3ds::Color(158, 158, 158, 225));
		barRating.setPosition(70.f, 226.f - 16.f * i);

		if (jsonStarRating.HasMember(_("star%i", i + 1).toAnsiString().c_str())) {
			auto actualStar = _("star%i", i + 1).toAnsiString().c_str();
			int actualStarCount = jsonStarRating[actualStar].GetInt();

			m_stars[i] = actualStarCount;

			ratingText.setString(_("%i", actualStarCount));
			barRating.setSize(cpp3ds::Vector2f(actualStarCount * 115.f / m_votes, 5.f));
		} else {
			m_stars[i] = 0;

			ratingText.setString(_("%i", 0));
			barRating.setSize(cpp3ds::Vector2f(0.f, 5.f));
		}

		m_ratings[i] = ratingText;
		m_progressRatings[i] = barRating;
	}

	m_voteCount.setOrigin(m_voteCount.getLocalBounds().width, 0);
}

void AppStats::setVisible(bool isVisible)
{
	m_tweenManager.killAll();

	if (isVisible) {
		m_isVisible = isVisible;

		TweenEngine::Tween::to(m_overlay, m_overlay.FILL_COLOR_ALPHA, 0.2f)
			.target(200.f)
			.start(m_tweenManager);

		TweenEngine::Tween::to(m_topView, m_topView.CENTER_Y, 0.4f)
			.target(120.f)
			.delay(0.2f)
			.start(m_tweenManager);

		TweenEngine::Tween::to(m_bottomView, m_bottomView.CENTER_Y, 0.4f)
			.target(120.f)
			.delay(0.2f)
			.start(m_tweenManager);

	} else {
		TweenEngine::Tween::to(m_overlay, m_overlay.FILL_COLOR_ALPHA, 0.4f)
			.target(0.f)
			.setCallback(TweenEngine::TweenCallback::COMPLETE, [=](TweenEngine::BaseTween* source) {
				m_isVisible = isVisible;
			})
			.start(m_tweenManager);

		TweenEngine::Tween::to(m_topView, m_topView.CENTER_Y, 0.2f)
			.target(3.f)
			.start(m_tweenManager);

		TweenEngine::Tween::to(m_bottomView, m_bottomView.CENTER_Y, 0.2f)
			.target(286.f)
			.start(m_tweenManager);
	}
}

bool AppStats::isVisible()
{
	return m_isVisible;
}

void AppStats::loadPreferences(const rapidjson::Value &jsonPreferences)
{
	// Type of player and play style stuff
	if (jsonPreferences.HasMember("target_player")) {
		if (jsonPreferences["target_player"].HasMember("everyone"))
			m_percentageEveryone = jsonPreferences["target_player"]["everyone"].GetInt();
		else
			m_percentageEveryone = 0;

		if (jsonPreferences["target_player"].HasMember("gamers"))
			m_percentageGamers = jsonPreferences["target_player"]["gamers"].GetInt();
		else
			m_percentageGamers = 0;
	}

	if (jsonPreferences.HasMember("play_style")) {
		if (jsonPreferences["play_style"].HasMember("casual"))
			m_percentageCasual = jsonPreferences["play_style"]["casual"].GetInt();
		else
			m_percentageCasual = 0;

		if (jsonPreferences["play_style"].HasMember("intense"))
			m_percentageIntense = jsonPreferences["play_style"]["intense"].GetInt();
		else
			m_percentageIntense = 0;
	}

	m_barEveryone.setSize(cpp3ds::Vector2f(m_percentageEveryone * 280.f / 100.f, 5.f));
	m_barCasual.setSize(cpp3ds::Vector2f(m_percentageCasual * 280.f / 100.f, 5.f));

	m_textEveryone.setString(_("Everyone - %i%%", m_percentageEveryone));
	m_textCasual.setString(_("Casual - %i%%", m_percentageCasual));

	m_textGamers.setString(_("%i%% - Gamers", m_percentageGamers));
	m_textGamers.setOrigin(m_textGamers.getLocalBounds().width, 0);
	m_textIntense.setString(_("%i%% - Intense", m_percentageIntense));
	m_textIntense.setOrigin(m_textIntense.getLocalBounds().width, 0);
}

}
