#include "LoadingState.hpp"
#include "../AssetManager.hpp"
#include "../Theme.hpp"
#include "../Config.hpp"
#include <TweenEngine/Tween.h>
#include <cpp3ds/Window/Window.hpp>
#include <cpp3ds/System/I18n.hpp>
#include <cpp3ds/System/Service.hpp>
#include <cpp3ds/System/FileSystem.hpp>

namespace FreeShop {

LoadingState::LoadingState(StateStack& stack, Context& context, StateCallback callback)
: State(stack, context, callback)
{
	std::string path = cpp3ds::FileSystem::getFilePath(FREESHOP_DIR "/theme/texts.json");
	if (pathExists(path.c_str(), false)) {
		if (Theme::loadFromFile()) {
			Theme::isTextThemed = true;

			//Load differents colors
			std::string loadingIcon = Theme::get("loadingColor").GetString();
			std::string transitionScreenColor = Theme::get("transitionScreen").GetString();

			//Set the colors
			int R, G, B;

			hexToRGB(loadingIcon, &R, &G, &B);
			Theme::loadingIcon = cpp3ds::Color(R, G, B);
			hexToRGB(transitionScreenColor, &R, &G, &B);
			Theme::transitionScreenColor = cpp3ds::Color(R, G, B);
		}
	}

	m_background.setSize(cpp3ds::Vector2f(400.f, 240.f));
	if (Theme::isTextThemed)
		m_background.setFillColor(Theme::transitionScreenColor);
	else if (Config::get(Config::DarkTheme).GetBool())
		m_background.setFillColor(cpp3ds::Color(98, 98, 98, 50));
	else
		m_background.setFillColor(cpp3ds::Color(0, 0, 0, 50));
	m_icon.setFont(AssetManager<cpp3ds::Font>::get("fonts/fontawesome.ttf"));
	if (Theme::isTextThemed)
		m_icon.setFillColor(Theme::loadingIcon);
	else if (Config::get(Config::DarkTheme).GetBool())
		m_icon.setFillColor(cpp3ds::Color::White);
	else
		m_icon.setFillColor(cpp3ds::Color(110, 110, 110, 255));
	m_icon.setCharacterSize(80);
	m_icon.setString(L"\uf110");
	cpp3ds::FloatRect textRect = m_icon.getLocalBounds();
	m_icon.setOrigin(textRect.left + textRect.width / 2.f, textRect.top + 1.f + textRect.height / 2.f);
	m_icon.setPosition(160.f, 120.f);
	m_icon.setScale(0.5f, 0.5f);

	// Reset the load informations at each state load
	LoadInformations::getInstance().reset();
}

void LoadingState::renderTopScreen(cpp3ds::Window& window)
{

}

void LoadingState::renderBottomScreen(cpp3ds::Window& window)
{
	window.draw(m_icon);

	window.draw(LoadInformations::getInstance());
}

bool LoadingState::update(float delta)
{
	if (m_rotateClock.getElapsedTime() > cpp3ds::milliseconds(80))
	{
		m_icon.rotate(45);
		m_rotateClock.restart();
	}

	LoadInformations::getInstance().update(delta);
	return true;
}

bool LoadingState::processEvent(const cpp3ds::Event& event)
{
	return false;
}

} // namespace FreeShop
