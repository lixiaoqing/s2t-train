#ifndef SYNTAXTREE_H
#define SYNTAXTREE_H
#include "stdafx.h"
#include "myutils.h"

// 源端句法树节点
struct SyntaxNode
{
	string label;                                    // 该节点的句法标签或者词
	SyntaxNode* father;
	vector<SyntaxNode*> children;
	pair<int,int> src_span;                          // 该节点对应的源端span
	pair<int,int> tgt_span;                          // 该节点对应的目标端span
	int type;                                        // 节点类型，0：单词节点，1：边界节点，2：非边界节点
	
	SyntaxNode ()
	{
		father   = NULL;
		src_span = make_pair(-1,-1);
		tgt_span = make_pair(-1,-1);
		type     = -1;
	}
	~SyntaxNode ()
	{
		for (auto node : children)
		{
			delete node;
		}
	}
};

class SyntaxTree
{
	public:
		SyntaxTree(const string &line_of_tree,vector<pair<int,int> > *si2ts,vector<vector<int> > *ti2si);
		~SyntaxTree()
		{
			delete root;
		}

	private:
		void build_tree_from_str(const string &line_of_tree);
		void check_frontier_for_nodes_in_subtree(SyntaxNode* node);
		void dump(SyntaxNode* node);

	public:
		SyntaxNode* root;
		vector<string> words;
		vector<pair<int,int> > *src_idx_to_tgt_span;          // 记录每个源语言单词对应的目标端span
		vector<vector<int> > *tgt_idx_to_src_idx;             // 记录每个目标语言单词对应的源端单词位置
};

#endif
