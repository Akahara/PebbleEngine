#pragma once

#include <algorithm>
#include <stdexcept>
#include <vector>
#include <concepts>

/*
 * A simple QuadTree implementation.
 *
 * Objects are stored in the quad tree, if the quad-tree must be non-owning
 * simply store pointers to your objects in the tree.
 * There is no method to remove an object from the quad tree or move an object
 * somewhere else. For game purposes fully clearing the tree and adding all
 * objects back in each frame is more than enough. You may want to have two
 * active quad trees, one for static objects that doesn't get cleared and the
 * other that gets rebuilt each frame.
 */

/* A quad tree region, similar to a 2D rect */
struct QuadTreeRegion {
  float minX{}, minY{}, maxX{}, maxY{};

  bool isDegenerate() const { return maxX <= minX || maxY <= minY; }

  constexpr bool overlaps(const QuadTreeRegion &other) const
  {
    return maxX >= other.minX && minX <= other.maxX
      && maxY >= other.minY && minY <= other.maxY;
  }

  constexpr bool contains(const QuadTreeRegion &other) const
  {
    return maxX >= other.maxX && minX <= other.minX
      && maxY >= other.maxY && minY <= other.minY;
  }
};

/*
 * For quad tree queries, a quad tree search function must be provided.
 *
 * A valid search function returns a pair of booleans <C, O> with C set if
 * the search *contains* the tree region and O set if the search overlaps at
 * at least one point the tree region. If the search fully contains a tree
 * region all its elements can be collected by the query without calling the
 * search function for each. If C is too heavy of a computation it can be set
 * to false, the query will just take a bit longer. If C is set there is no
 * need to compute O.
 *
 * Custom search functions can be very usefull when searching in non-rectangular
 * regions such as circles, frustums and such.
 */
template<class T>
constexpr bool is_quad_tree_search_v = std::is_invocable_r_v<std::pair<bool, bool>, T, const QuadTreeRegion &>;
template<class T, class K>
concept is_container_v = requires(T container, K element) { { container.push_back(element) }; };

/*
 * A Quad Tree of elements of type T.
 *
 * A TRegionMapper must be provided, it will be used at each insertion to
 * compute the region that the added object spans.
 *
 * The quad tree spans over a region that must be provided uppon creation,
 * there is no way of resizing or moving that region.
 *
 * To search in a quad tree, use one of the collectXX methods, most take
 * a "Mapper" argument that can be used to map contained objects to another
 * type (like collecting pointers instead of copies). For complex quad tree
 * queries, see is_quad_tree_search_v.
 */
template<class T, class TRegionMapper>
  requires std::is_invocable_r_v<QuadTreeRegion, TRegionMapper, T>
class QuadTree
{
public:
  using value_type = T;
  static constexpr size_t MAX_MEMBER_PER_CELL = 5;
  static constexpr size_t SUBCELLS_COUNT = 4;
  static constexpr size_t SUBCELLS_BUCKET_SIZE = 32;

private:
  using placed_type = std::pair<value_type, QuadTreeRegion>;
  using subcells_index = uint16_t;

  struct QuadTreeCell {
  public:
    void setRegion(const QuadTreeRegion &region) { m_region = region; }
    const QuadTreeRegion &getRegion() const { return m_region; }

    void reset()
    {
      m_members.clear();
      m_subcells = nullptr;
    }

    void addMember(QuadTree &tree, placed_type &&member)
    {
      if (!m_subcells && m_members.size() == MAX_MEMBER_PER_CELL)
        subdivideSelf(tree);
      if (!m_subcells || !insertInSubcells(tree, member))
        m_members.push_back(std::move(member));
    }

    template<class Container, class Search, class Mapper>
    void collectInShape(Container &collected, const Search &search, const Mapper &func)
    {
      for(auto &[val, reg] : m_members)
      {
        auto [contains, overlaps] = search(reg);
        if (contains || overlaps)
          collected.push_back(func(val));
      }
      visitIntersectedSubcells(search,
        [&](QuadTreeCell &cell) { cell.collectInShape(collected, search, func); },
        [&](QuadTreeCell &cell) { cell.collectAll(collected, func); });
    }

    template<class Container, class Mapper>
    void collectAll(Container &collected, const Mapper &func) 
    {
      std::ranges::transform(m_members, std::back_inserter(collected), func, [](placed_type &p) { return p.first; });
      if(m_subcells) std::ranges::for_each_n(m_subcells, SUBCELLS_COUNT, [&](auto &cell) { cell.collectAll(collected, func); });
    }

  private:
    template<class Search, class OverlapedVisitor, class ContainedVisitor>
      requires is_quad_tree_search_v<Search>
    void visitIntersectedSubcells(const Search &search, OverlapedVisitor overlapedVisitor, ContainedVisitor containedVisitor)
    {
      if (!m_subcells) return;
      for(size_t i = 0; i < SUBCELLS_COUNT; i++) {
        auto [contains, overlaps] = search(m_subcells[i].m_region);
        if (contains)
          containedVisitor(m_subcells[i]);
        else
          overlapedVisitor(m_subcells[i]);
      }
    }

    bool insertInSubcells(QuadTree &tree, placed_type &member)
    {
      QuadTreeRegion &region = member.second;
      float midX = (m_region.minX+m_region.maxX)*.5f;
      float midY = (m_region.minY+m_region.maxY)*.5f;
      if(region.maxX < midX && region.maxY < midY) {
        m_subcells[0].addMember(tree, std::move(member));
      } else if(region.minX >= midX && region.maxY < midY) {
        m_subcells[1].addMember(tree, std::move(member));
      } else if(region.maxX < midX && region.minY >= midY) {
        m_subcells[2].addMember(tree, std::move(member));
      } else if(region.minX >= midX && region.minY >= midY) {
        m_subcells[3].addMember(tree, std::move(member));
      } else {
        return false;
      }
      return true;
    }

    void subdivideSelf(QuadTree &tree)
    {
      m_subcells = tree.allocSubcellsSet(m_region);
      float midX = (m_region.minX+m_region.maxX)*.5f;
      float midY = (m_region.minY+m_region.maxY)*.5f;
      //  2 | 3    y
      // ---|---   ^  
      //  0 | 1    |->x
      m_subcells[0].m_region.maxX = midX; m_subcells[0].m_region.maxY = midY;
      m_subcells[1].m_region.minX = midX; m_subcells[1].m_region.maxY = midY;
      m_subcells[2].m_region.maxX = midX; m_subcells[2].m_region.minY = midY;
      m_subcells[3].m_region.minX = midX; m_subcells[3].m_region.minY = midY;

      // redistribute current items
      std::erase_if(m_members, [&](placed_type &placed) { return insertInSubcells(tree, placed); });
    }

  private:
    QuadTreeRegion           m_region;
    using placed_type = std::pair<value_type, QuadTreeRegion>;
    std::vector<placed_type> m_members;
    QuadTreeCell            *m_subcells = nullptr;
  };

public:
  explicit QuadTree(QuadTreeRegion spanningRegion, TRegionMapper regionMapper={})
    : m_regionMapper(std::move(regionMapper))
    , m_lastBucketSize(SUBCELLS_BUCKET_SIZE)
  {
    m_rootCell.setRegion(spanningRegion);
  }

  const QuadTreeRegion &getSpanningRegion() const { return m_rootCell.getRegion(); }

  void clear()
  {
    m_lastBucketSize = m_cellsBuffer.empty() ? SUBCELLS_BUCKET_SIZE : 0;
    m_lastBucket = m_cellsBuffer.empty() ? -1 : 0;
    std::ranges::for_each(m_cellsBuffer,
      [](auto &bucket) { std::ranges::for_each(*bucket, [](auto &cell) { cell.reset(); }); });
    m_rootCell.reset();
  }

  void add(value_type val)
  {
    QuadTreeRegion valRegion = m_regionMapper(val);

    if (!valRegion.overlaps(getSpanningRegion()))
      throw std::runtime_error("Tried to add an item outside the valid range of a quad tree");

    m_rootCell.addMember(*this, std::make_pair(std::move(val), valRegion));
  }

  template<class Collected=value_type, class Container, class Mapper=std::identity>
    requires std::is_invocable_r_v<Collected, Mapper, value_type>
         and is_container_v<Container, Collected>
  void collectInBounds(Container &container, const QuadTreeRegion &region, Mapper func={})
  {
    m_rootCell.collectInShape(container, [&](const QuadTreeRegion &r) -> std::pair<bool, bool> {
      bool contains = region.contains(r);
      return { contains, contains || region.overlaps(r) };
    }, func);
  }

  template<class Container>
    requires is_container_v<Container, value_type*>
  void collectRefInBounds(Container &container, const QuadTreeRegion &region) 
  {
    return collectInBounds(container, region, [](value_type &val) { return &val; });
  }

  template<class Collected, class Container, class Search, class Mapper=std::identity>
    requires std::is_invocable_r_v<Collected, Mapper, value_type>
         and is_container_v<Container, Collected>
         and is_quad_tree_search_v<Search>
  void collectMapInShape(Container &container, const Search &search, const Mapper &func={})
  {
    m_rootCell.collectInShape(container, search, func);
  }

private:
  friend QuadTreeCell;
  QuadTreeCell *allocSubcellsSet(const QuadTreeRegion &parentRegion)
  {
    if (m_lastBucketSize == SUBCELLS_BUCKET_SIZE) {
      if(m_lastBucket+1 == m_cellsBuffer.size())
        m_cellsBuffer.emplace_back(std::make_unique<std::array<QuadTreeCell, SUBCELLS_COUNT * SUBCELLS_BUCKET_SIZE>>());
      m_lastBucketSize = 0;
      m_lastBucket++;
    }
    QuadTreeCell *firstCell = &m_cellsBuffer[m_lastBucket]->at(SUBCELLS_COUNT * (m_lastBucketSize++));
    for (size_t i = 0; i < SUBCELLS_COUNT; i++)
      firstCell[i].setRegion(parentRegion);

    return firstCell;
  }

private:
  TRegionMapper             m_regionMapper;
  std::vector<std::unique_ptr<std::array<QuadTreeCell, SUBCELLS_COUNT * SUBCELLS_BUCKET_SIZE>>> m_cellsBuffer;
  size_t                    m_lastBucketSize;
  size_t                    m_lastBucket = -1;
  QuadTreeCell              m_rootCell;
};
