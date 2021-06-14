#include "APT.h"
#include <cmath>
#include <random>
#include "glm/glm.hpp"
namespace APT
{
	const char *ToC_str (NODE_TYPE type)
	{
		switch (type) {
			case NODE_TYPE::Max:
				return "Max(x, y)";
			case NODE_TYPE::Min:
				return "Min(x, y)";
			case NODE_TYPE::arcTan2:
				return "arcTan2(x, y)";
			case NODE_TYPE::Sin:
				return "Sin(x)";
			case NODE_TYPE::Cos:
				return "Cos(x)";
			case NODE_TYPE::Tan:
				return "Tan(x)";
			case NODE_TYPE::Lerp:
				return "Lerp(x, y, z)";
			case NODE_TYPE::Atan:
				return "Atan(x)";
			case NODE_TYPE::Plus:
				return "+ (x, y)";
			case NODE_TYPE::Minus:
				return "- (x, y)";
			case NODE_TYPE::Mult:
				return "* (x, y)";
			case NODE_TYPE::Div:
				return "/ (x, y)";
			case NODE_TYPE::Negate:
				return "- (x)";
			case NODE_TYPE::Square:
				return "Sqr (x)";
			case NODE_TYPE::Ceil:
				return "Ceil (x)";
			case NODE_TYPE::Log2:
				return "Log2 (x)";
			case NODE_TYPE::Abs:
				return "Abs (x)";
			case NODE_TYPE::Clip:
				return "Clip (x, y)";
			case NODE_TYPE::Floor:
				return "Floor (x)";
			case NODE_TYPE::Wrap:
				return "Wrap (x)";
			case NODE_TYPE::Const:
				return "Const";
			case NODE_TYPE::OpX:
				return "OpX()";
			case NODE_TYPE::OpY:
				return "OpY()";
			default:
				LOG_ERROR ("Unknown Node Type");
				return "Invalid";
		}
	}
	std::optional<APT::BaseNode> CreateNode (const NODE_TYPE type)
	{
		switch (type) 	{
			case NODE_TYPE::Max:
				return { BaseNode (type, NODE_CATEGORY::Branch_2) };
			case NODE_TYPE::Min:  
				return { BaseNode (type, NODE_CATEGORY::Branch_2) };
			case NODE_TYPE::arcTan2:
				return { BaseNode (type, NODE_CATEGORY::Branch_2) };
			case NODE_TYPE::Sin:  
				return { BaseNode (type, NODE_CATEGORY::Branch_1) };
			case NODE_TYPE::Cos:  
				return { BaseNode (type, NODE_CATEGORY::Branch_1) };
			case NODE_TYPE::Tan:  
				return { BaseNode (type, NODE_CATEGORY::Branch_1) };
			case NODE_TYPE::Lerp:
				return { BaseNode (type, NODE_CATEGORY::Branch_3) };
			case NODE_TYPE::Atan:
				return { BaseNode (type, NODE_CATEGORY::Branch_1) };
			case NODE_TYPE::Plus:
				return { BaseNode (type, NODE_CATEGORY::Branch_2) };
			case NODE_TYPE::Minus:
				return { BaseNode (type, NODE_CATEGORY::Branch_2) };
			case NODE_TYPE::Mult:
				return { BaseNode (type, NODE_CATEGORY::Branch_2) };
			case NODE_TYPE::Div:
				return { BaseNode (type, NODE_CATEGORY::Branch_2) };
			case NODE_TYPE::Negate:
				return { BaseNode (type, NODE_CATEGORY::Branch_1) };
			case NODE_TYPE::Square:
				return { BaseNode (type, NODE_CATEGORY::Branch_1) };
			case NODE_TYPE::Ceil:
				return { BaseNode (type, NODE_CATEGORY::Branch_1) };
			case NODE_TYPE::Log2:
				return { BaseNode (type, NODE_CATEGORY::Branch_1) };
			case NODE_TYPE::Abs:
				return { BaseNode (type, NODE_CATEGORY::Branch_1) };
			case NODE_TYPE::Clip:
				return { BaseNode (type, NODE_CATEGORY::Branch_2) };
			case NODE_TYPE::Floor:
				return { BaseNode (type, NODE_CATEGORY::Branch_1) };
			case NODE_TYPE::Wrap:
				return { BaseNode (type, NODE_CATEGORY::Branch_1) };
			case NODE_TYPE::Const:
				return { BaseNode (type, NODE_CATEGORY::Leaf, (rand ()%10000)/1000.0f) };
			case NODE_TYPE::OpX:
				return { BaseNode (type, NODE_CATEGORY::Leaf) };
			case NODE_TYPE::OpY:
				return { BaseNode (type, NODE_CATEGORY::Leaf) };
			default:
				LOG_ERROR ("Unknown Node Type");
				return {};
		}
	}
	float Eval (NODE_TYPE type, float x, float y, float z, float data)
	{
		switch (type) {
			case NODE_TYPE::Max:
				return  MAX (x, y);
			case NODE_TYPE::Min:
				return MIN (x, y);
			case NODE_TYPE::arcTan2:
				return atan2f (glm::radians(y), glm::radians(x));
			case NODE_TYPE::Sin:
				return sinf (glm::radians(x));
			case NODE_TYPE::Cos:
				return cosf (glm::radians(x));
			case NODE_TYPE::Tan:
				return tanf(glm::radians(x));
			case NODE_TYPE::Atan:
				return atanf(glm::radians(x));
			case NODE_TYPE::Lerp:
				return x + z*(y - x);
			case NODE_TYPE::Plus:
				return y + x;
			case NODE_TYPE::Minus:
				return x - y;
			case NODE_TYPE::Mult:
				return y*x;
			case NODE_TYPE::Div:
				return x/y;
			case NODE_TYPE::Negate:
				return -x;
			case NODE_TYPE::Square:
				return x*x;
			case NODE_TYPE::Ceil:
				return int(x) + 1;
			case NODE_TYPE::Log2:
				return log2f (x);
			case NODE_TYPE::Abs:
				return abs(x);
			case NODE_TYPE::Clip:
				return x > y ? y : (x < -y ? -y : x);
			case NODE_TYPE::Floor:
				return int(x);
			case NODE_TYPE::Wrap:
				return -1 + 2*(((x + 1)/2) - int(((x + 1)/2)));
			case NODE_TYPE::Const:
				return data;
			case NODE_TYPE::OpX:
				return x;
			case NODE_TYPE::OpY:
				return y;
			default:
				LOG_ERROR ("Unknown Node Type");
				return 0;
		}
	}
	void NodeTree::ResetCluster ()
	{
		if(m_RootNode) m_RootNode->DeleteTree () , *m_RootNode = BaseNode ();
		m_RootNode = nullptr;
		m_LastInsertedNode = nullptr;
	}
	bool NodeTree::InsertNode (NODE_TYPE type)
	{
		auto node = CreateNode (type);
		if (node) {
			pushToCluster (node.value ());
			if (m_RootNode != nullptr) {
				if (!BalancedInsert (*m_LastInsertedNode)) {
					(*m_LastInsertedNode) = BaseNode ();
					m_LastInsertedNode = nullptr;
					return false;
				}
			} else {
				m_RootNode = m_LastInsertedNode;
				return true;
			}
		} else {
			LOG_ERROR ("unknown node");
			return false;
		}
	}
	void NodeTree::PrintTree ()
	{
		BaseNode *node = m_RootNode;
		BaseNode *next_level_node = (BaseNode *)((void *)node->firstChild ().value_or (nullptr));
		while (node) {
			std::cout << ToC_str (*node) << " ";
			BaseNode *nxtNode = node->NextNodeAtSameLevel ();
			if (nxtNode) {
				node = nxtNode;
				if (!next_level_node) {
					next_level_node = (BaseNode *)((void *)node->firstChild ().value_or (nullptr));
				}
			} else {
				node = next_level_node;
				std::cout << std::endl;
				next_level_node = node ? (BaseNode *)((void *)node->firstChild ().value_or (nullptr)) : nullptr;
			}
		}
		std::cout << std::endl;
	}
};