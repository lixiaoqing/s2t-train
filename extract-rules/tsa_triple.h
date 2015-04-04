#ifndef TSA_TRIPLE_H
#define TSA_TRIPLE_H
#include "stdafx.h"
#include "myutils.h"
#include "syntaxtree.h"

class TSATriple
{
	public:
		TSATriple(const string &line_tree,const string &line_str,const string &line_align);
		~TSATriple()
		{
			delete src_tree;
		}
		void extract_GHKM_rules(const SyntaxNode* node);

	private:
		void load_alignment(const string &align_line);
		void find_frontier_frag(const SyntaxNode* node,string &src_tree_frag,vector<int> &tgt_word_status,int &variable_num);

	private:
		SyntaxTree *src_tree;
		vector<string> tgt_words;
		vector<pair<int,int> > src_idx_to_tgt_span;   // 记录每个源语言单词对应的目标端span
		vector<vector<int> > tgt_idx_to_src_idx;      // 记录每个目标语言单词对应的源端单词位置

};

#endif

