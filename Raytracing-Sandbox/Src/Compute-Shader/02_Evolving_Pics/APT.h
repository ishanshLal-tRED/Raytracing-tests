#pragma once
#include <GLCore.h>
#include <iostream>
#include <optional>
#include <vector>
namespace APT
{
	#define MIN(x,y) (x > y ? y : x)
	#define MAX(x,y) (x > y ? x : y)
	#define ABS(x)   (x > 0 ? x : -x)
	#define MOD(x,y) (x - ((int)(((float)x) / y) * y))

	class BaseNode;
	class NodeTree;
	static BaseNode *s_BaseNodesSelected[2] = { nullptr };
	static NodeTree *s_TargetNodeTree[2] = { nullptr };
	static NodeTree *s_CurrNodeTree = nullptr;

	enum class NODE_CATEGORY
		: uint8_t
	{
		None = 0,
		Leaf,
		Branch_1,
		Branch_1_and_data,
		Branch_2,
		Branch_2_and_data,
		Branch_3,
		Branch_3_and_data
	};
	enum class NODE_TYPE
		: uint8_t
	{ // update[APT] Eval, ToC_str, CreateNode // update[SHADER] TypeHasData, Evaluate
		None    = 0,
		Max     = 1,
		Min     = Max     + 1,
		arcTan2 = Min     + 1,
		Sin     = arcTan2 + 1,
		Cos     = Sin     + 1,
		Tan     = Cos     + 1,
		Atan    = Tan     + 1,
		Lerp    = Atan    + 1,
		Plus    = Lerp    + 1,
		Minus   = Plus    + 1,
		Mult    = Minus   + 1,
		Div     = Mult    + 1,
		Negate  = Div     + 1,
		Square  = Negate  + 1,
		Ceil    = Square  + 1,
		Log2    = Ceil    + 1,
		Abs     = Log2    + 1,
		Clip    = Abs     + 1,
		Floor   = Clip    + 1,
		Wrap    = Floor   + 1,
		Const   = Wrap    + 1,
		OpX     = Const   + 1,
		OpY     = OpX     + 1,
		TOTAL   = OpY     + 1
	};
	float Eval (NODE_TYPE, float x, float y, float z, float);
	const char *ToC_str (NODE_TYPE type);
	class BaseNode
	{
	public:
		BaseNode (NODE_TYPE type = NODE_TYPE::None, NODE_CATEGORY category = NODE_CATEGORY::None, double data = 0) 
			: m_Type (type), m_Category (category), m_Data ((float)data), m_Parent (nullptr), m_Child0(0), m_Child1(0), m_Child2(0)
		{};
		BaseNode (const BaseNode &node)// = default;
			: m_Type (node.m_Type), m_Category (node.m_Category), m_Data (node.m_Data), m_Parent (nullptr), m_Child0 (0), m_Child1 (0), m_Child2 (0)
		{}
		BaseNode &operator=(const BaseNode &node)// = default;
		{
			if (m_Type == NODE_TYPE::None) {
				*((NODE_TYPE *)((void *)&m_Type)) = node.m_Type;
				*((NODE_CATEGORY *)((void *)&m_Category)) = node.m_Category;
				m_Data = node.m_Data, m_Parent = 0, m_Child0 = 0, m_Child1 = 0, m_Child2 = 0;
				return BaseNode (node.m_Type, node.m_Category, node.m_Data);
			}
			if (node.m_Type == NODE_TYPE::None && !HasChild ()) {
				*((NODE_TYPE *)((void *)&m_Type)) = NODE_TYPE::None;
				*((NODE_CATEGORY *)((void *)&m_Category)) = NODE_CATEGORY::None;
				m_Data = 0, m_Parent = 0, m_Child0 = 0, m_Child1 = 0, m_Child2 = 0;
				return BaseNode ();
			}
			LOG_ASSERT ("use swap_tree, swap_child etc.");
			LOG_ASSERT (false);
			return *this;
		}
		const BaseNode &Parent () const{
			return *m_Parent;
		}
		void Parent (BaseNode &node){
			m_Parent = &node;
		}
		const float Data () const
		{
			switch (m_Category)
			{
				case NODE_CATEGORY::Leaf :
				case NODE_CATEGORY::Branch_1_and_data :
				case NODE_CATEGORY::Branch_2_and_data :
				case NODE_CATEGORY::Branch_3_and_data :
					return m_Data;
				case NODE_CATEGORY::Branch_1 :
				case NODE_CATEGORY::Branch_2 :
				case NODE_CATEGORY::Branch_3 :
					return 0;
			}
		}
		void Data (const float val)
		{
			switch (m_Category) {
				case NODE_CATEGORY::Leaf:
				case NODE_CATEGORY::Branch_1_and_data:
				case NODE_CATEGORY::Branch_2_and_data:
				case NODE_CATEGORY::Branch_3_and_data:
					m_Data = val;
					return;
				case NODE_CATEGORY::Branch_1:
				case NODE_CATEGORY::Branch_2:
				case NODE_CATEGORY::Branch_3:
					return;
			}
		}
		bool HasChild () const
		{
			switch (m_Category) {
				case NODE_CATEGORY::Leaf:
					return false;
				case NODE_CATEGORY::Branch_1:
				case NODE_CATEGORY::Branch_1_and_data:
					return (m_Child[0] ? true : false);
				case NODE_CATEGORY::Branch_2:
				case NODE_CATEGORY::Branch_2_and_data:
					return ((m_Child[0] ? true : false) | (m_Child[1] ? true : false));
				case NODE_CATEGORY::Branch_3:
				case NODE_CATEGORY::Branch_3_and_data:
					return ((m_Child[0] ? true : false) | (m_Child[1] ? true : false) | (m_Child[2] ? true : false));
				default:
					LOG_ASSERT (false);
					return 0;
			}
		}
		bool HasData () const
		{
			switch (m_Category) {
				case NODE_CATEGORY::Leaf:
				case NODE_CATEGORY::Branch_1_and_data:
				case NODE_CATEGORY::Branch_2_and_data:
				case NODE_CATEGORY::Branch_3_and_data:
					return true;
				case NODE_CATEGORY::Branch_1:
				case NODE_CATEGORY::Branch_2:
				case NODE_CATEGORY::Branch_3:
					return false;
				default:
					LOG_ASSERT (false);
					return false;
			}
		}
		std::optional<const BaseNode *> firstChild () const
		{
			switch (m_Category) {
				case NODE_CATEGORY::Leaf:
					return {};
				case NODE_CATEGORY::Branch_1:
				case NODE_CATEGORY::Branch_1_and_data:
					return { m_Child[0] };
				case NODE_CATEGORY::Branch_2:
				case NODE_CATEGORY::Branch_2_and_data:
					for (uint8_t i = 0; i < 2; i++) {
						if (m_Child[i])
							return { m_Child[i] };
					}
					return { nullptr };
				case NODE_CATEGORY::Branch_3:
				case NODE_CATEGORY::Branch_3_and_data:
					for (uint8_t i = 0; i < 3; i++) {
						if (m_Child[i])
							return { m_Child[i] };
					}
					return { nullptr };
				default:
					LOG_ASSERT (false);
					return{};
			};
		};
		uint32_t NodeCountInTree ()
		{
			switch (m_Category) {
				case NODE_CATEGORY::Leaf:
					return { 1 };
				case NODE_CATEGORY::Branch_1:
				case NODE_CATEGORY::Branch_1_and_data:
					return { 1 + (m_Child[0] ? m_Child[0]->NodeCountInTree () : 0) };
				case NODE_CATEGORY::Branch_2:
				case NODE_CATEGORY::Branch_2_and_data:
					return { 1 + (m_Child[0] ? m_Child[0]->NodeCountInTree () : 0) + (m_Child[1] ? m_Child[1]->NodeCountInTree () : 0) };
				case NODE_CATEGORY::Branch_3:
				case NODE_CATEGORY::Branch_3_and_data:
					return { 1 + (m_Child[0] ? m_Child[0]->NodeCountInTree () : 0) + (m_Child[1] ? m_Child[1]->NodeCountInTree () : 0) + (m_Child[2] ? m_Child[2]->NodeCountInTree () : 0) };
				default:
					LOG_ASSERT (false);
					return 0;
			}
		}
		uint16_t EmptyLeafPosnCount () const
		{
			switch (m_Category) {
				case NODE_CATEGORY::Leaf:
					return 0;
				case NODE_CATEGORY::Branch_1:
				case NODE_CATEGORY::Branch_1_and_data:
					return (0 + (m_Child[0] ? m_Child[0]->EmptyLeafPosnCount () : 1));
				case NODE_CATEGORY::Branch_2:
				case NODE_CATEGORY::Branch_2_and_data:
					return (0 + (m_Child[0] ? m_Child[0]->EmptyLeafPosnCount () : 1) + (m_Child[1] ? m_Child[1]->EmptyLeafPosnCount () : 1));
				case NODE_CATEGORY::Branch_3:
				case NODE_CATEGORY::Branch_3_and_data:
					return (0 + (m_Child[0] ? m_Child[0]->EmptyLeafPosnCount () : 1) + (m_Child[1] ? m_Child[1]->EmptyLeafPosnCount () : 1) + (m_Child[2] ? m_Child[2]->EmptyLeafPosnCount () : 1));
				default:
					LOG_ASSERT (false);
					return 0;
			}
		}
		BaseNode *NextNodeAtSameLevel(const int maxDepth = 0){
			int lastLevel = 1;
			int currLevel = 0;
			BaseNode *currNode = this;
			if (currNode->m_Parent == nullptr)
				return nullptr;
			do{
				if (lastLevel > currLevel) { // move up or same level
					if(maxDepth && currLevel <= maxDepth)
						return nullptr;
					switch (currNode->m_Parent->m_Category) {
						case NODE_CATEGORY::Leaf: // move up only
						case NODE_CATEGORY::Branch_1: 
						case NODE_CATEGORY::Branch_1_and_data:
							lastLevel = currLevel;
							currNode = currNode->m_Parent;
							currLevel--;
							break;
						case NODE_CATEGORY::Branch_2:
						case NODE_CATEGORY::Branch_2_and_data:{
								uint8_t itsParentsChildNum;
								constexpr uint8_t max_branch_size = 2;
								for (itsParentsChildNum = 0; currNode != currNode->m_Parent->m_Child[itsParentsChildNum]; itsParentsChildNum++) {
									LOG_ASSERT (itsParentsChildNum < max_branch_size);
								}
								uint8_t switchBranchTo = 0;
								for (uint8_t i = itsParentsChildNum + 1; i < max_branch_size; i++) { // find next sibling it can switch to
									if (currNode->m_Parent->m_Child[i]) {
										switchBranchTo = i;
										break;
									}
								}
								if (switchBranchTo) {
									lastLevel = currLevel;
									currNode = currNode->m_Parent->m_Child[switchBranchTo];
									// currLevel = currLevel; // same level
									if (currLevel == 0) {
										return currNode;
									}
								} else { // cannot switch branch move up
									lastLevel = currLevel;
									currNode = currNode->m_Parent;
									currLevel--;
								}
							}break;
						case NODE_CATEGORY::Branch_3:
						case NODE_CATEGORY::Branch_3_and_data:{
								uint8_t itsParentsChildNum;
								constexpr uint8_t max_branch_size = 3;
								for (itsParentsChildNum = 0; currNode != currNode->m_Parent->m_Child[itsParentsChildNum]; itsParentsChildNum++) {
									LOG_ASSERT (itsParentsChildNum < max_branch_size);
								}
								uint8_t switchBranchTo = 0;
								for (uint8_t i = itsParentsChildNum + 1; i < max_branch_size; i++) { // find next sibling it can switch to
									if (currNode->m_Parent->m_Child[i]) {
										switchBranchTo = i;
										break;
									}
								}
								if (switchBranchTo) {
									lastLevel = currLevel;
									currNode = currNode->m_Parent->m_Child[switchBranchTo];
									// currLevel = currLevel; // same level
									if (currLevel == 0) {
										return currNode;
									}
								} else { // cannot switch branch move up
									lastLevel = currLevel;
									currNode = currNode->m_Parent;
									currLevel--;
								}
							}break;
						default:
							LOG_ERROR ("This Node Is Invalid");
							return nullptr;
					}
				} else { // move down or up level
					switch (currNode->m_Category) {
						case NODE_CATEGORY::Leaf: // move up only
							lastLevel = currLevel;
							currNode = currNode->m_Parent;
							currLevel--;
						break; 
						case NODE_CATEGORY::Branch_1: case NODE_CATEGORY::Branch_1_and_data:
							if (currNode->m_Child[0] != nullptr) {
								lastLevel = currLevel;
								currNode = currNode->m_Child[0];
								currLevel++;
							} else { // child is nullptr, move up
								lastLevel = currLevel;
								currNode = currNode->m_Parent;
								currLevel--;
							}
							break;
						case NODE_CATEGORY::Branch_2:
						case NODE_CATEGORY::Branch_2_and_data:
						case NODE_CATEGORY::Branch_3:
						case NODE_CATEGORY::Branch_3_and_data:{
								BaseNode *childNode = ((BaseNode *)((void *)currNode->firstChild ().value_or (nullptr)));
								if (childNode) { // move down
									lastLevel = currLevel;
									currNode = childNode;
									currLevel++;
								} else { // no child, move up
									lastLevel = currLevel;
									currNode = currNode->m_Parent;
									currLevel--;
								}
							}break;
						default:
							LOG_ERROR ("This Node Is Invalid");
							return nullptr;
					}
				}
			}while (currNode->m_Parent != nullptr && currLevel != 0); // Autobreak if no currnode.parent (we are checking for currnode = nullptr condn) i.e no more space to travel reached end
			if (currLevel < 0)
			{
				return nullptr;
			}
			return currNode;
		}
		float Value (float x, float y, float z)
		{
			float const_data = m_Data;
			switch (m_Category){
				case NODE_CATEGORY::Leaf:
					//x = x, y = y, z = z;
					// const_data = m_Data;
					break;
				case NODE_CATEGORY::Branch_1:
					const_data = 0; // set 0
				case NODE_CATEGORY::Branch_1_and_data:
					x = m_Child[0] ? m_Child[0]->Value (x, y, z) : x;
					y = 0;
					z = 0;
					break;
				case NODE_CATEGORY::Branch_2:
					const_data = 0; // set 0A
				case NODE_CATEGORY::Branch_2_and_data:
					x = m_Child[0] ? m_Child[0]->Value (x, y, z) : x;
					y = m_Child[1] ? m_Child[1]->Value (x, y, z) : y;
					z = 0;
					break;
				case NODE_CATEGORY::Branch_3:
					const_data = 0; // set 0
				case NODE_CATEGORY::Branch_3_and_data:
					x = m_Child[0] ? m_Child[0]->Value (x, y, z) : x;
					y = m_Child[1] ? m_Child[1]->Value (x, y, z) : y;
					z = m_Child[2] ? m_Child[2]->Value (x, y, z) : z;
					break;
			}
			float val = Eval (m_Type, x, y, z, const_data);
			return val;
		}
		bool TryInsert (BaseNode &node)
		{
			switch (m_Category) {
				case NODE_CATEGORY::Leaf:
					return false;
				case NODE_CATEGORY::Branch_1: case NODE_CATEGORY::Branch_1_and_data:
					if (m_Child[0])
						return false;
					else {
						m_Child[0] = &node, node.Parent (*this);
						return true;
					}
				case NODE_CATEGORY::Branch_2:
				case NODE_CATEGORY::Branch_2_and_data:
					if (m_Child[1] == nullptr) {
						if (m_Child[0] == nullptr) {
							m_Child[0] = &node;
							node.Parent (*this);
							return true;
						}
						m_Child[1] = &node;
						node.Parent (*this);
						return true;
					} else {
						return false;
					}
				case NODE_CATEGORY::Branch_3:
				case NODE_CATEGORY::Branch_3_and_data:
					if (m_Child[2] == nullptr) {
						if (m_Child[1] == nullptr) {
							if (m_Child[0] == nullptr) {
								m_Child[0] = &node;
								node.Parent (*this);
								return true;
							}
							m_Child[1] = &node;
							node.Parent (*this);
							return true;
						}
						m_Child[2] = &node;
						node.Parent (*this);
						return true;
					} else {
						return false;
					}
			}
		}
		bool BalancedInsertChildNode (BaseNode &node, bool localInsert = false)
		{
			int parentAtDepth = localInsert ? -1 : 0;

			BaseNode *currNode = this;
			BaseNode *nxtLevels1stNode = (BaseNode *)((void*)currNode->firstChild().value_or (nullptr));
			while (currNode)
			{
				if (currNode->TryInsert (node)) {
					return true;
				}
				BaseNode *nxtNode = currNode->NextNodeAtSameLevel (parentAtDepth);
				if (nxtNode) {
					currNode = nxtNode;
					if (!nxtLevels1stNode){
						nxtLevels1stNode = (BaseNode *)((void *)currNode->firstChild ().value_or (nullptr));
					}
				}
				else {
					currNode = nxtLevels1stNode;
					nxtLevels1stNode = currNode ? (BaseNode *)((void *)currNode->firstChild ().value_or (nullptr)) : nullptr;
					parentAtDepth = localInsert ? parentAtDepth - 1 : 0;
				}
			}
			return false;
		}
		virtual const NODE_CATEGORY Category () { return m_Category; }
		virtual const NODE_TYPE     Type ()     { return m_Type; }
		operator NODE_TYPE () const { return m_Type; }
		const char* String () const
		{
			return ToC_str (m_Type);
		}
		void TrySwapChildren(BaseNode &node)
		{
			NODE_CATEGORY cat[2] = { m_Category, node.m_Category };
			for (NODE_CATEGORY &c: cat){
				switch (c) {
					case NODE_CATEGORY::Branch_1_and_data:
						c = NODE_CATEGORY::Branch_1; break;
					case NODE_CATEGORY::Branch_2_and_data:
						c = NODE_CATEGORY::Branch_2; break;
					case NODE_CATEGORY::Branch_3_and_data:
						c = NODE_CATEGORY::Branch_3; break;
				}
			}
			if (cat[0] == cat[1]){
				switch (cat[0]) {
					case NODE_CATEGORY::Leaf:
						break;
					case NODE_CATEGORY::Branch_1:{
							constexpr uint8_t max_branch_size = 1; BaseNode *temp;
							for (uint8_t i = 0; i < max_branch_size; i++) {
								BaseNode *temp = node.m_Child[i];
								node.m_Child[i] = m_Child[i]; node.m_Child[i]->Parent (node);
								m_Child[i] = temp; m_Child[i]->Parent (*this);
							}
						}break;
					case NODE_CATEGORY::Branch_2:{
							constexpr uint8_t max_branch_size = 2; BaseNode *temp;
							for (uint8_t i = 0; i < max_branch_size; i++) {
								BaseNode *temp = node.m_Child[i];
								node.m_Child[i] = m_Child[i]; node.m_Child[i]->Parent (node);
								m_Child[i] = temp; m_Child[i]->Parent (*this);
							}
						}break;
					case NODE_CATEGORY::Branch_3:{
							constexpr uint8_t max_branch_size = 2; BaseNode *temp;
							for (uint8_t i = 0; i < max_branch_size; i++) {
								BaseNode *temp = node.m_Child[i];
								node.m_Child[i] = m_Child[i]; node.m_Child[i]->Parent (node);
								m_Child[i] = temp; m_Child[i]->Parent (*this);
							}
						}break;
				}
			}
		}
		void SwapChildren(BaseNode &node)
		{
			NODE_CATEGORY cat[2] = { m_Category, node.m_Category };
			for (NODE_CATEGORY &c: cat){
				switch (c) {
					case NODE_CATEGORY::Branch_1_and_data:
						c = NODE_CATEGORY::Branch_1; break;
					case NODE_CATEGORY::Branch_2_and_data:
						c = NODE_CATEGORY::Branch_2; break;
					case NODE_CATEGORY::Branch_3_and_data:
						c = NODE_CATEGORY::Branch_3; break;
				}
			}
			if (cat[0] == cat[1]){
				switch (cat[0]) {
					case NODE_CATEGORY::Leaf:
						LOG_WARN ("Leaf node, No child - no swap");
						break;
					case NODE_CATEGORY::Branch_1:{
							constexpr uint8_t max_branch_size = 1; BaseNode *temp;
							for (uint8_t i = 0; i < max_branch_size; i++) {
								BaseNode *temp = node.m_Child[i];
								node.m_Child[i] = m_Child[i]; node.m_Child[i]->Parent (node);
								m_Child[i] = temp; m_Child[i]->Parent (*this);
							}
						}break;
					case NODE_CATEGORY::Branch_2:{
							constexpr uint8_t max_branch_size = 2; BaseNode *temp;
							for (uint8_t i = 0; i < max_branch_size; i++) {
								BaseNode *temp = node.m_Child[i];
								node.m_Child[i] = m_Child[i]; node.m_Child[i]->Parent (node);
								m_Child[i] = temp; m_Child[i]->Parent (*this);
							}
						}break;
					case NODE_CATEGORY::Branch_3:{
							constexpr uint8_t max_branch_size = 2; BaseNode *temp;
							for (uint8_t i = 0; i < max_branch_size; i++) {
								BaseNode *temp = node.m_Child[i];
								node.m_Child[i] = m_Child[i]; node.m_Child[i]->Parent (node);
								m_Child[i] = temp; m_Child[i]->Parent (*this);
							}
						}break;
				}
			} else
				LOG_WARN ("Cannot Swap children, use TrySwapChildren(): no warns or SwapNodeTree()");
		}
		bool SwapNodeTree (BaseNode &node)
		{
			if (m_Parent && node.m_Parent) { // none is root node, NodeTree should handle this case
				BaseNode *parent1 = m_Parent, *parent2 = node.m_Parent;
				BaseNode *child1 = this, *child2 = &node;
				//uint8_t child1Index = 0;
				uint8_t child2Index = 0;
				for (uint8_t i = 0; i < 3; i++) { // change child
					if (parent1->m_Child[i] == child1){
						parent1->m_Child[i] = child2; break;
					}
				}
				for (uint8_t i = 0; i < 3; i++) { // change child
					if (parent2->m_Child[i] == child2){
						parent2->m_Child[i] = child1; break;
					}
				}
				// change parent
				child1->Parent (*parent2), child2->Parent (*parent1);
			}
		}
		void SwapData(BaseNode &node)
		{
			if (HasData() && node.HasData ()) {
				float temp = node.m_Data;
				node.m_Data = m_Data;
				m_Data = temp;
			}
		}
		void DeleteTree ()
		{
			uint8_t max_branch_size = 0;
			switch (m_Category) {
				case NODE_CATEGORY::Branch_1: case NODE_CATEGORY::Branch_1_and_data: 
					max_branch_size = 1;break;
				case NODE_CATEGORY::Branch_2: case NODE_CATEGORY::Branch_2_and_data:
					max_branch_size = 2;break;
				case NODE_CATEGORY::Branch_3: case NODE_CATEGORY::Branch_3_and_data:
					max_branch_size = 3;break;
			}
			for (uint8_t i = 0; i < max_branch_size; i++) {
				if (m_Child[i]) {
					m_Child[i]->DeleteTree ();
					(*m_Child[i]) = BaseNode ();
					m_Child[i] = nullptr;
				}
			}
		}
		void ImGuiTreeRender ()
		{
			uint8_t max_branch_size = 0;
			switch (m_Category) {
				case NODE_CATEGORY::Branch_1: case NODE_CATEGORY::Branch_1_and_data:
					max_branch_size = 1; break;
				case NODE_CATEGORY::Branch_2: case NODE_CATEGORY::Branch_2_and_data:
					max_branch_size = 2; break;
				case NODE_CATEGORY::Branch_3: case NODE_CATEGORY::Branch_3_and_data:
					max_branch_size = 3; break;
			}
			if (ImGui::Button (GLCore::ImGuiLayer::UniqueName (">"))) {
				if (!s_BaseNodesSelected[0]){
					s_BaseNodesSelected[0] = this;
					s_TargetNodeTree[0] = s_CurrNodeTree;
				} else if (!s_BaseNodesSelected[1]) {
					s_BaseNodesSelected[1] = this;
					s_TargetNodeTree[1] = s_CurrNodeTree;
				}
			}
			ImGui::SameLine ();
			ImGui::Text ("[%d: %s] {parent: %d | %s , Child(%d): %s , %s , %s}",
						 (void *)this, String ()
						 , (void *)m_Parent, m_Parent ? m_Parent->String () : "null", max_branch_size
						 , m_Child[0] ? m_Child[0]->String () : "null"
						 , m_Child[1] ? m_Child[1]->String () : "null"
						 , m_Child[2] ? m_Child[2]->String () : "null");
			ImGui::Indent ();
			for (uint8_t i = 0; i < max_branch_size; i++) {
				if (m_Child[i])
					m_Child[i]->ImGuiTreeRender ();
			}
			ImGui::Unindent ();
		}
	protected:
		NODE_CATEGORY m_Category = NODE_CATEGORY::None;
		NODE_TYPE     m_Type     = NODE_TYPE::None;
	private:
		BaseNode *m_Parent;
		union
		{
			struct
			{
				BaseNode *m_Child0, *m_Child1, *m_Child2;
			};
			BaseNode *m_Child[3];
		};
		float m_Data;
		friend class NodeTree;
	};
	std::optional<BaseNode> CreateNode (NODE_TYPE type);

	// Note: all the operation ar performed inside tree can freely use base node functions
	// However the moment 2 or more trees get involved you cannot freely use Base-node functions
	class NodeTree
	{
	public:
		NodeTree ()
		{
			ResetCluster ();
		}
		NodeTree (std::vector<int> &arr, std::vector<float> &const_data) {
			if (arr[0] > 0)
			{
				InsertNode ((NODE_TYPE)arr[0]);
				if (m_RootNode->HasData ())
					m_RootNode->Data (const_data[0]);
				uint16_t i = 0;
				uint16_t j = m_RootNode->HasData () ? 1 : 0;
				PushArraysToTree (m_RootNode, i, arr, j, const_data);
			} else {
				LOG_ERROR("InValid-Tree");
			}
		}
		void ResetCluster ();
		~NodeTree () {};

		void SpawnRandomTree (const uint32_t max_tree_size)
		{
			if (!m_RootNode){
				uint8_t i = (uint8_t)rand () % ((uint8_t)NODE_TYPE::TOTAL - 1);
				InsertNode ((NODE_TYPE)(i + 1));
			}
			RandomTreeFill (m_RootNode, max_tree_size);
		}
		float Eval (float x, float y, float z = 0.0f) {
			return m_RootNode->Value (x, y, z);
		}
		void ImGuiTreeRender () {
			s_CurrNodeTree = this;
			if(m_RootNode) m_RootNode->ImGuiTreeRender ();
		}
		static void ImGuiManuplationButtons ()
		{
			if (ImGui::Button ("Target(reset)"))
				s_BaseNodesSelected[0] = nullptr;
			ImGui::SameLine ();
			ImGui::Text ("%s", s_BaseNodesSelected[0] ? s_BaseNodesSelected[0]->String () : "null");
			ImGui::SameLine ();
			if (ImGui::Button ("Source(reset)"))
				s_BaseNodesSelected[1] = nullptr;
			ImGui::SameLine ();
			ImGui::Text ("%s", s_BaseNodesSelected[1] ? s_BaseNodesSelected[1]->String () : "null");
			if (ImGui::Button ("Copy Tree")) {
				if (s_BaseNodesSelected[0] && s_BaseNodesSelected[1]) {
					s_TargetNodeTree[0]->CopyTree (*s_BaseNodesSelected[0], *s_BaseNodesSelected[1]);
					s_TargetNodeTree[0] = nullptr; s_TargetNodeTree[1] = nullptr;
					s_BaseNodesSelected[0] = nullptr;
					s_BaseNodesSelected[1] = nullptr;
				} else
					LOG_ERROR ("Select Base Nodes");
			}
			ImGui::SameLine ();
			if (ImGui::Button ("Swap Tree")) {
				if (s_BaseNodesSelected[0] && s_BaseNodesSelected[1]) {
					SwapTree (*s_TargetNodeTree[0], *s_BaseNodesSelected[0], *s_TargetNodeTree[1], *s_BaseNodesSelected[1]);
					s_TargetNodeTree[0] = nullptr, s_TargetNodeTree[1] = nullptr;
					s_BaseNodesSelected[0] = nullptr, s_BaseNodesSelected[1] = nullptr;
				} else
					LOG_ERROR ("Select Base Nodes");
			}
			ImGui::SameLine ();
			if (ImGui::Button ("Mutate")) {
				if (s_BaseNodesSelected[0]) {
					s_TargetNodeTree[0]->MutateNode (*s_BaseNodesSelected[0]);
					s_TargetNodeTree[0] = nullptr, s_TargetNodeTree[1] = nullptr;
					s_BaseNodesSelected[0] = nullptr, s_BaseNodesSelected[1] = nullptr;
				} else
					LOG_ERROR ("Select Base Nodes");
			}
			ImGui::SameLine ();
			{
				static std::vector<float> m_data_arr;
				static std::vector<int> m_arr;
				static NodeTree *tree;
				if (ImGui::Button ("NodeTree To Arrays Array")) {
					m_data_arr.clear ();
					m_arr.clear ();
					if (s_TargetNodeTree[0]) {
						s_TargetNodeTree[0]->PushTreeToArrays (s_BaseNodesSelected[0], m_arr, m_data_arr);
						delete tree;
						tree = nullptr;
					}
					s_TargetNodeTree[0] = nullptr, s_TargetNodeTree[1] = nullptr;
					s_BaseNodesSelected[0] = nullptr, s_BaseNodesSelected[1] = nullptr;
				}

				if (ImGui::CollapsingHeader ("Extras")) {
					static constexpr int OpenBrace = -1;
					static constexpr int CloseBrace = -2;
					int j = 0;
					for (const int i : m_arr) {
						if (i == OpenBrace) {
							ImGui::Indent (); continue;
						} else if (i == CloseBrace) {
							ImGui::Unindent (); continue;
						}
						ImGui::Text ("%s", i ? ToC_str ((NODE_TYPE)i) : "null");
						{
							bool hasDATA = false;
							switch (CreateNode ((NODE_TYPE)i).value ().m_Category) {
								case NODE_CATEGORY::Leaf:
								case NODE_CATEGORY::Branch_1_and_data:
								case NODE_CATEGORY::Branch_2_and_data:
								case NODE_CATEGORY::Branch_3_and_data:
									hasDATA = true;
							}
							if (hasDATA) {
								ImGui::SameLine ();
								ImGui::Text (" - %f", m_data_arr[j]);
								j++;
							}
						}
					}
					if (!tree && !m_arr.empty () && !m_data_arr.empty ())
						tree = new NodeTree (m_arr, m_data_arr);
					
					if(!tree->isEmpty ())
						tree->ImGuiTreeRender ();
				}
			}
		}
		bool isEmpty () const { return m_RootNode ? false : true; };
		uint32_t NumOfNodesInTree () { return m_RootNode ? m_RootNode->NodeCountInTree () : 0; }
		
		// Imp. functionality
		bool InsertNode (NODE_TYPE type);
		bool CopyTree (BaseNode &copy_to_node, BaseNode &copy_from_node){
			// check whether node exists in NodeCluster
			{
				BaseNode *currNode1 = &copy_to_node;
				while (currNode1->m_Parent != nullptr)
					currNode1 = currNode1->m_Parent;
				LOG_ASSERT (currNode1 == m_RootNode);
				
				BaseNode *currNode2 = &copy_to_node;
				while (currNode2->m_Parent != nullptr)
					currNode2 = currNode2->m_Parent;
				
				if(currNode1 == currNode2){ // tree clash
					// check ancestral relation
					bool clashHappened = false;
					{
						BaseNode *currNode = &copy_from_node;
						while (currNode != &copy_to_node && currNode != nullptr)
							currNode = currNode->m_Parent;
						clashHappened |= (bool)currNode; // if nullptr clash not happened

						currNode = &copy_to_node;
						while (currNode != &copy_from_node && currNode != nullptr)
							currNode = currNode->m_Parent;
						clashHappened |= (bool)currNode; // if nullptr clash not happened
					}
					if (clashHappened) // clash occured
					{
						NodeTree tempTree;
						tempTree.InsertNode (copy_from_node);
						tempTree.RawCopyTree (*tempTree.m_RootNode, copy_from_node);
						RawCopyTree (copy_to_node, *tempTree.m_RootNode);
						return true;
					}
				}
			}
			
			// TODO check whether cluster has space to copy
			RawCopyTree (copy_to_node, copy_from_node);
			return true;
		}
		static bool SwapTree (NodeTree &tree_of_node1, BaseNode &node1, NodeTree &tree_of_node2, BaseNode &node2)
		{
			// check whether node exists in NodeCluster
			{
				BaseNode *currNode = &node1;
				while (currNode->m_Parent != nullptr)
					currNode = currNode->m_Parent;
				LOG_ASSERT (currNode == tree_of_node1.m_RootNode);
			}{
				BaseNode *currNode = &node2;
				while (currNode->m_Parent != nullptr)
					currNode = currNode->m_Parent;
				LOG_ASSERT (currNode == tree_of_node2.m_RootNode);
			}
			if (&tree_of_node2 == &tree_of_node1) {
				// check ancestral relation
				bool clashHappened = false;
				{
					BaseNode *currNode = &node1;
					while (currNode != &node2 && currNode != nullptr)
						currNode = currNode->m_Parent;
					clashHappened |= (bool)currNode; // if nullptr clash not happened

					currNode = &node2;
					while (currNode != &node1 && currNode != nullptr)
						currNode = currNode->m_Parent;
					clashHappened |= (bool)currNode; // if nullptr clash not happened
				}
				if (clashHappened) {
					LOG_ERROR ("Cannot Replace node, there's ancestral relation b/w {0} & {1}", node1.String (), node2.String ());
					return false;
				}
			}
			NodeTree tempTree;
			tempTree.InsertNode (node2);
			tempTree.RawCopyTree (*tempTree.m_RootNode, node2);
			tree_of_node2.RawCopyTree (node2, node1);
			tree_of_node1.RawCopyTree (node1, *tempTree.m_RootNode);
			return true;
		}
		void MutateNode (BaseNode &node, const uint16_t node_tree_size = 5)
		{
			BaseNode *currNode1 = &node;
			while (currNode1->m_Parent != nullptr)
				currNode1 = currNode1->m_Parent;
			LOG_ASSERT (currNode1 == m_RootNode);

			BaseNode *parentOfNode = node.m_Parent;
			node.DeleteTree ();
			node = BaseNode();
			uint8_t i = (uint8_t)rand () % ((uint8_t)NODE_TYPE::TOTAL - 1);
			node = CreateNode ((NODE_TYPE)(i + 1)).value ();
			node.Parent (*parentOfNode);
			if (node.EmptyLeafPosnCount ()) {
				RandomTreeFill (&node, node_tree_size);
			}
		}
		void PrintTree ();
		void FillAllLeafPosnWithRandomLeaf ()
		{
			do 
			{
				// 3 types of leaf node
				uint8_t i = (uint8_t)(rand ()) % 3;
				switch (i) {
					case 0: 
						pushToCluster (CreateNode (NODE_TYPE::Const).value ()); break;
					case 1: 
						pushToCluster (CreateNode (NODE_TYPE::OpX).value ()); break;
					case 2: 
						pushToCluster (CreateNode (NODE_TYPE::OpY).value ()); break;
				}
			} while (BalancedInsert (*m_LastInsertedNode));
			*m_LastInsertedNode = BaseNode (); // last node was a faliure
		}
		std::pair<std::vector<int>, std::vector<float>> TreeToArray (){
			std::vector<int> arr;
			std::vector<float> const_data;
			if (m_RootNode)
				PushTreeToArrays (m_RootNode, arr, const_data);
			else
				LOG_ERROR ("No Root node");
			return { arr, const_data };
		}
	private:
		// Helper Inernals
		void PushTreeToArrays (BaseNode *node, std::vector<int> &arr, std::vector<float> &const_data)
		{
			if (node == nullptr) {
				// 0 for none (for debugging)
				arr.push_back (0);
			}
			arr.push_back ((int)node->m_Type);
			if (node->HasData ()) {
				const_data.push_back (node->m_Data);
			}
			constexpr int OpenBrace = -1;
			constexpr int CloseBrace = -2;
			// -1 for '(' AND -2 for ')'
			if (node->HasChild ()) {
				arr.push_back (OpenBrace);
				uint8_t max_branch_size = 0;
				switch (node->m_Category) {
					case NODE_CATEGORY::Branch_1:
					case NODE_CATEGORY::Branch_1_and_data:
						max_branch_size = 1; break;
					case NODE_CATEGORY::Branch_2:
					case NODE_CATEGORY::Branch_2_and_data:
						max_branch_size = 2; break;
					case NODE_CATEGORY::Branch_3:
					case NODE_CATEGORY::Branch_3_and_data:
						max_branch_size = 3; break;
				}
				for (uint8_t i = 0; i < max_branch_size; i++) {
					PushTreeToArrays (node->m_Child[i], arr, const_data);
				}
				arr.push_back (CloseBrace);
			}
		}
		void PushArraysToTree (BaseNode *node, uint16_t &index, const std::vector<int> &arr, uint16_t &data_index, const std::vector<float> &const_data)
		{
			if (node->m_Type != (NODE_TYPE)arr[index]) {
				BaseNode *parent = node->m_Parent;
				node->DeleteTree ();
				*node = BaseNode ();
				*node = CreateNode ((NODE_TYPE)arr[index]).value ();
				node->Parent (*parent);
			}
			index++;
			if (node->HasData ())
			{
				node->m_Data = const_data[data_index];
				data_index++;
			}
			constexpr int OpenBrace  = -1;
			constexpr int CloseBrace = -2;
			// {max, ( , X, X, ) }
			if (arr[index] == OpenBrace) {
				uint8_t i = 0;
				index++;
				while (arr[index] != CloseBrace){
					pushToCluster (CreateNode ((NODE_TYPE)arr[index]).value ());
					if (!node->TryInsert (*m_LastInsertedNode)) {
						LOG_ERROR ("InValid Tree, Cannot Insert Node {1} in Node {0}", node->String (), m_LastInsertedNode->String ());
						*m_LastInsertedNode = BaseNode ();
						m_LastInsertedNode = nullptr;
					} else
						PushArraysToTree (m_LastInsertedNode, index, arr, data_index, const_data);
					i++;
				}
				// found CloseBrace
				index++;

			}
		}
		void RandomTreeFill (BaseNode *target, const uint32_t max_tree_size)
		{
			uint16_t emptyLeafCount = target->EmptyLeafPosnCount ();
			while (emptyLeafCount && (emptyLeafCount + target->NodeCountInTree ()) < max_tree_size) {
				uint8_t i = (uint8_t)(rand () % ((uint8_t)NODE_TYPE::TOTAL - 1));
				{
					auto node = CreateNode ((NODE_TYPE)(i + 1));
					if (node) {
						pushToCluster (node.value ());
						if (!target->BalancedInsertChildNode(*m_LastInsertedNode, true)) {
							(*m_LastInsertedNode) = BaseNode ();
							m_LastInsertedNode = nullptr;
							return;
						}
					} else {
						LOG_ERROR ("unknown node");
						return;
					}
				}
				emptyLeafCount = m_RootNode->EmptyLeafPosnCount ();
			}
			FillAllLeafPosnWithRandomLeaf ();
		}
		void RawCopyTree (BaseNode &copy_to_node, BaseNode &copy_from_node)
		{
			{
				BaseNode *parentOf_copy_to_node = copy_to_node.m_Parent;
				copy_to_node.DeleteTree ();
				copy_to_node = BaseNode ();
				copy_to_node = CreateNode (copy_from_node).value ();
				copy_to_node.Parent (*parentOf_copy_to_node);
			}
			_treeCopy (&copy_to_node, &copy_from_node);
		}
		void _treeCopy (BaseNode *copy_to_node, BaseNode *copy_from_node) {
			uint8_t max_branch_size = 0;
			switch (copy_from_node->m_Category) {
				case NODE_CATEGORY::Branch_1: case NODE_CATEGORY::Branch_1_and_data:
					max_branch_size = 1; break;
				case NODE_CATEGORY::Branch_2: case NODE_CATEGORY::Branch_2_and_data:
					max_branch_size = 2; break;
				case NODE_CATEGORY::Branch_3: case NODE_CATEGORY::Branch_3_and_data:
					max_branch_size = 3; break;
			}
			
			for (uint8_t i = 0; i < max_branch_size; i++) {
				if (copy_from_node->m_Child[i]) {
					pushToCluster (CreateNode (*(copy_from_node->m_Child[i])).value ());
					copy_to_node->m_Child[i] = m_LastInsertedNode; copy_to_node->m_Child[i]->Parent (*copy_to_node);
					_treeCopy (copy_to_node->m_Child[i], copy_from_node->m_Child[i]);
				}
			}
		}
		bool BalancedInsert (BaseNode &node)
		{
			return m_RootNode->BalancedInsertChildNode (node);
		}
		bool pushToCluster (BaseNode node){
			for (uint8_t i = 0; i < capacity; i++) {
				if (m_NodeCluster[i].m_Type == NODE_TYPE::None) {
					m_NodeCluster[i] = node, m_LastInsertedNode = &m_NodeCluster[i];
					return true;
				}
			}
		}
	private:
		static const uint32_t capacity = 50;
		BaseNode m_NodeCluster[capacity];
		BaseNode *m_RootNode = nullptr;
		BaseNode *m_LastInsertedNode = nullptr;
	};
}