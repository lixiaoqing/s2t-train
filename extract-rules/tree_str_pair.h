#ifndef TREE_STR_PAIR_H
#define TREE_STR_PAIR_H
#include "stdafx.h"
#include "myutils.h"


struct SyntaxNode;

struct Rule
{
	vector<SyntaxNode*> src_tree_frag;				//按照先序顺序记录规则源端句法树片段中的节点
	vector<int> src_node_status;					//记录规则源端每个节点的状态，i表示第i个变量节点，-1表示根节点，-2表示内部节点，-3表示单词节点
	vector<pair<int,int> > tgt_spans_for_src_node;  //记录规则源端每个节点在目标端的span
	pair<int,int> tgt_span;							//记录规则目标端的span
	vector<int> tgt_word_status;					//记录目标端span中每个单词的状态，i表示被源端第i个变量替换，-1表示没被替换，-2表示被跳过的未对齐的词(-2状态用于SPMT规则)
	int variable_num;								//规则中变量的个数
	int type;                                       //规则类型，1为最小规则，2为扩展了未对齐单词的最小规则，3为SMPT规则，4为组合规则
	int size;                            	        //规则大小，表示规则由几个最小规则组合而成
	Rule ()
	{
		tgt_span = make_pair(-1,-1);
		variable_num = 0;
		type = 1;
		size = 1;
	}
};

// 源端句法树节点
struct SyntaxNode
{
	string label;                                   // 该节点的句法标签或者词
	SyntaxNode* father;
	vector<SyntaxNode*> children;
	pair<int,int> src_span;                         // 该节点对应的源端span,用首位两个单词的位置表示
	pair<int,int> tgt_span;                         // 该节点对应的目标端span
	int type;                                       // 节点类型，0：单词节点，1：边界节点，2：非边界节点
	vector<Rule> rules;								// 该节点能抽取的所有规则
	
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

class TreeStrPair
{
	public:
		TreeStrPair(const string &line_tree,const string &line_str,const string &line_align);
		~TreeStrPair()
		{
			delete root;
		}
		void dump_rule(SyntaxNode* node);

	private:
		void load_alignment(const string &align_line);
		void build_tree_from_str(const string &line_of_tree);
		void check_frontier_for_nodes_in_subtree(SyntaxNode* node);

	public:
		SyntaxNode* root;
		vector<SyntaxNode*> word_nodes;
		vector<pair<int,int> > src_idx_to_tgt_span;   						// 记录每个源语言单词对应的目标端span
		vector<pair<int,int> > tgt_idx_to_src_span;   						// 记录每个目标语言单词对应的源端span
		vector<vector<int> > src_idx_to_tgt_idx;     					    // 记录每个源语言单词对应的目标端单词位置
		vector<vector<int> > tgt_idx_to_src_idx;      						// 记录每个目标语言单词对应的源端单词位置
		vector<string> tgt_words;
		int tgt_sen_len;
};

#endif
