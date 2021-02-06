#include "LoadInformations.hpp"
#include "AssetManager.hpp"
#include "DownloadQueue.hpp"
#include "Notification.hpp"
#include "Theme.hpp"
#include "Util.hpp"
#include "States/StateIdentifiers.hpp"
#include <cpp3ds/System/I18n.hpp>
#include <cpp3ds/System/FileSystem.hpp>

namespace FreeShop {

LoadInformations::LoadInformations()
: m_loadingPercentage(0)
{
	std::string path = cpp3ds::FileSystem::getFilePath(FREESHOP_DIR "/theme/texts.json");
	if (pathExists(path.c_str(), false)) {
		if (Theme::loadFromFile()) {
			Theme::isTextThemed = true;

			//Load differents colors
			std::string percentageColorValue = Theme::get("percentageText").GetString();

			//Set the colors
			int R, G, B;

			hexToRGB(percentageColorValue, &R, &G, &B);
			Theme::percentageText = cpp3ds::Color(R, G, B);
		}
	}

	//Texts
	m_textLoadingPercentage.setCharacterSize(14);
	if (Theme::isTextThemed)
		m_textLoadingPercentage.setFillColor(Theme::percentageText);
	else
		m_textLoadingPercentage.setFillColor(cpp3ds::Color::Black);
	m_textLoadingPercentage.setOutlineColor(cpp3ds::Color(0, 0, 0, 70));
	m_textLoadingPercentage.setOutlineThickness(2.f);
	m_textLoadingPercentage.setPosition(160.f, 230.f);
	m_textLoadingPercentage.useSystemFont();
	m_textLoadingPercentage.setString(_("0%%"));

	m_textStatus.setCharacterSize(14);
	if (Theme::isTextThemed)
		m_textStatus.setFillColor(Theme::loadingText);
	else
		m_textStatus.setFillColor(cpp3ds::Color::Black);
	m_textStatus.setOutlineColor(cpp3ds::Color(0, 0, 0, 70));
	m_textStatus.setOutlineThickness(2.f);
	m_textStatus.setPosition(160.f, 155.f);
	m_textStatus.useSystemFont();
	TweenEngine::Tween::to(m_textStatus, util3ds::TweenText::FILL_COLOR_ALPHA, 0.3f)
			.target(180)
			.repeatYoyo(-1, 0)
			.start(m_tweenManager);
}

LoadInformations::~LoadInformations()
{

}

void LoadInformations::draw(cpp3ds::RenderTarget &target, cpp3ds::RenderStates states) const
{
	states.transform *= getTransform();

	if (m_loadingPercentage > -1)
		target.draw(m_textLoadingPercentage);

	target.draw(m_textStatus);
}

void LoadInformations::update(float delta)
{
	m_tweenManager.update(delta);
}

void LoadInformations::updateLoadingPercentage(int newPercentage)
{
	// If the new percentage is the same than the actual percentage, no need to recalculate the text
	if (newPercentage == m_loadingPercentage)
		return;

	// Check that percentage is between 0 and 100
	if (newPercentage <= -1)
		m_loadingPercentage = -1;
	else if (newPercentage >= 100)
		m_loadingPercentage = 100;
	else
		m_loadingPercentage = newPercentage;

	// Set the text to the percentage and center it
	m_textLoadingPercentage.setString(_("%i%%", m_loadingPercentage));
	cpp3ds::FloatRect rect = m_textLoadingPercentage.getLocalBounds();
	m_textLoadingPercentage.setOrigin(rect.width / 2.f, rect.height / 2.f);
}

LoadInformations &LoadInformations::getInstance()
{
	static LoadInformations loadInfos;
	return loadInfos;
}

void LoadInformations::reset()
{
	updateLoadingPercentage(-1);
	setStatus("");
}

void LoadInformations::setStatus(const std::string &message)
{
	m_textStatus.setString(message);
	cpp3ds::FloatRect rect = m_textStatus.getLocalBounds();
	m_textStatus.setOrigin(rect.width / 2.f, rect.height / 2.f);
}

} // namespace FreeShop
