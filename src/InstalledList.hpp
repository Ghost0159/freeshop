#ifndef FREESHOP_INSTALLEDLIST_HPP
#define FREESHOP_INSTALLEDLIST_HPP

#include <TweenEngine/TweenManager.h>
#include <cpp3ds/Window/Event.hpp>
#include <cpp3ds/System/Mutex.hpp>
#include "InstalledItem.hpp"
#include "InstalledOptions.hpp"
#include "TweenObjects.hpp"
#include "GUI/Scrollable.hpp"
#include "States/BrowseState.hpp"

namespace FreeShop {

class InstalledList : public cpp3ds::Drawable, public Scrollable, public util3ds::TweenTransformable<cpp3ds::Transformable>  {
public:
	enum SortType {
		Name,
		Size,
		VoteScore,
		VoteCount,
		ReleaseDate,
	};

	static InstalledList &getInstance();
	void refresh();
	void setSortType(SortType sortType, bool ascending);

	void filterBySearch(std::string search);

	void update(float delta);
	bool processEvent(const cpp3ds::Event& event);

	virtual void setScroll(float position);
	virtual float getScroll();
	virtual const cpp3ds::Vector2f &getScrollSize();

	static bool isInstalled(cpp3ds::Uint64 titleId);
	std::vector<std::shared_ptr<InstalledItem>> &getList() { return m_installedItems; }

	void setDrawList(bool canDrawList);
	bool canDrawList();

	int getGameCount();

protected:
	InstalledList();
	virtual void draw(cpp3ds::RenderTarget& target, cpp3ds::RenderStates states) const;
	void repositionItems();
	void expandItem(InstalledItem *item);
	void sort();

private:
	cpp3ds::Mutex m_mutexRefresh;
	bool m_cardInserted;
	std::vector<cpp3ds::Uint64> m_installedTitleIds;
	std::vector<std::shared_ptr<InstalledItem>> m_installedItems;
	std::vector<std::shared_ptr<InstalledItem>> m_installedItemsFiltered;
	TweenEngine::TweenManager m_tweenManager;
	float m_scrollPos;
	cpp3ds::Vector2f m_size;
	InstalledItem *m_expandedItem;

	InstalledOptions m_options;

	bool m_isUpdatingList;
	int m_gameCount;
	bool m_canDrawList;

	SortType m_sortType;
	bool m_sortAscending;

	std::string m_currentSearch;

};

} // namespace FreeShop

#endif // FREESHOP_INSTALLEDLIST_HPP
