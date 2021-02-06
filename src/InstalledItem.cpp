#include <cpp3ds/System/I18n.hpp>
#include "InstalledItem.hpp"
#include "AssetManager.hpp"
#include "AppList.hpp"
#include "TitleKeys.hpp"
#include "Theme.hpp"

namespace FreeShop {

InstalledItem::InstalledItem(cpp3ds::Uint64 titleId)
: m_titleId(titleId)
, m_updateInstallCount(0)
, m_dlcInstallCount(0)
{
	TitleKeys::TitleType titleType = static_cast<TitleKeys::TitleType>(titleId >> 32);

	bool found = false;
	if (titleType == TitleKeys::SystemApplication || titleType == TitleKeys::SystemApplet)
	{
		m_appItem = std::make_shared<AppItem>();
		m_appItem->loadFromSystemTitleId(titleId);
	}
	else
	{
		for (auto& appItem : AppList::getInstance().getList())
		{
			m_appItem = appItem->getAppItem();
			if (titleId == m_appItem->getTitleId())
			{
				found = true;
				break;
			}
		}

		if (!found)
		{
				throw 0;
		}
	}

	m_appItem->setInstalled(true);

	if (Theme::isInstalledItemBG9Themed)
		m_background.setTexture(&AssetManager<cpp3ds::Texture>::get(FREESHOP_DIR "/theme/images/installed_item_bg.9.png"));
	else
		m_background.setTexture(&AssetManager<cpp3ds::Texture>::get("images/installed_item_bg.9.png"));

	m_textTitle.setString(m_appItem->getTitle());
	m_textTitle.setCharacterSize(11);
	m_textTitle.setPosition(m_background.getPadding().left, m_background.getPadding().top);
	if (Theme::isTextThemed)
		m_textTitle.setFillColor(Theme::primaryTextColor);
	else
		m_textTitle.setFillColor(cpp3ds::Color::Black);
	m_textTitle.useSystemFont();
	if (titleType == TitleKeys::SystemApplication || titleType == TitleKeys::SystemApplet)
		m_textSize.setString(_(" - System Title"));
	else if (m_appItem->getFilesize() > 1024 * 1024 * 1024)
		m_textSize.setString(_(" - %.1f GB", static_cast<float>(m_appItem->getFilesize()) / 1024.f / 1024.f / 1024.f));
	else if (m_appItem->getFilesize() > 1024 * 1024)
		m_textSize.setString(_(" - %.1f MB", static_cast<float>(m_appItem->getFilesize()) / 1024.f / 1024.f));
	else
		m_textSize.setString(_(" - %d KB", m_appItem->getFilesize() / 1024));
	m_textSize.setCharacterSize(11);
	m_textSize.setPosition(m_textTitle.getLocalBounds().width + 4, m_background.getPadding().top);
	if (Theme::isTextThemed)
		m_textSize.setFillColor(Theme::secondaryTextColor);
	else
		m_textSize.setFillColor(cpp3ds::Color(150, 150, 150));
	m_textSize.useSystemFont();
	m_textWarningUpdate.setFont(AssetManager<cpp3ds::Font>::get("fonts/fontawesome.ttf"));
	m_textWarningUpdate.setString(L"\uf071");
	m_textWarningUpdate.setCharacterSize(14);
	m_textWarningUpdate.setFillColor(cpp3ds::Color(50, 50, 50));
	m_textWarningUpdate.setOutlineColor(cpp3ds::Color(0, 200, 0));
	m_textWarningUpdate.setOutlineThickness(1.f);
	m_textWarningUpdate.setOrigin(0, m_textWarningUpdate.getLocalBounds().top + m_textWarningUpdate.getLocalBounds().height / 2.f);

	m_textWarningDLC = m_textWarningUpdate;
	m_textWarningDLC.setOutlineColor(cpp3ds::Color(255, 255, 0, 255));

	setHeight(16.f);
}

void InstalledItem::draw(cpp3ds::RenderTarget &target, cpp3ds::RenderStates states) const
{
	states.transform *= getTransform();

	target.draw(m_background, states);
	target.draw(m_textTitle, states);
	target.draw(m_textSize, states);
	if (m_updates.size() - m_updateInstallCount > 0)
		target.draw(m_textWarningUpdate, states);
	if (m_dlc.size() - m_dlcInstallCount > 0)
		target.draw(m_textWarningDLC, states);
}

cpp3ds::Uint64 InstalledItem::getTitleId() const
{
	return m_titleId;
}

void InstalledItem::setUpdateStatus(cpp3ds::Uint64 titleId, bool installed)
{
	m_updates[titleId] = installed;
	m_updateInstallCount = 0;
	for (auto& update : m_updates)
		if (update.second)
			m_updateInstallCount++;
}

void InstalledItem::setDLCStatus(cpp3ds::Uint64 titleId, bool installed)
{
	m_dlc[titleId] = installed;
	m_dlcInstallCount = 0;
	for (auto& dlc : m_dlc)
		if (dlc.second)
			m_dlcInstallCount++;
}

std::shared_ptr<AppItem> InstalledItem::getAppItem() const
{
	return m_appItem;
}

void InstalledItem::processEvent(const cpp3ds::Event &event)
{

}

void InstalledItem::setValues(int tweenType, float *newValues)
{
	switch (tweenType) {
		case HEIGHT:
			setHeight(newValues[0]);
			break;
		default:
			TweenTransformable::setValues(tweenType, newValues);
	}
}

int InstalledItem::getValues(int tweenType, float *returnValues)
{
	switch (tweenType) {
		case HEIGHT:
			returnValues[0] = getHeight();
			return 1;
		default:
			return TweenTransformable::getValues(tweenType, returnValues);
	}
}

void InstalledItem::setHeight(float height)
{
	m_height = height;

	m_background.setContentSize(320.f + m_background.getPadding().width - m_background.getTexture()->getSize().x + 2.f, height);
	float paddingRight = m_background.getSize().x - m_background.getContentSize().x - m_background.getPadding().left;
	float paddingBottom = m_background.getSize().y - m_background.getContentSize().y - m_background.getPadding().top;

	m_textWarningUpdate.setPosition(m_background.getSize().x - paddingRight - m_textWarningUpdate.getLocalBounds().width - 7.f,
	                                m_background.getPadding().top + height - height * 0.4f - 3.f);
	m_textWarningDLC.setPosition(m_textWarningUpdate.getPosition());
	m_textWarningDLC.move(-20.f, 0);
}

float InstalledItem::getHeight() const
{
	return m_height;
}

void InstalledItem::updateGameTitle()
{
	int numberOfDLC = 0;
	int totalDLC = TitleKeys::getRelated(m_appItem->getTitleId(), TitleKeys::DLC).size();
	for (auto &dlc : getDLC())
		if (dlc.second)
			numberOfDLC++;

	int numberOfUpdates = 0;
	int totalUpdates = TitleKeys::getRelated(m_appItem->getTitleId(), TitleKeys::Update).size();
	for (auto &update : getUpdates())
		if (update.second)
			numberOfUpdates++;

	int maxSize = 310;
	if ((totalDLC - numberOfDLC) > 0)
		maxSize = m_textWarningDLC.getPosition().x - 5;
	else if ((totalUpdates - numberOfUpdates) > 0)
		maxSize = m_textWarningUpdate.getPosition().x - 5;

	// Shorten the app name if it's out of the screen
	if (m_textTitle.getLocalBounds().width + m_textSize.getLocalBounds().width > maxSize) {
		cpp3ds::Text tmpText;
		tmpText.setCharacterSize(11);
		tmpText.useSystemFont();
		auto s = m_textTitle.getString().toUtf8();

		for (int i = 0; i <= s.size(); ++i) {
			tmpText.setString(cpp3ds::String::fromUtf8(s.begin(), s.begin() + i));

			if (tmpText.getLocalBounds().width + m_textSize.getLocalBounds().width > maxSize) {
				cpp3ds::String titleTxt = tmpText.getString();
				titleTxt.erase(titleTxt.getSize() - 3, 3);
				titleTxt.insert(titleTxt.getSize(), "...");

				m_textTitle.setString(titleTxt);
				m_textSize.setPosition(m_textTitle.getLocalBounds().width + 4, m_background.getPadding().top);
				break;
			}
		}
	}
}

} // namespace FreeShop
