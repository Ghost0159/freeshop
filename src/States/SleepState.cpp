#include "SleepState.hpp"
#include "../Config.hpp"
#include <cpp3ds/Window/Window.hpp>
#include <cpp3ds/System/I18n.hpp>
#include <TweenEngine/Tween.h>
#ifndef EMULATION
#include <3ds.h>
#include "../MCU/Mcu.hpp"
#endif

namespace FreeShop {

bool SleepState::isSleeping = false;
cpp3ds::Clock SleepState::clock;

SleepState::SleepState(StateStack& stack, Context& context, StateCallback callback)
: State(stack, context, callback)
, m_sleepEnding(false)
{
#ifndef EMULATION
	if (R_SUCCEEDED(gspLcdInit()))
	{
		if (Config::get(Config::SleepMode).GetBool())
			GSPLCD_PowerOffBacklight(GSPLCD_SCREEN_TOP);

		if (Config::get(Config::SleepModeBottom).GetBool())
			GSPLCD_PowerOffBacklight(GSPLCD_SCREEN_BOTTOM);

		gspLcdExit();
	}

	if (Config::get(Config::DimLEDs).GetBool()) {
		if (R_SUCCEEDED(MCU::getInstance().mcuInit())) {
			MCU::getInstance().dimLeds(0x09);
			MCU::getInstance().mcuExit();
		}
	}
#endif
	isSleeping = true;

	m_overlay.setSize(cpp3ds::Vector2f(400.f, 240.f));
	m_overlay.setFillColor(cpp3ds::Color::Transparent);

	TweenEngine::Tween::to(m_overlay, util3ds::TweenRectangleShape::FILL_COLOR_ALPHA, 0.5f)
		.target(200.f)
		.start(m_tweenManager);
}

SleepState::~SleepState()
{
#ifndef EMULATION
	if (R_SUCCEEDED(gspLcdInit()))
	{
		GSPLCD_PowerOnBacklight(GSPLCD_SCREEN_BOTH);
		gspLcdExit();
	}

	if (Config::get(Config::DimLEDs).GetBool()) {
		if (R_SUCCEEDED(MCU::getInstance().mcuInit())) {
			MCU::getInstance().dimLeds(0xFF);
			MCU::getInstance().mcuExit();
		}
	}
#endif
	isSleeping = false;
}

void SleepState::renderTopScreen(cpp3ds::Window& window)
{
	window.draw(m_overlay);
}

void SleepState::renderBottomScreen(cpp3ds::Window& window)
{
	window.draw(m_overlay);
}

bool SleepState::update(float delta)
{
	if (!isSleeping)
	{
		requestStackPop();
		clock.restart();
	}
	m_tweenManager.update(delta);
	return true;
}

bool SleepState::processEvent(const cpp3ds::Event& event)
{
	if (event.type == cpp3ds::Event::SliderVolumeChanged)
		return false;
	if (m_sleepEnding)
		return false;

	m_sleepEnding = true;

	TweenEngine::Tween::to(m_overlay, util3ds::TweenRectangleShape::FILL_COLOR_ALPHA, 0.5f)
		.target(0.f)
		.setCallback(TweenEngine::TweenCallback::COMPLETE, [&](TweenEngine::BaseTween* source) {
			requestStackPop();
			clock.restart();
		})
		.start(m_tweenManager);

	return true;
}

} // namespace FreeShop
