#ifndef FREESHOP_TOPINFORMATIONS_HPP
#define FREESHOP_TOPINFORMATIONS_HPP

#include <cpp3ds/Graphics/Drawable.hpp>
#include <cpp3ds/Graphics/Text.hpp>
#include <cpp3ds/Window/Event.hpp>
#include <cpp3ds/System/Clock.hpp>
#include "TweenObjects.hpp"
#include "TitleKeys.hpp"
#include <TweenEngine/Tween.h>
#include <TweenEngine/TweenManager.h>
#ifndef EMULATION
#include <3ds.h>
#endif

namespace FreeShop {

class TopInformations : public cpp3ds::Drawable, public util3ds::TweenTransformable<cpp3ds::Transformable> {
public:
	void update(float delta);

	void setCollapsed(bool collapsed);
	void setTextMode(int newMode);
	void setModeChangeEnabled(bool newMode);
	void resetModeTimer();
	void wokeUp();

	TopInformations();
	~TopInformations();

protected:
	virtual void draw(cpp3ds::RenderTarget& target, cpp3ds::RenderStates states) const;

private:
	util3ds::TweenText m_textClock;
	util3ds::TweenText m_textTwoPoints;

	cpp3ds::Texture m_textureBattery;
	util3ds::TweenSprite m_batteryIcon;
	cpp3ds::Texture m_textureSignal;
	util3ds::TweenSprite m_signalIcon;

	cpp3ds::Clock m_switchClock;
	cpp3ds::Clock m_updateClock;
	cpp3ds::Clock m_wokeUpClock;
	int m_textClockMode;

	int m_batteryPercent;

	bool m_isCollapsed;
	bool m_isTransitioning;
	bool m_canTransition;

	TweenEngine::TweenManager m_tweenManager;

	void updateIcons(std::string timeTextFmt);

	bool m_lowBatteryNotified;
	bool m_noInternetNotified;
	bool m_justWokeUp;

#ifndef EMULATION
	Result PTMU_GetAdapterState(u8 *out);
	Result MCUHWC_GetBatteryLevel(u8 *out);
#endif

};

} // namespace FreeShop

#endif // FREESHOP_TOPINFORMATIONS_HPP
