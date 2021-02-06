#ifndef FREESHOP_TITLESTATE_HPP
#define FREESHOP_TITLESTATE_HPP

#include "State.hpp"
#include "../TweenObjects.hpp"
#include <cpp3ds/Graphics/Sprite.hpp>
#include <cpp3ds/Graphics/Text.hpp>
#include <TweenEngine/TweenManager.h>

namespace FreeShop {

class TitleState : public State
{
public:
	TitleState(StateStack& stack, Context& context, StateCallback callback);
	~TitleState();

	virtual void renderTopScreen(cpp3ds::Window& window);
	virtual void renderBottomScreen(cpp3ds::Window& window);
	virtual bool update(float delta);
	virtual bool processEvent(const cpp3ds::Event& event);

private:
	cpp3ds::Texture m_textureEshop;
	cpp3ds::Texture m_textureBag;
	cpp3ds::Texture m_AP_Texture;
	cpp3ds::Text m_textVersion;
	util3ds::TweenText m_textFree;
	util3ds::TweenSprite m_spriteEshop;
	util3ds::TweenSprite m_spriteBag;

	TweenEngine::TweenManager m_manager;
};

} // namespace FreeShop

#endif // FREESHOP_TITLESTATE_HPP
