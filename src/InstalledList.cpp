#define FTS_FUZZY_MATCH_IMPLEMENTATION

#include <cmath>
#include <TweenEngine/Tween.h>
#include <cpp3ds/System/Lock.hpp>
#include "InstalledList.hpp"
#include "AppList.hpp"
#include "TitleKeys.hpp"
#include "fts_fuzzy_match.h"
#ifndef EMULATION
#include "KeyboardApplet.hpp"
#endif

namespace FreeShop {

InstalledList::InstalledList()
: m_scrollPos(0.f)
, m_size(320.f, 0.f)
, m_expandedItem(nullptr)
, m_isUpdatingList(false)
, m_gameCount(0)
, m_sortType(Name)
, m_sortAscending(true)
, m_canDrawList(true)
{
	// Make install options initially transparent for fade in
	TweenEngine::Tween::set(m_options, InstalledOptions::ALPHA)
			.target(0.f).start(m_tweenManager);

	m_currentSearch = "";
}

InstalledList &InstalledList::getInstance()
{
	static InstalledList installedList;
	return installedList;
}

void InstalledList::refresh()
{
	cpp3ds::Lock lock(m_mutexRefresh);
	cpp3ds::Uint64 relatedTitleId;
	std::vector<cpp3ds::Uint64> installedTitleIds;
	m_isUpdatingList = true;

	m_installedTitleIds.clear();
	m_installedItems.clear();
	m_expandedItem = nullptr;
	TweenEngine::Tween::set(m_options, InstalledOptions::ALPHA).target(0.f).start(m_tweenManager);

#ifdef EMULATION
	// some hardcoded title IDs for testing
	installedTitleIds.emplace_back(0x00040000000edf00); // [US] Super Smash Bros.
	installedTitleIds.emplace_back(0x0004000e000edf00); // [US] Super Smash Bros. [UPDATE]
	installedTitleIds.emplace_back(0x0004008c000edf00); // [US] Super Smash Bros. [DLC]
	installedTitleIds.emplace_back(0x00040002000edf01); // [US] Super Smash Bros. [DEMO]
	installedTitleIds.emplace_back(0x0004001000021800); // [US] StreetPass Mii Plaza
	installedTitleIds.emplace_back(0x0004000000030800); // [US] Mario Kart 7
	installedTitleIds.emplace_back(0x0004000000041700); // [US] Kirby's Dream Land
	installedTitleIds.emplace_back(0x0004008000008f00); // [US] Home Menu
	installedTitleIds.emplace_back(0x0004000000162000); // [EU] Project X Zone 2
	installedTitleIds.emplace_back(0x000400000017a400); // [EU] Fire Emblem Fates
	installedTitleIds.emplace_back(0x00040000001b7800); // [US] Pic-a-Pix Color
#else
	u32 titleCount;
	AM_GetTitleCount(MEDIATYPE_SD, &titleCount);
	installedTitleIds.resize(titleCount);
	AM_GetTitleList(nullptr, MEDIATYPE_SD, titleCount, &installedTitleIds[0]);
	AM_GetTitleCount(MEDIATYPE_NAND, &titleCount);
	installedTitleIds.resize(titleCount + installedTitleIds.size());
	AM_GetTitleList(nullptr, MEDIATYPE_NAND, titleCount, &installedTitleIds[installedTitleIds.size() - titleCount]);

	FS_CardType type;
	bool cardInserted;
	m_cardInserted = (R_SUCCEEDED(FSUSER_CardSlotIsInserted(&cardInserted)) && cardInserted && R_SUCCEEDED(FSUSER_GetCardType(&type)) && type == CARD_CTR);
	if (m_cardInserted)
	{
		// Retry a bunch of times. When the card is newly inserted,
		// it sometimes takes a short while before title can be read.
		int retryCount = 100;
		u64 cardTitleId;
		while (retryCount-- > 0)
			if (R_SUCCEEDED(AM_GetTitleList(nullptr, MEDIATYPE_GAME_CARD, 1, &cardTitleId)))
			{
				try
				{
					std::shared_ptr<InstalledItem> item(new InstalledItem(cardTitleId));
					m_installedItems.emplace_back(std::move(item));
				}
				catch (int e)
				{
					//
				}
				break;
			}
			else
				cpp3ds::sleep(cpp3ds::milliseconds(5));
	}
#endif

	for (auto& titleId : installedTitleIds)
	{
		size_t titleType = titleId >> 32;
		if (titleType == TitleKeys::Game || titleType == TitleKeys::Update ||
				titleType == TitleKeys::DLC || titleType == TitleKeys::Demo || titleType == TitleKeys::DSiWare ||
				titleType == TitleKeys::SystemApplet || titleType == TitleKeys::SystemApplication)
			m_installedTitleIds.push_back(titleId);
	}

	// Add all primary game titles first
	for (auto& titleId : installedTitleIds)
	{
		TitleKeys::TitleType titleType = static_cast<TitleKeys::TitleType>(titleId >> 32);
		if (titleType == TitleKeys::Game || titleType == TitleKeys::DSiWare || titleType == TitleKeys::SystemApplication || titleType == TitleKeys::SystemApplet)
			try
			{
				std::shared_ptr<InstalledItem> item(new InstalledItem(titleId));
				m_installedItems.emplace_back(std::move(item));
			}
			catch (int e)
			{
				//std::cout << "Error while adding: " << titleId << std::endl;
			}
	}

	// Add updates that have not yet been installed for which we have a titlekey
	for (auto& titleId : TitleKeys::getIds())
	{
		size_t titleType = titleId >> 32;
		if (titleType == TitleKeys::Update || titleType == TitleKeys::DLC)
		{
			size_t titleLower = (titleId & 0xFFFFFFFF) >> 8;
			for (auto& installedItem : m_installedItems)
			{
				if (titleLower == (installedItem->getTitleId() & 0xFFFFFFFF) >> 8)
				{
					if (titleType == TitleKeys::Update)
						installedItem->setUpdateStatus(titleId, false);
					else
						installedItem->setDLCStatus(titleId, false);
				}
			}
		}
	}

	// Add all installed updates/DLC to the above titles added.
	// If not found, attempt to fetch parent title info from system.
	for (auto& titleId : installedTitleIds)
	{
		TitleKeys::TitleType titleType = static_cast<TitleKeys::TitleType>(titleId >> 32);
		if (titleType == TitleKeys::Update || titleType == TitleKeys::DLC)
		{
			cpp3ds::Uint32 titleLower = (titleId & 0xFFFFFFFF) >> 8;
			for (auto& installedItem : m_installedItems)
			{
				if (titleLower == (installedItem->getTitleId() & 0xFFFFFFFF) >> 8)
				{
					if (titleType == TitleKeys::Update)
						installedItem->setUpdateStatus(titleId, true);
					else
						installedItem->setDLCStatus(titleId, true);
					break;
				}
			}
		}
	}

	// Remove all system titles that have no Update or DLC
	for (auto it = m_installedItems.begin(); it != m_installedItems.end();)
	{
		size_t titleType = (*it)->getTitleId() >> 32;
		if ((titleType != TitleKeys::SystemApplication && titleType != TitleKeys::SystemApplet) ||
				((*it)->hasDLC() || (*it)->hasDLC()))
			it++;
		else
			it = m_installedItems.erase(it);
	}

	// For too long title name, shorten them
	for (auto& installedItem : m_installedItems)
	{
		installedItem->updateGameTitle();
	}

	// Copy the content of the installed list into the shown installed list
	m_installedItemsFiltered = m_installedItems;

	// Remove all titles that are not searched
	if (!m_currentSearch.empty()) {
		for (auto it = m_installedItemsFiltered.begin(); it != m_installedItemsFiltered.end();)
		{
			int matchScore;
			if (!fts::fuzzy_match(m_currentSearch.c_str(), (*it)->getAppItem()->getNormalizedTitle().c_str(), matchScore))
				it = m_installedItemsFiltered.erase(it);
			else
				it++;
		}
	}

	// Sort the list
	sort();

	m_isUpdatingList = false;
}

void InstalledList::draw(cpp3ds::RenderTarget &target, cpp3ds::RenderStates states) const
{
	if (!m_canDrawList)
		return;

	states.transform *= getTransform();
	states.scissor = cpp3ds::UintRect(0, 50, 320, 190);

	for (auto& item : m_installedItemsFiltered)
	{
		float posY = item->getPosition().y;
		if (posY > -10.f && posY < 240.f)
			target.draw(*item, states);
	}

	if (m_expandedItem)
		target.draw(m_options, states);
}

void InstalledList::update(float delta)
{
#ifdef _3DS
	FS_CardType type;
	bool cardInserted;
	if (m_cardInserted != (R_SUCCEEDED(FSUSER_CardSlotIsInserted(&cardInserted)) && cardInserted && R_SUCCEEDED(FSUSER_GetCardType(&type)) && type == CARD_CTR))
		refresh();
#endif
	m_tweenManager.update(delta);
}

bool InstalledList::processEvent(const cpp3ds::Event &event)
{
	if (m_tweenManager.getRunningTweensCount() > 0)
		return false;

	if (event.type == cpp3ds::Event::TouchEnded)
	{
		if (event.touch.y < 30)
			return false;

			if (cpp3ds::UintRect(0, 32, 320, 16).contains(event.touch.x, event.touch.y)) {
#ifndef EMULATION
				KeyboardApplet kb(KeyboardApplet::Text);
				swkbdSetHintText(kb, _("Type a game name...").toAnsiString().c_str());

				cpp3ds::String input = kb.getInput();
				filterBySearch(input);
#else
				filterBySearch("");
#endif
				return false;
			}

		for (auto &item : m_installedItemsFiltered)
		{
			float posY = getPosition().y + item->getPosition().y;
			if (event.touch.y > posY && event.touch.y < posY + item->getHeight())
			{
				if (item.get() == m_expandedItem)
				{
					if (event.touch.y < posY + 16.f)
						expandItem(nullptr);
					else
						m_options.processTouchEvent(event);
				}
				else
					expandItem(item.get());
				break;
			}
		}
	}
	return false;
}

void InstalledList::setScroll(float position)
{
	m_scrollPos = std::round(position);
	repositionItems();
}

float InstalledList::getScroll()
{
	return m_scrollPos;
}

void InstalledList::repositionItems()
{
	float posY = 50.f + m_scrollPos;
	for (auto& item : m_installedItemsFiltered)
	{
		if (item.get() == m_expandedItem)
			m_options.setPosition(0.f, posY + 20.f);
		item->setPosition(0.f, posY);
		posY += item->getHeight();
	}
	m_size.y = posY - 6.f - m_scrollPos;
	if (m_expandedItem)
		m_size.y -= 24.f;
	updateScrollSize();
}

const cpp3ds::Vector2f &InstalledList::getScrollSize()
{
	return m_size;
}

void InstalledList::expandItem(InstalledItem *item)
{
	if (item == m_expandedItem)
		return;

	const float optionsFadeDelay = 0.15f;
	const float expandDuration = 0.2f;

	// Expand animation
	if (m_expandedItem)
	{
		TweenEngine::Tween::to(*m_expandedItem, InstalledItem::HEIGHT, expandDuration)
			.target(16.f)
			.setCallback(TweenEngine::TweenCallback::COMPLETE, [=](TweenEngine::BaseTween* source) {
				m_expandedItem = item;
				repositionItems();
			})
			.delay(optionsFadeDelay)
			.start(m_tweenManager);
		TweenEngine::Tween::to(m_options, InstalledOptions::ALPHA, optionsFadeDelay + 0.05f)
			.target(0.f)
			.start(m_tweenManager);
	}
	if (item)
	{
		TweenEngine::Tween::to(*item, InstalledItem::HEIGHT, expandDuration)
			.target(40.f)
			.setCallback(TweenEngine::TweenCallback::COMPLETE, [=](TweenEngine::BaseTween* source) {
				m_expandedItem = item;
				m_options.setInstalledItem(item);
				repositionItems();
			})
			.delay(m_expandedItem ? optionsFadeDelay : 0.f)
			.start(m_tweenManager);
		TweenEngine::Tween::to(m_options, InstalledOptions::ALPHA, expandDuration)
			.target(255.f)
			.delay(m_expandedItem ? expandDuration + optionsFadeDelay : optionsFadeDelay)
			.start(m_tweenManager);
	}

	// Move animation for items in between expanded items
	bool foundItem = false;
	bool foundExpanded = false;
	for (auto &itemToMove : m_installedItemsFiltered)
	{
		if (foundItem && !foundExpanded)
		{
			TweenEngine::Tween::to(*itemToMove, InstalledItem::POSITION_Y, expandDuration)
				.targetRelative(24.f)
				.delay(m_expandedItem ? optionsFadeDelay : 0.f)
				.start(m_tweenManager);
		}
		else if (foundExpanded && !foundItem)
		{
			TweenEngine::Tween::to(*itemToMove, InstalledItem::POSITION_Y, expandDuration)
				.targetRelative(-24.f)
				.delay(m_expandedItem ? optionsFadeDelay : 0.f)
				.start(m_tweenManager);
		}

		if (itemToMove.get() == m_expandedItem)
			foundExpanded = true;
		else if (itemToMove.get() == item)
		{
			foundItem = true;
		}

		if (foundExpanded && foundItem)
			break;
	}
}

bool InstalledList::isInstalled(cpp3ds::Uint64 titleId)
{
	auto &v = getInstance().m_installedTitleIds;
	return std::find(v.begin(), v.end(), titleId) != v.end();
}

int InstalledList::getGameCount()
{
	if (!m_isUpdatingList)
		m_gameCount = m_installedItems.size();

	return m_gameCount;
}

void InstalledList::sort()
{
	m_tweenManager.killAll();
	std::sort(m_installedItemsFiltered.begin(), m_installedItemsFiltered.end(), [&](const std::shared_ptr<InstalledItem>& a, const std::shared_ptr<InstalledItem>& b)
	{
		if (m_sortAscending)
		{
			switch(m_sortType) {
				case Name: return a->getAppItem()->getNormalizedTitle() < b->getAppItem()->getNormalizedTitle();
				case Size: return a->getAppItem()->getFilesize() < b->getAppItem()->getFilesize();
				case VoteScore: return a->getAppItem()->getVoteScore() < b->getAppItem()->getVoteScore();
				case VoteCount: return a->getAppItem()->getVoteCount() < b->getAppItem()->getVoteCount();
				case ReleaseDate: return a->getAppItem()->getReleaseDate() < b->getAppItem()->getReleaseDate();
			}
		}
		else
		{
			switch(m_sortType) {
				case Name: return a->getAppItem()->getNormalizedTitle() > b->getAppItem()->getNormalizedTitle();
				case Size: return a->getAppItem()->getFilesize() > b->getAppItem()->getFilesize();
				case VoteScore: return a->getAppItem()->getVoteScore() > b->getAppItem()->getVoteScore();
				case VoteCount: return a->getAppItem()->getVoteCount() > b->getAppItem()->getVoteCount();
				case ReleaseDate: return a->getAppItem()->getReleaseDate() > b->getAppItem()->getReleaseDate();
			}
		}
	});

	repositionItems();
}

void InstalledList::setSortType(InstalledList::SortType sortType, bool ascending)
{
	m_sortType = sortType;
	m_sortAscending = ascending;
	sort();
}

void InstalledList::filterBySearch(std::string search)
{
	if (search != m_currentSearch) {
		m_currentSearch = search;

		g_browseState->setInstalledListSearchText(m_currentSearch);

		// Copy the content of the installed list into the shown installed list
		m_installedItemsFiltered = m_installedItems;

		// Remove all titles that are not searched
		if (!m_currentSearch.empty()) {
			for (auto it = m_installedItemsFiltered.begin(); it != m_installedItemsFiltered.end();)
			{
				int matchScore;
				if (!fts::fuzzy_match(m_currentSearch.c_str(), (*it)->getAppItem()->getNormalizedTitle().c_str(), matchScore))
					it = m_installedItemsFiltered.erase(it);
				else
					it++;
			}
		}

		sort();
		expandItem(nullptr);
	}
}

void InstalledList::setDrawList(bool canDrawList)
{
	m_canDrawList = canDrawList;
}

bool InstalledList::canDrawList()
{
	return m_canDrawList;
}

} // namespace FreeShop
