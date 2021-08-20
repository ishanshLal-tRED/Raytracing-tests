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

		glm::vec3 min_BB = glm::vec3 (0), max_BB = glm::vec3 (0);
	};
	struct BVHNodeBuff // Note this data can be represented only by 2 vec4
	{// Like, -tve no.s can be used for representing data, when left is positive, it refers to child Index, -tve then ObjectID, right can be inferred from lefts position(as they are stored next to each other), and in place of right store parents Index
		float bb_min[3] = { 0,0,0 };
		float bb_max[3] = { 0,0,0 };
		float leftData = 0;
		float rightData = 0;
	}; // Yahoo no padding

	// BB_min, BB_max
	template<class Geometry>
	std::vector<BVHNode> ConstructLBVH (Geometry *geometries, const uint32_t NumberOfGeometries)
	{
		LOG_ASSERT (NumberOfGeometries > 0);
		if (NumberOfGeometries == 1) {
			auto [minBB, maxBB] = geometries[0].CalculateBBMinMax ();
			std::vector<BVHNode> Nodes;
			Nodes.resize (1);
			Nodes.front ().min_BB = minBB;
			Nodes.front ().max_BB = maxBB;
			return Nodes;
		}
		BVHNode *root = nullptr;

		std::vector<std::pair<glm::vec3, glm::vec3>> geometries_BBs;
		geometries_BBs.reserve (NumberOfGeometries);
		for (uint32_t i = 0; i < NumberOfGeometries; i++) {
			geometries_BBs.emplace_back (geometries[i].CalculateBBMinMax ());
		}
		geometries = nullptr;

		std::vector<Key> keys;
		keys.reserve (NumberOfGeometries);
		std::vector<uint8_t> uncommon_upper_bit;
		uncommon_upper_bit.reserve (NumberOfGeometries - 1);

		// Create Scene BB
		glm::vec3 scene_min = geometries_BBs[0].first, scene_max = geometries_BBs[0].second;
		for (uint32_t i = 1; i < NumberOfGeometries; i++) {
			scene_min.x = MIN (scene_min.x, geometries_BBs[i].first.x);
			scene_min.y = MIN (scene_min.y, geometries_BBs[i].first.y);
			scene_min.z = MIN (scene_min.z, geometries_BBs[i].first.z);
											
			scene_max.x = MAX (scene_max.x, geometries_BBs[i].second.x);
			scene_max.y = MAX (scene_max.y, geometries_BBs[i].second.y);
			scene_max.z = MAX (scene_max.z, geometries_BBs[i].second.z);
		}

		{// Gen mortan_code
			uint32_t i = 0;
			for (auto &geom : geometries_BBs) {
				// BB_centroid
				glm::vec3 p = { (geom.first.x + geom.second.x)*0.5f, (geom.first.y + geom.second.y)*0.5f, (geom.first.z + geom.second.z)*0.5f };
				p -= scene_min;

				p.x /= (scene_max.x - scene_min.x);
				p.y /= (scene_max.y - scene_min.y);
				p.z /= (scene_max.z - scene_min.z);

				keys.push_back ({ morton_code (p), i });
				i++;
			}
		}

		// sort keys acc. mortan_code
		std::sort (keys.begin (), keys.end (),
				   [&](Key a, Key b) -> bool {
					   if (a.MortonCode == b.MortonCode) {
						   glm::vec3 A = geometries_BBs[a.ObjectID].second - geometries_BBs[a.ObjectID].first;
						   glm::vec3 B = geometries_BBs[b.ObjectID].second - geometries_BBs[b.ObjectID].first;
						   return glm::dot (A, A) < glm::dot (B, B);
					   }
					   return a.MortonCode < b.MortonCode;
				   });
		////// TODO resolve mortan_code conflicts

		// Get Highest common upper bit
		for (uint32_t i = 1; i < keys.size (); i++) {
			uint32_t xor_result = keys[i - 1].MortonCode ^ keys[i].MortonCode;
			uint8_t highest_uncommon_upper_bit = 0;
			while (xor_result > 0)
				xor_result = (xor_result >> 1), highest_uncommon_upper_bit++;
			uncommon_upper_bit.push_back (highest_uncommon_upper_bit);
		}

		std::vector<BVHNode> Nodes;
		Nodes.reserve (NumberOfGeometries*2 - 1); // N - 1 -> Internal, N -> leaf 
		// Leaf Nodes 
		{
			uint32_t idx = 0;
			for (Key &key : keys) {
				BVHNode node;
				node.range_left = idx;
				node.range_right = idx;
				node.idx = idx;

				node.ObjectID = key.ObjectID;
				node.min_BB = geometries_BBs[key.ObjectID].first;
				node.max_BB = geometries_BBs[key.ObjectID].second;

				Nodes.push_back (node);
				idx++;
			}
		}

		// Internal Nodes (Total: N-1)
		std::queue<BVHNode *> InternalNodeQueue;
		// Occupy Places
		for (uint32_t i = 0; i < NumberOfGeometries - 1; i++) {
			BVHNode node;
			node.idx = i;
			Nodes.push_back (node);

			InternalNodeQueue.push (&Nodes.back ());
		}
		uint32_t level = uncommon_upper_bit[0];
		for (uint8_t val : uncommon_upper_bit) {
			level = MIN (val, level);
		}
		uint32_t nxtLevel = 0, initial_queue_size = InternalNodeQueue.size ();
		while (!InternalNodeQueue.empty ()) {
			BVHNode &currNode = *InternalNodeQueue.front ();
			InternalNodeQueue.pop ();
			initial_queue_size--;

			bool solved = false;

			if (uncommon_upper_bit[currNode.idx] <= level) {
				BVHNode *_left = &Nodes[currNode.idx];
				while (_left->_parent)
					_left = _left->_parent;

				BVHNode *_right = &Nodes[currNode.idx + 1];
				while (_right->_parent)
					_right = _right->_parent;

				currNode._left = _left, currNode._right = _right;
				currNode.range_left = _left->range_left, currNode.range_right = _right->range_right;
				_left->_parent = &currNode, _right->_parent = &currNode;

				currNode.min_BB.x = MIN (_left->min_BB.x, _right->min_BB.x);
				currNode.min_BB.y = MIN (_left->min_BB.y, _right->min_BB.y);
				currNode.min_BB.z = MIN (_left->min_BB.z, _right->min_BB.z);

				currNode.max_BB.x = MAX (_left->max_BB.x, _right->max_BB.x);
				currNode.max_BB.y = MAX (_left->max_BB.y, _right->max_BB.y);
				currNode.max_BB.z = MAX (_left->max_BB.z, _right->max_BB.z);

				solved = true;
			} else { // Else find next level's uncommon_upper_bit
				if (nxtLevel == 0 || nxtLevel > uncommon_upper_bit[currNode.idx]) 
					nxtLevel = uncommon_upper_bit[currNode.idx];
			}

			if (!solved)
				InternalNodeQueue.push (&currNode);
			if (initial_queue_size == 0) {
				level = nxtLevel;
				nxtLevel = 0;
				initial_queue_size = InternalNodeQueue.size ();
				if (initial_queue_size == 1)
					root = InternalNodeQueue.front ();
			}
		}

		return Nodes;
	}

	template<class Geometry>
	std::pair<BVHNodeBuff*, const uint32_t> ConstructLBVH_Buff (Geometry *geometries, const uint32_t NumberOfGeometries)
	{
		std::vector<BVHNode> HeirarchyTree = ConstructLBVH (geometries, NumberOfGeometries);
		geometries = nullptr;

		BVHNode *root = &HeirarchyTree.back ();
		while (root->_parent) // aking sure it really is
			root = root->_parent; 

		BVHNodeBuff *BufferPtr = new BVHNodeBuff[HeirarchyTree.size ()];// 3*4 for 3*vec4 {(BB_min, _left), (BB_max, _right), (Corresponding_Obj, parent, 0, 0)}

		std::queue<std::pair<BVHNode *, uint32_t>> NodeQueue;
		NodeQueue.push ({ root , 0 });
		uint32_t index = 0;
		while (!NodeQueue.empty ()) {
			BVHNode *currNode = NodeQueue.front ().first;
			uint32_t parent = NodeQueue.front ().second;
			NodeQueue.pop ();

			uint32_t LeftRefIdx = 0, RightRefIdx = 0; // Predicting
			if (currNode->_left) {
				NodeQueue.push ({ currNode->_left, index });
				LeftRefIdx = index + NodeQueue.size ();
			}
			if (currNode->_right) {
				NodeQueue.push ({ currNode->_right, index });
				RightRefIdx = index + NodeQueue.size ();
			}

			// BB_min
			BufferPtr[index].bb_min[0] = currNode->min_BB[0];
			BufferPtr[index].bb_min[1] = currNode->min_BB[1];
			BufferPtr[index].bb_min[2] = currNode->min_BB[2];

			// BB_max
			BufferPtr[index].bb_max[0] = currNode->max_BB[0];
			BufferPtr[index].bb_max[1] = currNode->max_BB[1];
			BufferPtr[index].bb_max[2] = currNode->max_BB[2];

			if (LeftRefIdx > 0 && RightRefIdx > 0) {
				LOG_ASSERT (LeftRefIdx == RightRefIdx - 1);
			} else {
				LOG_ASSERT (LeftRefIdx == RightRefIdx);
				LOG_ASSERT (LeftRefIdx == 0);
			}
			// child, ObjRef, Parent
			BufferPtr[index].leftData = (LeftRefIdx == 0) ? -float (currNode->ObjectID) : float (LeftRefIdx);
			BufferPtr[index].rightData = float (parent);

			index++;
		}

		return { BufferPtr, HeirarchyTree.size () };
	}
}