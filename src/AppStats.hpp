#ifndef FREESHOP_APPSTATS_HPP
#define FREESHOP_APPSTATS_HPP

#include <cpp3ds/Graphics/Drawable.hpp>
#include <cpp3ds/Graphics/Text.hpp>
#include <cpp3ds/Window/Event.hpp>
#include <cpp3ds/System/Clock.hpp>
#include "TweenObjects.hpp"
#include "TitleKeys.hpp"
#include "AppItem.hpp"
#include <TweenEngine/Tween.h>
#include <TweenEngine/TweenManager.h>
#include <memory>
#ifndef EMULATION
#include <3ds.h>
#endif

namespace FreeShop {

class AppStats : public cpp3ds::Drawable, public util3ds::TweenTransformable<cpp3ds::Transformable> {
public:
	void drawTop(cpp3ds::Window &window);

	bool processEvent(const cpp3ds::Event& event);
	void update(float delta);

	void loadApp(std::shared_ptr<AppItem> appItem, const rapidjson::Value &jsonStarRating);
	void loadPreferences(const rapidjson::Value &jsonPreferences);

	void setVisible(bool isVisible);
	bool isVisible();

	AppStats();
	~AppStats();

protected:
	virtual void draw(cpp3ds::RenderTarget& target, cpp3ds::RenderStates states) const;

private:
	bool m_isVisible;

	util3ds::TweenRectangleShape m_overlay;
	util3ds::TweenRectangleShape m_topBackground;
	util3ds::TweenRectangleShape m_botBackground;

	util3ds::TweenableView m_topView;
	util3ds::TweenableView m_bottomView;

	// Top screen
	util3ds::TweenText m_statText;

	// Rating related stuff
	std::vector<int> m_stars;

	std::vector<util3ds::TweenText> m_starsText;
	std::vector<util3ds::TweenText> m_ratings;
	std::vector<util3ds::TweenRectangleShape> m_progressRatings;
	std::vector<util3ds::TweenRectangleShape> m_progressRatingsBackground;
	int m_votes;
	float m_finalScore;
	util3ds::TweenText m_finalScoreText;
	util3ds::TweenText m_textAverageScore;
	util3ds::TweenText m_voteCount;
	util3ds::TweenText m_voteCountUserIcon;

	// Bottom screen
	util3ds::TweenText m_preferenceText;

	// Type of player and play style stuff
	util3ds::TweenText m_textPlayerType;
	util3ds::TweenRectangleShape m_barEveryone;
	util3ds::TweenRectangleShape m_barGamers;
	util3ds::TweenText m_textEveryone;
	util3ds::TweenText m_textGamers;
	int m_percentageEveryone;
	int m_percentageGamers;

	util3ds::TweenText m_textPlayStyle;
	util3ds::TweenRectangleShape m_barCasual;
	util3ds::TweenRectangleShape m_barIntense;
	util3ds::TweenText m_textCasual;
	util3ds::TweenText m_textIntense;
	int m_percentageCasual;
	int m_percentageIntense;

	std::shared_ptr<AppItem> m_appItem;

	TweenEngine::TweenManager m_tweenManager;
};

} // namespace FreeShop

#endif // FREESHOP_APPSTATS_HPP
