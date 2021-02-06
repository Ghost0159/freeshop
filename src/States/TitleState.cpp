#include "TitleState.hpp"
#include <TweenEngine/Tween.h>
#include <cpp3ds/Window/Window.hpp>
#include <cpp3ds/System/I18n.hpp>
#include <cpp3ds/System/Service.hpp>
#include <cpp3ds/System/Sleep.hpp>
#include <cpp3ds/System/FileSystem.hpp>
#include "../Config.hpp"
#include "../Notification.hpp"
#include "../Theme.hpp"
#include "../Util.hpp"
#include "../AssetManager.hpp"
#ifndef EMULATION
#include "../MCU/Mcu.hpp"
#endif

using namespace TweenEngine;
using namespace util3ds;

namespace FreeShop {

TitleState::TitleState(StateStack& stack, Context& context, StateCallback callback)
: State(stack, context, callback)
{
	std::string path = cpp3ds::FileSystem::getFilePath(FREESHOP_DIR "/theme/texts.json");
	if (pathExists(path.c_str(), false)) {
		if (Theme::loadFromFile()) {
			Theme::isTextThemed = true;

			//Load differents colors
			std::string freTextValue = Theme::get("freText").GetString();
			std::string versionTextValue = Theme::get("versionText").GetString();

			//Set the colors
			int R, G, B;

			hexToRGB(freTextValue, &R, &G, &B);
			Theme::freText = cpp3ds::Color(R, G, B);

			hexToRGB(versionTextValue, &R, &G, &B);
			Theme::versionText = cpp3ds::Color(R, G, B);
		}
	}

	m_textVersion.setString(FREESHOP_VERSION);
	m_textVersion.setCharacterSize(12);
	if (Theme::isTextThemed)
		m_textVersion.setFillColor(Theme::versionText);
	else if (Config::get(Config::DarkTheme).GetBool())
		m_textVersion.setFillColor(cpp3ds::Color(176, 176, 176));
	else
		m_textVersion.setFillColor(cpp3ds::Color::White);
	m_textVersion.setOutlineColor(cpp3ds::Color(0, 0, 0, 100));
	m_textVersion.setOutlineThickness(1.f);
	m_textVersion.setPosition(318.f - m_textVersion.getLocalBounds().width, 222.f);

	m_textFree.setString("fre");
	if (Theme::isTextThemed)
		m_textFree.setFillColor(cpp3ds::Color(Theme::freText.r, Theme::freText.g, Theme::freText.b, 0));
	else if (Config::get(Config::DarkTheme).GetBool())
		m_textFree.setFillColor(cpp3ds::Color(98, 98, 98, 0));
	else
		m_textFree.setFillColor(cpp3ds::Color(255, 255, 255, 0));

	m_textFree.setCharacterSize(75);
	m_textFree.setPosition(23, 64);
	m_textFree.setFont(AssetManager<cpp3ds::Font>::get("fonts/prod-sans.ttf"));

	if (fopen(FREESHOP_DIR "/theme/images/eshop.png", "rb"))
		m_textureEshop.loadFromFile(FREESHOP_DIR "/theme/images/eshop.png");
  else
		m_textureEshop.loadFromFile("images/eshop.png");

	m_spriteEshop.setTexture(m_textureEshop, true);
	m_spriteEshop.setColor(cpp3ds::Color(0xf4, 0x6c, 0x26, 0x00));
	m_spriteEshop.setPosition(390.f - m_spriteEshop.getLocalBounds().width, 40.f);

	if (fopen(FREESHOP_DIR "/theme/images/bag.png", "rb"))
		m_textureBag.loadFromFile(FREESHOP_DIR "/theme/images/bag.png");
	else
		m_textureBag.loadFromFile("images/bag.png");

	//if(Config::get(Config::DownloadTitleKeys).GetBool()) {
		m_AP_Texture.loadFromFile("images/AP.png");
	//}

	m_spriteBag.setTexture(m_textureBag, true);
	m_spriteBag.setColor(cpp3ds::Color(0xf4, 0x6c, 0x26, 0x00));
	m_spriteBag.setPosition(50.f, 50.f);
	m_spriteBag.setScale(0.4f, 0.4f);

	// Tween the bag
	Tween::to(m_spriteBag, TweenSprite::COLOR_ALPHA, 0.5f)
			.target(255.f)
			.delay(1.f)
			.start(m_manager);
	Tween::to(m_spriteBag, TweenSprite::POSITION_XY, 0.5f)
			.target(10.f, 30.f)
			.delay(1.f)
			.start(m_manager);
	Tween::to(m_spriteBag, TweenSprite::SCALE_XY, 0.5f)
			.target(1.f, 1.f)
			.delay(1.f)
			.start(m_manager);
	Tween::to(m_spriteBag, TweenSprite::COLOR_RGB, 1.f)
			.target(0, 0, 0)
			.delay(3.5f)
			.start(m_manager);

	// Tween the "fre" text
	Tween::to(m_textFree, TweenSprite::COLOR_ALPHA, 1.f)
			.target(255.f)
			.delay(3.5f)
			.start(m_manager);

	// Tween the logo after the bag
	Tween::to(m_spriteEshop, TweenSprite::POSITION_Y, 1.f)
			.targetRelative(30.f)
			.delay(1.5f)
			.start(m_manager);
	Tween::to(m_spriteEshop, TweenSprite::COLOR_ALPHA, 1.f)
			.target(255)
			.delay(1.5f)
			.start(m_manager);
	Tween::to(m_spriteEshop, TweenSprite::COLOR_RGB, 1.f)
			.target(0, 0, 0)
			.delay(3.5f)
			.start(m_manager);
}

TitleState::~TitleState()
{

}

void TitleState::renderTopScreen(cpp3ds::Window& window)
{
	window.draw(m_spriteEshop);
	window.draw(m_spriteBag);
	window.draw(m_textFree);
}

void TitleState::renderBottomScreen(cpp3ds::Window& window)
{
	window.draw(m_textVersion);
}

bool TitleState::update(float delta)
{
	m_manager.update(delta);
	return true;
}

bool TitleState::processEvent(const cpp3ds::Event& event)
{
	return true;
}

} // namespace FreeShop
