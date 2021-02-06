#ifndef FREESHOP_NOTIFICATION_HPP
#define FREESHOP_NOTIFICATION_HPP

#include <TweenEngine/TweenManager.h>
#include "GUI/Button.hpp"

#define NOTIFICATION_SPACING 0.f
#define NOTIFICATION_DURATION 5.f

namespace FreeShop {

class Notification: public gui3ds::Button {
public:
	Notification();
	~Notification();

	void animate();

	static void update(float delta);
	static void spawn(cpp3ds::String message);

	static std::vector<std::unique_ptr<Notification>> notifications;

	static void sendNews(cpp3ds::String title, cpp3ds::String message);

private:
	static cpp3ds::Texture m_texture;
	static float m_spawnPositionY;

	bool m_markForDelete;
	TweenEngine::TweenManager m_tweenManager;
	float m_destinationY;
};

} // namespace FreeShop

#endif // FREESHOP_NOTIFICATION_HPP
