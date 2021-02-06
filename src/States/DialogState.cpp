#include <cpp3ds/System/I18n.hpp>
#include <TweenEngine/Tween.h>
#include <cmath>
#include <algorithm>
#include "DialogState.hpp"
#include "../AssetManager.hpp"
#include "../Theme.hpp"
#include "SleepState.hpp"
#include "BrowseState.hpp"

namespace FreeShop {

DialogState::DialogState(StateStack &stack, Context &context, StateCallback callback)
: State(stack, context, callback)
, m_isClosing(false)
, m_finishedFadeIn(false)
, m_isOkButtonTouched(false)
, m_isCancelButtonTouched(false)
{
	m_overlay.setSize(cpp3ds::Vector2f(400.f, 240.f));
	m_overlay.setFillColor(cpp3ds::Color(0, 0, 0, 0));

	if (Theme::isButtonRadius9Themed) {
		cpp3ds::Texture &texture = AssetManager<cpp3ds::Texture>::get(FREESHOP_DIR "/theme/images/button-radius.9.png");
		m_background.setTexture(&texture);
	} else if (Config::get(Config::DarkTheme).GetBool()) {
		cpp3ds::Texture &texture = AssetManager<cpp3ds::Texture>::get("darkimages/button-radius.9.png");
		m_background.setTexture(&texture);
	} else {
		cpp3ds::Texture &texture = AssetManager<cpp3ds::Texture>::get("images/button-radius.9.png");
		m_background.setTexture(&texture);
	}

	m_background.setSize(cpp3ds::Vector2f(280.f, 200.f));
	m_background.setPosition(20.f, 20.f);
	if (Theme::isTextThemed)
		m_background.setColor(cpp3ds::Color(Theme::dialogBackground.r, Theme::dialogBackground.g, Theme::dialogBackground.b, 128));
	else if (Config::get(Config::DarkTheme).GetBool())
		m_background.setColor(cpp3ds::Color(126, 126, 126));
	else
		m_background.setColor(cpp3ds::Color(255, 255, 255, 128));

	m_message.setCharacterSize(12);
	m_message.setFillColor(cpp3ds::Color(66, 66, 66, 0));
	m_message.useSystemFont();
	m_message.setPosition(25.f, 60.f);

	m_title.setFont(AssetManager<cpp3ds::Font>::get("fonts/grandhotel.ttf"));
	m_title.setPosition(160.f, 40.f);
	m_title.setFillColor(cpp3ds::Color(100, 100, 100, 0));
	m_title.setOutlineColor(cpp3ds::Color(255, 255, 255, 0));
	m_title.setOutlineThickness(3.f);
	m_title.setCharacterSize(16);

	m_buttonOkBackground.setSize(cpp3ds::Vector2f(110.f, 25.f));
	if (Theme::isTextThemed)
		m_buttonOkBackground.setOutlineColor(cpp3ds::Color(Theme::dialogButton.r, Theme::dialogButton.g, Theme::dialogButton.b, 0));
	else if (Config::get(Config::DarkTheme).GetBool())
		m_buttonOkBackground.setOutlineColor(cpp3ds::Color(201, 28, 28));
	else
		m_buttonOkBackground.setOutlineColor(cpp3ds::Color(158, 158, 158, 0));
	m_buttonOkBackground.setOutlineThickness(1.f);
	m_buttonOkBackground.setPosition(165.f, 180.f);
	m_buttonOkBackground.setFillColor(cpp3ds::Color(m_buttonOkBackground.getOutlineColor().r - 20, m_buttonOkBackground.getOutlineColor().g - 20, m_buttonOkBackground.getOutlineColor().b - 20, 0));

	m_buttonCancelBackground.setSize(cpp3ds::Vector2f(110.f, 25.f));
	if (Theme::isTextThemed)
		m_buttonCancelBackground.setOutlineColor(cpp3ds::Color(Theme::dialogButton.r, Theme::dialogButton.g, Theme::dialogButton.b, 0));
	else if (Config::get(Config::DarkTheme).GetBool())
		m_buttonCancelBackground.setOutlineColor(cpp3ds::Color(201, 28, 28));
	else
		m_buttonCancelBackground.setOutlineColor(cpp3ds::Color(158, 158, 158, 0));
	m_buttonCancelBackground.setOutlineThickness(1.f);
	m_buttonCancelBackground.setPosition(40.f, 180.f);
	m_buttonCancelBackground.setFillColor(cpp3ds::Color(m_buttonCancelBackground.getOutlineColor().r - 20, m_buttonCancelBackground.getOutlineColor().g - 20, m_buttonCancelBackground.getOutlineColor().b - 20, 0));

	m_buttonOkText.setString(_("\uE000 Ok"));
	m_buttonOkText.setCharacterSize(14);
	m_buttonOkText.setFillColor(cpp3ds::Color(3, 169, 244, 0));
	if (Theme::isTextThemed)
		m_buttonOkText.setFillColor(cpp3ds::Color(Theme::dialogButtonText.r, Theme::dialogButtonText.g, Theme::dialogButtonText.b, 0));
	else if (Config::get(Config::DarkTheme).GetBool())
		m_buttonOkText.setFillColor(cpp3ds::Color(165, 44, 44));
	else
		m_buttonOkText.setFillColor(cpp3ds::Color(3, 169, 244, 0));
	m_buttonOkText.useSystemFont();
	m_buttonOkText.setPosition(m_buttonOkBackground.getPosition().x + m_buttonOkBackground.getGlobalBounds().width / 2, m_buttonOkBackground.getPosition().y + m_buttonOkBackground.getGlobalBounds().height / 2);
	m_buttonOkText.setOrigin(m_buttonOkText.getGlobalBounds().width / 2, m_buttonOkText.getGlobalBounds().height / 1.6f);

	m_buttonCancelText = m_buttonOkText;
	m_buttonCancelText.setString(_("\uE001 Cancel"));
	m_buttonCancelText.setCharacterSize(12);
	m_buttonCancelText.setPosition(m_buttonCancelBackground.getPosition().x + m_buttonCancelBackground.getGlobalBounds().width / 2, m_buttonCancelBackground.getPosition().y + m_buttonCancelBackground.getGlobalBounds().height / 2);
	m_buttonCancelText.setOrigin(m_buttonCancelText.getGlobalBounds().width / 2, m_buttonCancelText.getGlobalBounds().height / 1.5f);

	m_bottomView.setCenter(cpp3ds::Vector2f(160.f, 120.f));
	m_bottomView.setSize(cpp3ds::Vector2f(320.f * 0.9f, 240.f * 0.9f));

	cpp3ds::String tmp;
	Event event = {GetText, &tmp};
	if (runCallback(&event)) {
		auto message = reinterpret_cast<cpp3ds::String*>(event.data);
		m_message.setString(*message);
	} else
		m_message.setString(_("Are you sure you want to continue?"));

	// Calculate word-wrapping
	int startPos = 0;
	int lastSpace = 0;
	auto s = m_message.getString().toUtf8();
	cpp3ds::Text tmpText = m_message;
	for (int i = 0; i < s.size(); ++i)
	{
		if (s[i] == ' ')
			lastSpace = i;
		tmpText.setString(cpp3ds::String::fromUtf8(s.begin() + startPos, s.begin() + i));
		if (tmpText.getLocalBounds().width > 260)
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
	m_message.setString(cpp3ds::String::fromUtf8(s.begin(), s.end()));

	event = {GetTitle, &tmp};
	if (runCallback(&event)) {
		auto message = reinterpret_cast<cpp3ds::String*>(event.data);
		m_title.setString(*message);
	} else
		m_title.setString(_("Dialog"));

	// Shorten the dialog title if it's out of the screen
	int maxSize = 270;
	if (m_title.getLocalBounds().width > maxSize) {
		cpp3ds::Text tmpText = m_title;
		tmpText.setString("");
		auto s = m_title.getString().toUtf8();

			for (int i = 0; i <= s.size(); ++i) {
			tmpText.setString(cpp3ds::String::fromUtf8(s.begin(), s.begin() + i));

			if (tmpText.getLocalBounds().width > maxSize) {
				cpp3ds::String titleTxt = tmpText.getString();
				titleTxt.erase(titleTxt.getSize() - 3, 3);
				titleTxt.insert(titleTxt.getSize(), "...");

				m_title.setString(titleTxt);
				break;
			}
		}
	}
	m_title.setOrigin(std::round(m_title.getLocalBounds().width / 2), std::round(m_title.getLocalBounds().height / 2));

	m_scrollbar.setSize(cpp3ds::Vector2u(2, 158));
	m_scrollbar.setScrollAreaSize(cpp3ds::Vector2u(320, 109));
	m_scrollbar.setDragRect(cpp3ds::IntRect(0, 0, 320, 240));
	m_scrollbar.setColor(cpp3ds::Color(0, 0, 0, 40));
	m_scrollbar.setPosition(292.f, 60.f);
	m_scrollbar.setAutoHide(false);
	m_scrollbar.attachObject(this);

	m_scrollSize = cpp3ds::Vector2f(320.f, m_message.getLocalBounds().top + m_message.getLocalBounds().height);

#define TWEEN_IN(obj) \
	TweenEngine::Tween::to(obj, obj.FILL_COLOR_ALPHA, 0.2f).target(255.f).start(m_tweenManager); \
	TweenEngine::Tween::to(obj, obj.OUTLINE_COLOR_ALPHA, 0.2f).target(128.f).start(m_tweenManager);

	TweenEngine::Tween::to(m_overlay, m_overlay.FILL_COLOR_ALPHA, 0.2f).target(150.f).start(m_tweenManager);
	TweenEngine::Tween::to(m_background, m_background.COLOR_ALPHA, 0.2f).target(255.f).setCallback(TweenEngine::TweenCallback::COMPLETE, [this](TweenEngine::BaseTween* source) {
		m_finishedFadeIn = true;
	}).start(m_tweenManager);
	TweenEngine::Tween::to(m_buttonOkBackground, m_buttonOkBackground.OUTLINE_COLOR_ALPHA, 0.2f).target(128.f).start(m_tweenManager);
	TweenEngine::Tween::to(m_buttonCancelBackground, m_buttonCancelBackground.OUTLINE_COLOR_ALPHA, 0.2f).target(128.f).start(m_tweenManager);
	TWEEN_IN(m_message);
	TWEEN_IN(m_buttonOkText);
	TWEEN_IN(m_buttonCancelText);
	TWEEN_IN(m_title);
	TweenEngine::Tween::to(m_bottomView, m_bottomView.SIZE_XY, 0.2f).target(320.f, 240.f).start(m_tweenManager);
}

void DialogState::renderTopScreen(cpp3ds::Window &window)
{
	window.draw(m_overlay);
}

void DialogState::renderBottomScreen(cpp3ds::Window &window)
{
	cpp3ds::UintRect scissor(0, 40 + m_background.getPosition().y, 320, (240.f / m_bottomView.getSize().y) * 119.f);
	window.draw(m_overlay);

	window.setView(m_bottomView);

	window.draw(m_background);
	window.draw(m_message, scissor);
	window.draw(m_buttonOkBackground);
	window.draw(m_buttonOkText);
	window.draw(m_buttonCancelBackground);
	window.draw(m_buttonCancelText);
	window.draw(m_title);

	if (m_finishedFadeIn && !m_isClosing)
		window.draw(m_scrollbar);

	window.setView(window.getDefaultView());
}

bool DialogState::update(float delta)
{
	m_tweenManager.update(delta);
	m_scrollbar.update(delta);
	return false;
}

bool DialogState::processEvent(const cpp3ds::Event &event)
{
	BrowseState::clockDownloadInactivity.restart();
	SleepState::clock.restart();
	if (m_isClosing)
		return false;

	m_scrollbar.processEvent(event);

	bool triggerResponse = false;
	bool accepted = false;

	if (event.type == cpp3ds::Event::TouchEnded)
	{
		if (m_buttonOkBackground.getGlobalBounds().contains(event.touch.x, event.touch.y) && m_isOkButtonTouched)
			triggerResponse = accepted = true;
		else if (m_buttonCancelBackground.getGlobalBounds().contains(event.touch.x, event.touch.y) && m_isCancelButtonTouched)
			triggerResponse = true;
	}
	else if (event.type == cpp3ds::Event::TouchMoved)
	{
		if (!m_buttonOkBackground.getGlobalBounds().contains(event.touch.x, event.touch.y) && m_isOkButtonTouched) {
			TweenEngine::Tween::to(m_buttonOkBackground, m_buttonOkBackground.FILL_COLOR_ALPHA, 0.2f).target(0.f).start(m_tweenManager);
			m_isOkButtonTouched = false;
		}

		if (!m_buttonCancelBackground.getGlobalBounds().contains(event.touch.x, event.touch.y) && m_isCancelButtonTouched) {
			TweenEngine::Tween::to(m_buttonCancelBackground, m_buttonCancelBackground.FILL_COLOR_ALPHA, 0.2f).target(0.f).start(m_tweenManager);
			m_isCancelButtonTouched = false;
		}
	}
	else if (event.type == cpp3ds::Event::TouchBegan)
	{
		if (m_buttonOkBackground.getGlobalBounds().contains(event.touch.x, event.touch.y)) {
			TweenEngine::Tween::to(m_buttonOkBackground, m_buttonOkBackground.FILL_COLOR_ALPHA, 0.2f).target(255.f).start(m_tweenManager);
			m_isOkButtonTouched = true;
		} else if (m_buttonCancelBackground.getGlobalBounds().contains(event.touch.x, event.touch.y)) {
			TweenEngine::Tween::to(m_buttonCancelBackground, m_buttonCancelBackground.FILL_COLOR_ALPHA, 0.2f).target(255.f).start(m_tweenManager);
			m_isCancelButtonTouched = true;
		}
	}
	else if (event.type == cpp3ds::Event::KeyReleased)
	{
		if (event.key.code == cpp3ds::Keyboard::A)
			triggerResponse = accepted = true;
		else if (event.key.code == cpp3ds::Keyboard::B)
			triggerResponse = true;
	}

#define TWEEN_OUT(obj) \
	TweenEngine::Tween::to(obj, obj.FILL_COLOR_ALPHA, 0.2f).target(0.f).start(m_tweenManager); \
	TweenEngine::Tween::to(obj, obj.OUTLINE_COLOR_ALPHA, 0.2f).target(0.f).start(m_tweenManager);

	if (triggerResponse)
	{
		Event evt = {Response, &accepted};

		if (runCallback(&evt))
		{
			m_isClosing = true;
			m_tweenManager.killAll();

			// Highlight selected button
			if (accepted)
				TweenEngine::Tween::to(m_buttonOkBackground, m_buttonOkBackground.FILL_COLOR_RGB, 0.1f).target(m_buttonOkBackground.getOutlineColor().r, m_buttonOkBackground.getOutlineColor().g, m_buttonOkBackground.getOutlineColor().b).start(m_tweenManager);
			else
				TweenEngine::Tween::to(m_buttonCancelBackground, m_buttonCancelBackground.FILL_COLOR_RGB, 0.1f).target(m_buttonCancelBackground.getOutlineColor().r, m_buttonCancelBackground.getOutlineColor().g, m_buttonCancelBackground.getOutlineColor().b).start(m_tweenManager);

			TweenEngine::Tween::to(m_background, m_background.COLOR_ALPHA, 0.2f).target(0.f).start(m_tweenManager);
			TWEEN_OUT(m_message);
			TWEEN_OUT(m_buttonOkBackground);
			TWEEN_OUT(m_buttonCancelBackground);
			TWEEN_OUT(m_buttonOkText);
			TWEEN_OUT(m_buttonCancelText);
			TWEEN_OUT(m_title);
			TweenEngine::Tween::to(m_bottomView, m_bottomView.SIZE_XY, 0.2f).target(320.f * 1.2f, 240.f * 1.2f).start(m_tweenManager);
			TweenEngine::Tween::to(m_overlay, m_overlay.FILL_COLOR_ALPHA, 0.3f).target(0.f)
				.setCallback(TweenEngine::TweenCallback::COMPLETE, [this](TweenEngine::BaseTween* source) {
					requestStackPop();
				})
				.delay(0.2f).start(m_tweenManager);
		}
	}
	return false;
}

void DialogState::setScroll(float position)
{
	m_scrollPos = position;
	m_message.setPosition(25.f, std::round(60.f + position));
}

float DialogState::getScroll()
{
	return m_scrollPos;
}

const cpp3ds::Vector2f &DialogState::getScrollSize()
{
	return m_scrollSize;
}

} // namespace FreeShop
