#ifndef FREESHOP_LOADINFORMATIONS_HPP
#define FREESHOP_LOADINFORMATIONS_HPP

#include <cpp3ds/Graphics/Drawable.hpp>
#include <cpp3ds/Graphics/Text.hpp>
#include <cpp3ds/Window/Event.hpp>
#include "TweenObjects.hpp"
#include <TweenEngine/Tween.h>
#include <TweenEngine/TweenManager.h>

namespace FreeShop {

class LoadInformations : public cpp3ds::Drawable, public util3ds::TweenTransformable<cpp3ds::Transformable> {
public:
	void update(float delta);

	LoadInformations();
	~LoadInformations();

	static LoadInformations& getInstance();

	void updateLoadingPercentage(int newPercentage);
	void setStatus(const std::string& message);
	void reset();

protected:
	virtual void draw(cpp3ds::RenderTarget& target, cpp3ds::RenderStates states) const;

private:
	cpp3ds::Text m_textLoadingPercentage;
	util3ds::TweenText m_textStatus;

	int m_loadingPercentage;

	TweenEngine::TweenManager m_tweenManager;

};

} // namespace FreeShop

#endif // FREESHOP_LOADINFORMATIONS_HPP
