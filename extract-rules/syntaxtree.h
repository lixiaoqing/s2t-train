#ifndef SYNTAXTREE_H
#define SYNTAXTREE_H
#include "stdafx.h"
#include "myutils.h"

struct Rule
{
	string src_side;
	string tgt_side;
	int type;                                         // 规则类型，1为最小规则，2为扩展了未对齐单词的最小规则，3为SMPT规则，4为组合规则
};

// 源端句法树节点
struct SyntaxNode
{
	string label;                                    // 该节点的句法标签或者词
	SyntaxNode* father;
	vector<SyntaxNode*> children;
	pair<int,int> src_span;                          // 该节点对应的源端span,用首位两个单词的位置表示
	pair<int,int> tgt_span;                          // 该节点对应的目标端span
	int type;                                        // 节点类型，0：单词节点，1：边界节点，2：非边界节点
	vector<Rule> rules;								 // 该节点能抽取的所有规则
	
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
		void dump(SyntaxNode* node);

	private:
		void build_tree_from_str(const string &line_of_tree);
		void check_frontier_for_nodes_in_subtree(SyntaxNode* node);

	public:
		SyntaxNode* root;
		vector<SyntaxNode*> word_nodes;
		vector<pair<int,int> > *src_idx_to_tgt_span;          				// 记录每个源语言单词对应的目标端span
		vector<vector<int> > *tgt_idx_to_src_idx;           			    // 记录每个目标语言单词对应的源端单词位置
		vector<vector<SyntaxNode*> > tgt_span_lbound_to_frontier_nodes;     // 将tgt_span左边界相同的边界节点放到一起，处理目标语言未对齐的单词用
		vector<vector<SyntaxNode*> > tgt_span_rbound_to_frontier_nodes;     // 将tgt_span右边界相同的边界节点放到一起，处理目标语言未对齐的单词用
};

#endif
