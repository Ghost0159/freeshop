#include "TopInformations.hpp"
#include "AssetManager.hpp"
#include "DownloadQueue.hpp"
#include "Notification.hpp"
#include "Theme.hpp"
#include "Util.hpp"
#include "Config.hpp"
#include "States/StateIdentifiers.hpp"
#include "States/DialogState.hpp"
#include "States/SyncState.hpp"
#include <cpp3ds/System/I18n.hpp>
#include <cpp3ds/System/FileSystem.hpp>
#include <TweenEngine/Tween.h>
#include <time.h>
#include <stdlib.h>

namespace FreeShop {

TopInformations::TopInformations()
: m_batteryPercent(-1)
, m_textClockMode(1)
, m_isCollapsed(true)
, m_isTransitioning(false)
, m_canTransition(false)
, m_lowBatteryNotified(false)
, m_noInternetNotified(false)
, m_justWokeUp(false)
{
	//Start the clock
	m_switchClock.restart();
	m_updateClock.restart();

	//Get the time to show it in the top part of the App List
	time_t t = time(NULL);
	struct tm * timeinfo;
	timeinfo = localtime(&t);

	char timeTextFmt[12];
	char tempSec[3];
	strftime(tempSec, 3, "%S", timeinfo);

	strftime(timeTextFmt, 12, "%H %M", timeinfo);

	m_textClock.setString(timeTextFmt);
	m_textClock.useSystemFont();
	m_textClock.setCharacterSize(14);
	if (Theme::isTextThemed)
		m_textClock.setFillColor(Theme::primaryTextColor);
	else
		m_textClock.setFillColor(cpp3ds::Color(80, 80, 80, 255));

	//Battery icon
	if (fopen(FREESHOP_DIR "/theme/images/battery0.png", "rb"))
		m_textureBattery.loadFromFile(FREESHOP_DIR "/theme/images/battery0.png");
	else
		m_textureBattery.loadFromFile("images/battery0.png");

	m_batteryIcon.setPosition(370.f - m_textureBattery.getSize().x, 5.f);
	m_batteryIcon.setTexture(m_textureBattery, true);

	//Signal icon
	if (fopen(FREESHOP_DIR "/theme/images/wifi0.png", "rb"))
		m_textureSignal.loadFromFile(FREESHOP_DIR "/theme/images/wifi0.png");
	else
		m_textureSignal.loadFromFile("images/wifi0.png");

	m_signalIcon.setPosition(-50.f, 5.f);
	m_signalIcon.setTexture(m_textureSignal, true);

	//Define texts position
	m_textClock.setPosition(308.f - (m_textureBattery.getSize().x + m_textClock.getLocalBounds().width), -50.f);

	//Two points in clock
	m_textTwoPoints = m_textClock;
	m_textTwoPoints.setString(":");
	m_textTwoPoints.setCharacterSize(14);
	m_textTwoPoints.setPosition((m_textClock.getPosition().x + (m_textClock.getLocalBounds().width / 2)) - 3, m_textClock.getPosition().y);

#define TWEEN_IN(obj, posY) \
	TweenEngine::Tween::to(obj, obj.POSITION_Y, 0.6f).target(posY).delay(0.5f).start(m_tweenManager);

#define TWEEN_IN_NOWAIT(obj, posY) \
	TweenEngine::Tween::to(obj, obj.POSITION_Y, 0.6f).target(posY).start(m_tweenManager);

#define TWEEN_IN_X(obj, posX) \
	TweenEngine::Tween::to(obj, obj.POSITION_X, 0.6f).target(posX).delay(0.5f).start(m_tweenManager);

#define TWEEN_IN_X_NOWAIT(obj, posX) \
	TweenEngine::Tween::to(obj, obj.POSITION_X, 0.6f).target(posX).start(m_tweenManager);

	TweenEngine::Tween::to(m_textClock, m_textClock.POSITION_Y, 0.6f).target(5.f).delay(0.5f).setCallback(TweenEngine::TweenCallback::COMPLETE, [this](TweenEngine::BaseTween* source) {m_isCollapsed = false;}).start(m_tweenManager);
	TWEEN_IN(m_textTwoPoints, 5.f);
	TWEEN_IN_X(m_batteryIcon, 318.f - m_textureBattery.getSize().x);
	TWEEN_IN_X(m_signalIcon, 2.f);

	//Two points animation
	TweenEngine::Tween::to(m_textTwoPoints, util3ds::TweenText::FILL_COLOR_ALPHA, 1.f).target(0).repeatYoyo(-1, 0).start(m_tweenManager);
}

TopInformations::~TopInformations()
{

}

void TopInformations::draw(cpp3ds::RenderTarget &target, cpp3ds::RenderStates states) const
{
	states.transform *= getTransform();

	//Draw clock
	target.draw(m_textClock);

	//Draw only the two points if the clock text is in clock mode
	if (m_textClockMode == 1 && !m_isTransitioning)
		target.draw(m_textTwoPoints);

	//Draw battery & text
	target.draw(m_batteryIcon);

	//Draw signal
	target.draw(m_signalIcon);
}

void TopInformations::update(float delta)
{
	//Update the time to show it in the top part of the App List
	time_t t = time(NULL);
	struct tm * timeinfo;
	timeinfo = localtime(&t);

	char timeTextFmt[12];
	char tempSec[3];
	strftime(tempSec, 3, "%S", timeinfo);

	strftime(timeTextFmt, 12, "%H %M", timeinfo);

	m_tweenManager.update(delta);

	if (m_wokeUpClock.getElapsedTime() >= cpp3ds::seconds(10) && m_justWokeUp)
		m_justWokeUp = false;

	//Update battery and signal icons
	if (m_updateClock.getElapsedTime() >= cpp3ds::seconds(2)) {
		m_updateClock.restart();
		updateIcons(std::string(timeTextFmt));
	}
}

void TopInformations::setCollapsed(bool collapsed)
{
	if (collapsed)
		m_isCollapsed = collapsed;

	if (collapsed) {
		TweenEngine::Tween::to(m_textClock, m_textClock.POSITION_Y, 0.6f).target(-50.f).setCallback(TweenEngine::TweenCallback::COMPLETE, [this](TweenEngine::BaseTween* source) {m_isCollapsed = true;}).start(m_tweenManager);
		TWEEN_IN_NOWAIT(m_textTwoPoints, -50.f);
		TWEEN_IN_X_NOWAIT(m_batteryIcon, 370.f - m_textureBattery.getSize().x);
		TWEEN_IN_X_NOWAIT(m_signalIcon, -50.f);
	} else {
		TweenEngine::Tween::to(m_textClock, m_textClock.POSITION_Y, 0.6f).target(5.f).delay(0.5f).setCallback(TweenEngine::TweenCallback::COMPLETE, [this](TweenEngine::BaseTween* source) {m_isCollapsed = false;}).start(m_tweenManager);
		TWEEN_IN(m_textTwoPoints, 5.f);
		TWEEN_IN_X(m_batteryIcon, 318.f - m_textureBattery.getSize().x);
		TWEEN_IN_X(m_signalIcon, 2.f);
	}
}

void TopInformations::updateIcons(std::string timeTextFmt)
{
#ifndef EMULATION
	//Update battery icon and percentage
	cpp3ds::Uint8 batteryChargeState = 0;
	cpp3ds::Uint8 isAdapterPlugged = 0;
	cpp3ds::Uint8 batteryLevel = 0;
	cpp3ds::Uint8 batteryPercentHolder = 0;
	std::string batteryPath;

	if (R_SUCCEEDED(PTMU_GetBatteryChargeState(&batteryChargeState)) && batteryChargeState) {
    batteryPath = "battery_charging.png";
		m_lowBatteryNotified = false;
  } else if (R_SUCCEEDED(PTMU_GetAdapterState(&isAdapterPlugged)) && isAdapterPlugged) {
    batteryPath = "battery_charging_full.png";
		m_lowBatteryNotified = false;
	} else if (R_SUCCEEDED(PTMU_GetBatteryLevel(&batteryLevel))) {
  	batteryPath = "battery" + std::to_string(batteryLevel - 1) + ".png";

		if (batteryLevel - 1 == 1 && !m_lowBatteryNotified) {
			Notification::spawn(_("Battery power is running low"));
			m_lowBatteryNotified = true;
		}
	}
	else
  	batteryPath = "battery0.png";

 	//Update battery percentage
 	if (R_SUCCEEDED(MCUHWC_GetBatteryLevel(&batteryPercentHolder)))
 			m_batteryPercent = static_cast<int>(batteryPercentHolder);

	std::string themedBatteryPath = cpp3ds::FileSystem::getFilePath(FREESHOP_DIR "/theme/images/" + batteryPath);

	if (pathExists(themedBatteryPath.c_str(), false))
		m_textureBattery.loadFromFile(themedBatteryPath);
	else
		m_textureBattery.loadFromFile("images/" + batteryPath);

	//Update signal icon
	uint32_t wifiStatus = 0;
	std::string signalPath;

	if (R_SUCCEEDED(ACU_GetWifiStatus(&wifiStatus)) && wifiStatus) {
  	signalPath = "wifi" + std::to_string(osGetWifiStrength()) + ".png";

		m_noInternetNotified = false;
	} else {
    signalPath = "wifi_disconnected.png";

		if (!m_noInternetNotified && !m_justWokeUp) {
			m_noInternetNotified = true;
			Notification::spawn(_("The console is disconnected from the internet"));
		}
	}

	std::string themedSignalPath = cpp3ds::FileSystem::getFilePath(FREESHOP_DIR "/theme/images/" + signalPath);

	if (pathExists(themedSignalPath.c_str(), false))
		m_textureSignal.loadFromFile(themedSignalPath);
	else
		m_textureSignal.loadFromFile("images/" + signalPath);
#else
	//Update battery icon
	std::string batteryPath = "battery" + std::to_string(rand() % 5) + ".png";
	std::string themedBatteryPath = cpp3ds::FileSystem::getFilePath(FREESHOP_DIR "/theme/images/" + batteryPath);

	m_batteryPercent = rand() % 101;

	if (pathExists(themedBatteryPath.c_str(), false))
		m_textureBattery.loadFromFile(themedBatteryPath);
	else
		m_textureBattery.loadFromFile("images/" + batteryPath);

	//Update signal icon
	std::string signalPath = "wifi" + std::to_string(rand() % 4) + ".png";
	std::string themedSignalPath = cpp3ds::FileSystem::getFilePath(FREESHOP_DIR "/theme/images/" + signalPath);

	if (pathExists(themedSignalPath.c_str(), false))
		m_textureSignal.loadFromFile(themedSignalPath);
	else
		m_textureSignal.loadFromFile("images/" + signalPath);
#endif
	//Change the mode of the clock if enough time passed
	if (m_switchClock.getElapsedTime() >= cpp3ds::seconds(10)) {
		//Reset the clock
		m_switchClock.restart();
		m_isTransitioning = true;

		//Switch mode
#ifndef EMULATION
		if (m_textClockMode == 1 && Config::get(Config::ShowBattery).GetBool() && m_canTransition && !envIsHomebrew()) {
#else
		if (m_textClockMode == 1 && Config::get(Config::ShowBattery).GetBool() && m_canTransition) {
#endif
			//Battery percentage mode
			m_textClockMode = 2;

			//Fade out the old text
			TweenEngine::Tween::to(m_textClock, m_textClock.FILL_COLOR_ALPHA, 0.2f)
				.target(0.f)
				.setCallback(TweenEngine::TweenCallback::COMPLETE, [=](TweenEngine::BaseTween* source) {
						m_textClock.setString(std::to_string(m_batteryPercent) + "%");
						if (!m_isCollapsed)
							m_textClock.setPosition(308.f - (m_textureBattery.getSize().x + m_textClock.getLocalBounds().width), 5.f);
					})
				.start(m_tweenManager);
			TweenEngine::Tween::to(m_textClock, m_textClock.SCALE_Y, 0.2f)
				.target(.5f)
				.start(m_tweenManager);

			//Fade in the new text
			TweenEngine::Tween::to(m_textClock, m_textClock.FILL_COLOR_ALPHA, 0.2f)
				.target(255.f)
				.delay(0.3f)
				.setCallback(TweenEngine::TweenCallback::COMPLETE, [=](TweenEngine::BaseTween* source) {
					m_isTransitioning = false;
				})
				.start(m_tweenManager);
			TweenEngine::Tween::to(m_textClock, m_textClock.SCALE_Y, 0.2f)
				.target(1.f)
				.delay(0.3f)
				.start(m_tweenManager);
		} else if (m_textClockMode != 1) {
			//Clock mode
			m_textClockMode = 1;

			//Fade out old text
			TweenEngine::Tween::to(m_textClock, m_textClock.FILL_COLOR_ALPHA, 0.2f)
				.target(0.f)
				.setCallback(TweenEngine::TweenCallback::COMPLETE, [=](TweenEngine::BaseTween* source) {
				m_textClock.setString(timeTextFmt);
				if (!m_isCollapsed)
					m_textClock.setPosition(308.f - (m_textureBattery.getSize().x + m_textClock.getLocalBounds().width), 5.f);
				})
				.start(m_tweenManager);
			TweenEngine::Tween::to(m_textClock, m_textClock.SCALE_Y, 0.2f)
				.target(.5f)
				.start(m_tweenManager);

			//Fade in new text
			TweenEngine::Tween::to(m_textClock, m_textClock.FILL_COLOR_ALPHA, 0.2f)
				.target(255.f)
				.delay(0.3f)
				.setCallback(TweenEngine::TweenCallback::COMPLETE, [this](TweenEngine::BaseTween* source) {
					m_isTransitioning = false;
				})
				.start(m_tweenManager);
			TweenEngine::Tween::to(m_textClock, m_textClock.SCALE_Y, 0.2f)
				.target(1.f)
				.delay(0.3f)
				.start(m_tweenManager);
		} else {
			m_isTransitioning = false;
		}
	}

	//Don't forget to update the text at each update (except if a transition is occuring)
	if (!m_isTransitioning) {
		switch (m_textClockMode) {
			case 1:
				//Clock mode
				m_textClock.setString(timeTextFmt);
				break;
			case 2:
				//Battery mode
				m_textClock.setString(std::to_string(m_batteryPercent) + "%");
				break;
			default:
				//We should never be here
				break;
		}

		if (!m_isCollapsed)
			m_textClock.setPosition(308.f - (m_textureBattery.getSize().x + m_textClock.getLocalBounds().width), 5.f);
	}
}

void TopInformations::setTextMode(int newMode)
{
	//Update the time to show it in the top part of the App List
	time_t t = time(NULL);
	struct tm * timeinfo;
	timeinfo = localtime(&t);

	char timeTextFmt[12];
	char tempSec[3];
	strftime(tempSec, 3, "%S", timeinfo);

	strftime(timeTextFmt, 12, "%H %M", timeinfo);

	m_textClockMode = newMode;
	updateIcons(timeTextFmt);
}

void TopInformations::setModeChangeEnabled(bool newMode)
{
	m_canTransition = newMode;
}

void TopInformations::resetModeTimer()
{
	m_switchClock.restart();
}

void TopInformations::wokeUp()
{
	m_justWokeUp = true;
	m_wokeUpClock.restart();
}

#ifndef EMULATION
Result TopInformations::PTMU_GetAdapterState(u8 *out)
{
	Handle serviceHandle = 0;
	Result result = srvGetServiceHandle(&serviceHandle, "ptm:u");

	u32* ipc = getThreadCommandBuffer();
	ipc[0] = 0x50000;
	Result ret = svcSendSyncRequest(serviceHandle);

	svcCloseHandle(serviceHandle);

	if(ret < 0) return ret;

	*out = ipc[2];
	return ipc[1];
}

// Saw on the awesome 3DShell file explorer homebrew
Result TopInformations::MCUHWC_GetBatteryLevel(u8 *out)
{
	Handle serviceHandle = 0;
	Result result = srvGetServiceHandle(&serviceHandle, "mcu::HWC");

	u32* ipc = getThreadCommandBuffer();
	ipc[0] = 0x50000;
	Result ret = svcSendSyncRequest(serviceHandle);

	svcCloseHandle(serviceHandle);

	if(ret < 0) return ret;

	*out = ipc[2];
	return ipc[1];
}
#endif

} // namespace FreeShop
