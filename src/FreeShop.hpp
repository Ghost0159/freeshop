#ifndef FREESHOP_FREESHOP_HPP
#define FREESHOP_FREESHOP_HPP

#include <TweenEngine/TweenManager.h>
#include <cpp3ds/Graphics.hpp>
#include <cpp3ds/Network.hpp>
#include "States/StateStack.hpp"
#include "GUI/NinePatch.hpp"
#include "AssetManager.hpp"

namespace FreeShop {

extern cpp3ds::Uint64 g_requestJump;
extern bool g_requestShutdown;
extern bool g_requestReboot;

class FreeShop: public cpp3ds::Game {
public:
	FreeShop();
	~FreeShop();
	void update(float delta);
	void processEvent(cpp3ds::Event& event);
	void renderTopScreen(cpp3ds::Window& window);
	void renderBottomScreen(cpp3ds::Window& window);

	static void prepareToCloseApp();

private:
	StateStack *m_stateStack;
	cpp3ds::Text textFPS;

	// Shared State context variables
	std::vector<char*> m_data;
	cpp3ds::String m_text;

	// Backgrounds
	bool m_topBG;
	bool m_botBG;
	gui3ds::NinePatch m_rectTopBG;
	gui3ds::NinePatch m_rectBotBG;
};

}

#endif // FREESHOP_FREESHOP_HPP
