#include <scene_graph/scene_graph.hpp>

namespace vkb
{

NodePtr Node::create(NodePtr parent)
{
	auto ptr = std::shared_ptr<Node>(new Node());

	if (!parent)
	{
		SceneGraph::get()->add_root(ptr);
		return ptr;
	}

	ptr->reparent(parent);

	return ptr;
}

Node::Node()
{
	auto *graph = SceneGraph::get();
	m_entity    = graph->m_registry->create();
}

NodePtr Node::parent() const
{
	return m_parent;
}

std::vector<NodePtr> Node::children() const
{
	return m_children;
}

entt::entity Node::entity() const
{
	return m_entity;
}

Transform &Node::transform()
{
	return m_transform;
}

const Transform &Node::transform() const
{
	return m_transform;
}

void Node::reparent(NodePtr new_parent)
{
	if (m_parent)
	{
		m_parent->m_children.erase(std::remove(m_parent->m_children.begin(), m_parent->m_children.end(), shared_from_this()), m_parent->m_children.end());
	}

	m_parent = new_parent;

	if (m_parent)
	{
		m_parent->m_children.push_back(shared_from_this());
	}
	else
	{
		SceneGraph::get()->add_root(shared_from_this());
	}
}

const glm::mat4 &Node::world_matrix() const
{
	// Calculate the local transform matrix
	glm::mat4 matrix = glm::translate(m_transform.translation) * glm::mat4_cast(m_transform.rotation) * glm::scale(m_transform.scale);

	// Calculate the world matrix of the parent node and multiply it by the local transform of the current node
	m_world_matrix = m_parent ? m_parent->world_matrix() * matrix : matrix;

	return m_world_matrix;
}

SceneGraph *SceneGraph::get()
{
	static SceneGraph instance;
	return &instance;
}

SceneGraph::SceneGraph() :
    m_registry{make_registry()}
{}

void SceneGraph::reset()
{
	m_registry->clear();
	m_roots.clear();
}

RegistryPtr SceneGraph::registry() const
{
	return m_registry;
}

std::vector<NodePtr> SceneGraph::roots() const
{
	return {m_roots.begin(), m_roots.end()};
}

void SceneGraph::add_root(NodePtr root)
{
	m_roots.emplace(root);
}

void SceneGraph::remove_root(NodePtr root)
{
	m_roots.erase(root);
}

}        // namespace vkb