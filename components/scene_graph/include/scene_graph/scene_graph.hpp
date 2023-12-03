#pragma once

#include <set>
#include <vector>

#include <scene_graph/entt.hpp>

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/transform.hpp>

namespace vkb
{

struct Transform
{
	glm::vec3 translation = glm::vec3(0.0, 0.0, 0.0);
	glm::quat rotation    = glm::quat(1.0, 0.0, 0.0, 0.0);
	glm::vec3 scale       = glm::vec3(1.0, 1.0, 1.0);
};

using NodePtr = std::shared_ptr<class Node>;

class Node : public std::enable_shared_from_this<Node>
{
  public:
	static NodePtr create(NodePtr parent = nullptr);

	virtual ~Node() = default;

	NodePtr              parent() const;
	std::vector<NodePtr> children() const;
	entt::entity         entity() const;
	const glm::mat4     &world_matrix() const;

	Transform       &transform();
	const Transform &transform() const;

	// Reparents the node to a new parent
	// If the new parent is null, the node will be added as a root node
	// If the new parent is not null, the node will be added as a child of the new parent
	void reparent(NodePtr new_parent);

  protected:
	Node();

	entt::entity m_entity;

	NodePtr              m_parent{nullptr};
	std::vector<NodePtr> m_children;

  private:
	RegistryPtr m_registry;

	Transform         m_transform;
	mutable glm::mat4 m_world_matrix;
};

// A scene graph represents all nodes loaded into a sample
// It is a singleton, so it can be accessed from anywhere
// When a node is created without a parent, it is added as a root node
class SceneGraph
{
  public:
	friend class Node;

	static SceneGraph *get();

	// Resets the scene graph
	// This will destroy all nodes and their entities
	// This should be called if you want a complete reset of the scene graph
	void reset();

	RegistryPtr registry() const;

	std::vector<NodePtr> roots() const;

  private:
	SceneGraph();

	void add_root(NodePtr root);
	void remove_root(NodePtr root);

	std::set<NodePtr> m_roots;

	RegistryPtr m_registry;
};

}        // namespace vkb