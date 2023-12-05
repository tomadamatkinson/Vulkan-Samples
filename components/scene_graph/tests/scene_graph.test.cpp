#include <catch2/catch_test_macros.hpp>

#include <scene_graph/scene_graph.hpp>

inline void reset()
{
	vkb::sg::SceneGraph::get()->reset();
}

TEST_CASE("SceneGraph", "[scene_graph]")
{
	SECTION("When a scene graph is created, it should have a valid registry")
	{
		auto *scene_graph = vkb::sg::SceneGraph::get();
		REQUIRE(scene_graph != nullptr);

		auto registry = scene_graph->registry();
		REQUIRE(registry != nullptr);

		reset();
	}

	SECTION("When a node is created it should be added as a root node")
	{
		auto parent = vkb::sg::Node::create();
		REQUIRE(vkb::sg::SceneGraph::get()->roots().size() == 1);

		reset();
	}

	SECTION("When a node is created with a parent it should be added as a child node")
	{
		auto parent = vkb::sg::Node::create();
		auto child  = vkb::sg::Node::create(parent);

		// Only one node is the root node (parent)
		REQUIRE(vkb::sg::SceneGraph::get()->roots().size() == 1);

		REQUIRE(child->parent() == parent);
		REQUIRE(parent->children().size() == 1);
		REQUIRE(parent->children()[0] == child);

		reset();
	}
}

TEST_CASE("Transform", "[scene_graph]")
{
	SECTION("When a transform is created, it should have a valid translation, rotation and scale")
	{
		auto transform = vkb::sg::Transform();
		REQUIRE(transform.translation == glm::vec3(0.0, 0.0, 0.0));
		REQUIRE(transform.rotation == glm::quat(1.0, 0.0, 0.0, 0.0));
		REQUIRE(transform.scale == glm::vec3(1.0, 1.0, 1.0));
	}

	SECTION("When a transform is created, it should have a valid world matrix")
	{
		auto node = vkb::sg::Node::create();

		auto &transform       = node->transform();
		transform.translation = glm::vec3(1.0, 2.0, 3.0);
		transform.rotation    = glm::quat(1.0, 0.0, 0.0, 0.0);
		transform.scale       = glm::vec3(1.0, 1.0, 1.0);

		glm::mat4 expected_world_matrix = glm::translate(glm::vec3(1.0, 2.0, 3.0)) * glm::mat4_cast(glm::quat(1.0, 0.0, 0.0, 0.0)) * glm::scale(glm::vec3(1.0, 1.0, 1.0));

		REQUIRE(node->world_matrix() == expected_world_matrix);

		reset();
	}

	SECTION("Require world matrix to be calculated using parent", "[scene_graph]")
	{
		auto parent = vkb::sg::Node::create();
		auto child  = vkb::sg::Node::create(parent);

		auto &parent_transform       = parent->transform();
		parent_transform.translation = glm::vec3(1.0, 2.0, 3.0);
		parent_transform.rotation    = glm::quat(1.0, 0.0, 0.0, 0.0);
		parent_transform.scale       = glm::vec3(1.0, 1.0, 1.0);

		auto &child_transform       = child->transform();
		child_transform.translation = glm::vec3(3.0, 2.0, 1.0);
		child_transform.rotation    = glm::quat(1.0, 0.0, 2.0, 0.0);
		child_transform.scale       = glm::vec3(12.0, 0.3, 11.0);

		glm::mat4 parent_matrix = glm::translate(glm::vec3(1.0, 2.0, 3.0)) * glm::mat4_cast(glm::quat(1.0, 0.0, 0.0, 0.0)) * glm::scale(glm::vec3(1.0, 1.0, 1.0));
		glm::mat4 child_matrix  = glm::translate(glm::vec3(3.0, 2.0, 1.0)) * glm::mat4_cast(glm::quat(1.0, 0.0, 2.0, 0.0)) * glm::scale(glm::vec3(12.0, 0.3, 11.0));

		// Parent * Child
		REQUIRE(child->world_matrix() == parent_matrix * child_matrix);

		// Just Parent
		REQUIRE(parent->world_matrix() == parent_matrix);

		reset();
	}
}

TEST_CASE("Node", "[scene_graph]")
{
	SECTION("When a node is created, it should have a valid entity")
	{
		auto node = vkb::sg::Node::create();
		REQUIRE((node->entity() != entt::null));

		reset();
	}

	SECTION("A node can have several children")
	{
		auto child_1 = vkb::sg::Node::create();
		auto child_2 = vkb::sg::Node::create();
		auto child_3 = vkb::sg::Node::create();

		auto parent = vkb::sg::Node::create();

		child_1->reparent(parent);
		child_2->reparent(parent);
		child_3->reparent(parent);

		REQUIRE(parent->children().size() == 3);
		REQUIRE(parent->children()[0] == child_1);
		REQUIRE(parent->children()[1] == child_2);
		REQUIRE(parent->children()[2] == child_3);

		reset();
	}

	SECTION("A node can have a parent")
	{
		auto child  = vkb::sg::Node::create();
		auto parent = vkb::sg::Node::create();

		child->reparent(parent);

		REQUIRE((child->parent() == parent));

		reset();
	}

	SECTION("A node can be reparented")
	{
		auto child   = vkb::sg::Node::create();
		auto parent1 = vkb::sg::Node::create();
		auto parent2 = vkb::sg::Node::create();

		child->reparent(parent1);
		REQUIRE(child->parent() == parent1);
		REQUIRE(parent1->children().size() == 1);
		REQUIRE(parent1->children()[0] == child);
		REQUIRE(parent2->children().size() == 0);

		child->reparent(parent2);
		REQUIRE(child->parent() == parent2);
		REQUIRE(parent1->children().size() == 0);
		REQUIRE(parent2->children().size() == 1);
		REQUIRE(parent2->children()[0] == child);

		reset();
	}
}