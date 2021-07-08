#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <tuple>
#include <queue>
#include <algorithm>

namespace LBVH
{
	inline uint32_t expand_bits (uint32_t v) noexcept
	{
		v = (v * 0x00010001u) & 0xFF0000FFu;
		v = (v * 0x00000101u) & 0x0F00F00Fu;
		v = (v * 0x00000011u) & 0xC30C30C3u;
		v = (v * 0x00000005u) & 0x49249249u;
		return v;
	}
	// Calculates a 30-bit Morton code for the
	// given 3D point located within the unit cube [0,1].
	inline uint32_t morton_code (glm::vec3 xyz, float resolution = 1024.0f) noexcept
	{
		xyz.x = ::fminf (::fmaxf (xyz.x * resolution, 0.0f), resolution - 1.0f);
		xyz.y = ::fminf (::fmaxf (xyz.y * resolution, 0.0f), resolution - 1.0f);
		xyz.z = ::fminf (::fmaxf (xyz.z * resolution, 0.0f), resolution - 1.0f);
		const uint32_t xx = expand_bits (static_cast<uint32_t>(xyz.x));
		const uint32_t yy = expand_bits (static_cast<uint32_t>(xyz.y));
		const uint32_t zz = expand_bits (static_cast<uint32_t>(xyz.z));
		return xx * 4 + yy * 2 + zz;
	}

	struct Key
	{
		uint32_t MortonCode;
		uint32_t ObjectID;
	};

	struct BVHNode
	{
		uint32_t idx = 0;

		uint32_t ObjectID = 0;
		uint32_t range_left = 0, range_right = 0;
		BVHNode *_parent = nullptr;
		BVHNode *_left = nullptr, *_right = nullptr;
		
		glm::vec3 min_BB = glm::vec3(0), max_BB = glm::vec3(0);
	};
	
	// BB_min, BB_max
	template<class Geometry>
	std::vector<BVHNode> ConstructLBVH (const std::vector<Geometry> &geometries)
	{
		BVHNode *root = nullptr;

		std::vector<Key> keys;
		keys.reserve (geometries.size ());
		std::vector<uint8_t> uncommon_upper_bit;
		uncommon_upper_bit.reserve (geometries.size () - 1);

		// Create Scene BB
		glm::vec3 scene_min = geometries[0].BB_min, scene_max = geometries[0].BB_max;
		for (uint32_t i = 1; i < geometries.size (); i++) {
			scene_min.x = MIN (scene_min.x, geometries[i].BB_min.x);
			scene_min.y = MIN (scene_min.y, geometries[i].BB_min.y);
			scene_min.z = MIN (scene_min.z, geometries[i].BB_min.z);

			scene_max.x = MAX (scene_max.x, geometries[i].BB_max.x);
			scene_max.y = MAX (scene_max.y, geometries[i].BB_max.y);
			scene_max.z = MAX (scene_max.z, geometries[i].BB_max.z);
		}

		{// Gen mortan_code
			uint32_t i = 0;
			for (auto &geom : geometries) {
				// BB_centroid
				glm::vec3 p = { (geom.BB_min.x + geom.BB_max.x)*0.5f, (geom.BB_min.y + geom.BB_max.y)*0.5f, (geom.BB_min.z + geom.BB_max.z)*0.5f };
				p -= scene_min;

				p.x /= (scene_max.x - scene_min.x);
				p.y /= (scene_max.y - scene_min.y);
				p.z /= (scene_max.z - scene_min.z);

				keys.push_back ({ morton_code (p), i });
				i++;
			}
		}

		// sort keys acc. mortan_code
		std::stable_sort (keys.begin (), keys.end (), [&](Key a, Key b) -> bool 
						  {
							  return a.MortonCode < b.MortonCode;
						  });
		////// TODO resolve mortan_code conflicts
		
		// Get Highest common upper bit
		for (uint32_t i = 1; i < keys.size (); i++){
			uint32_t xor_result = keys[i - 1].MortonCode ^ keys[i].MortonCode;
			uint8_t highest_uncommon_upper_bit = 0;
			while (xor_result > 0)
				xor_result = (xor_result >> 1), highest_uncommon_upper_bit++;
			uncommon_upper_bit.push_back(highest_uncommon_upper_bit);
		}

		std::vector<BVHNode> Nodes;
		Nodes.reserve (geometries.size ()*2 - 1); // N - 1 -> Internal, N -> leaf 
		// Leaf Nodes 
		{
			uint32_t idx = 0;
			for (Key &key : keys) {
				BVHNode node;
				node.range_left  = idx;
				node.range_right = idx;
				node.idx = idx;

				node.ObjectID = key.ObjectID;
				node.min_BB = geometries[key.ObjectID].BB_min;
				node.max_BB = geometries[key.ObjectID].BB_max;

				Nodes.push_back (node);
				idx++;
			}
		}
		
		// Internal Nodes (Total: N-1)
		std::queue<BVHNode *> InternalNodeQueue;
		// Occupy Places
		for (uint32_t i = 0; i < geometries.size () - 1; i++) {
			BVHNode node;
			node.idx = i;
			Nodes.push_back (node);

			InternalNodeQueue.push(&Nodes.back());
		}
		uint32_t level = uncommon_upper_bit[0];
		for(uint8_t val : uncommon_upper_bit){
			level = MIN(val, level);
		}
		uint32_t nxtLevel = 0, initial_queue_size = InternalNodeQueue.size();
		while(!InternalNodeQueue.empty()){
			BVHNode &currNode = *InternalNodeQueue.front();
			InternalNodeQueue.pop();
			initial_queue_size--;

			bool solved = false;

			if(uncommon_upper_bit[currNode.idx] <= level){
				BVHNode *_left = &Nodes[currNode.idx];
				while(_left->_parent)
					_left = _left->_parent;
				
				BVHNode *_right = &Nodes[currNode.idx + 1];
				while(_right->_parent)
					_right = _right->_parent;

				currNode._left = _left, currNode._right = _right;
				currNode.range_left = _left->range_left, currNode.range_right = _right->range_right;
				_left->_parent = &currNode, _right->_parent = &currNode;

				currNode.min_BB.x = MIN(_left->min_BB.x, _right->min_BB.x);
				currNode.min_BB.y = MIN(_left->min_BB.y, _right->min_BB.y);
				currNode.min_BB.z = MIN(_left->min_BB.z, _right->min_BB.z);

				currNode.max_BB.x = MAX (_left->max_BB.x, _right->max_BB.x);
				currNode.max_BB.y = MAX (_left->max_BB.y, _right->max_BB.y);
				currNode.max_BB.z = MAX (_left->max_BB.z, _right->max_BB.z);

				solved = true;
			} else {
				if(nxtLevel == 0 || nxtLevel > uncommon_upper_bit[currNode.idx])
					nxtLevel = uncommon_upper_bit[currNode.idx];
			}

			if(!solved)
				InternalNodeQueue.push(&currNode);
			if(initial_queue_size == 0){
				level = nxtLevel;
				nxtLevel = 0;
				initial_queue_size = InternalNodeQueue.size();
				if(initial_queue_size == 1)
					root = InternalNodeQueue.front ();
			}
		}

		return Nodes;
	}

	template<class Geometry>
	std::pair<float*, uint32_t> ConstructLBVH_Buff (const std::vector<Geometry> &geometries)
	{
		std::vector<BVHNode> HeirarchyTree = ConstructLBVH (geometries);
		BVHNode *root = &HeirarchyTree.back ();
		while (root->_parent)
			root = root->_parent;

		float *BufferPtr = new float[HeirarchyTree.size () * 4 * 3];// 3*4 for 3*vec4 {(BB_min, _left), (BB_max, _right), (Corresponding_Obj, parent, 0, 0)}

		std::queue<std::pair<BVHNode *, uint32_t>> NodeQueue;
		NodeQueue.push ({ root , 0 });
		uint32_t line = 0;
		while (!NodeQueue.empty ()) {
			BVHNode *currNode = NodeQueue.front ().first;
			uint32_t parent = NodeQueue.front ().second;
			NodeQueue.pop ();

			uint32_t LeftRefIdx = 0, RightRefIdx = 0; // Predicting
			if (currNode->_left) {
				NodeQueue.push ({ currNode->_left, line });
				LeftRefIdx = line + NodeQueue.size ();
			}
			if (currNode->_right) {
				NodeQueue.push ({ currNode->_right, line });
				RightRefIdx = line + NodeQueue.size ();
			}

			// BB_min
			BufferPtr[4*3*line + 0] = currNode->min_BB[0];
			BufferPtr[4*3*line + 1] = currNode->min_BB[1];
			BufferPtr[4*3*line + 2] = currNode->min_BB[2];

			// BB_max
			BufferPtr[4*3*line + 4 + 0] = currNode->max_BB[0];
			BufferPtr[4*3*line + 4 + 1] = currNode->max_BB[1];
			BufferPtr[4*3*line + 4 + 2] = currNode->max_BB[2];

			// child s
			BufferPtr[4*3*line + 0 + 3] = float (LeftRefIdx);
			BufferPtr[4*3*line + 4 + 3] = float (RightRefIdx);

			// ObjectRef & parent
			BufferPtr[4*3*line + 8 + 0] = (LeftRefIdx == 0 && RightRefIdx == 0) ? currNode->ObjectID : 0;
			BufferPtr[4*3*line + 8 + 1] = float (parent);

			line++;
		}

		//for (uint32_t i = 0; i < HeirarchyTree.size (); i++) {
		//	std::cout << "\n#" << i << " BB{Min: " << BufferPtr[4*3*i + 0 + 0] << ", " << BufferPtr[4*3*i + 0 + 1] << ", " << BufferPtr[4*3*i + 0 + 2] << "  Max: " << BufferPtr[4*3*i + 4 + 0]  << ", " << BufferPtr[4*3*i + 4 + 1]  << ", " << BufferPtr[4*3*i + 4 + 2] << " }";
		//	std::cout << " L> " << BufferPtr[4*3*i + 0 + 3] << " R> " << BufferPtr[4*3*i + 4 + 3] << " Prnt> " << BufferPtr[4*3*i + 8 + 1] << " OBJ> " << BufferPtr[4*3*i + 8 + 0];
		//}

		return { BufferPtr, HeirarchyTree.size () };
	}
}