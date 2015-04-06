#ifndef RULE_EXTRACTOR_H
#define RULE_EXTRACTOR_H
#include "stdafx.h"
#include "myutils.h"
#include "syntaxtree.h"

class RuleExtractor
{
	public:
		RuleExtractor(const string &line_tree,const string &line_str,const string &line_align);
		~RuleExtractor()
		{
			delete src_tree;
		}
		void extract_rules();

	private:
		void load_alignment(const string &align_line);    // TODO 移到句法树文件中去
		void extract_GHKM_rules(SyntaxNode* node);
		void find_frontier_frag(SyntaxNode* node,Rule &rule);
		void attach_unaligned_words();
		void extract_SPMT_rules();
		bool find_syntax_phrase_frag(SyntaxNode* node,Rule &rule,pair<int,int> src_span);
		pair<int,int> cal_src_span_for_tgt_span(pair<int,int> tgt_span);
		bool check_alignment_for_src_span(pair<int,int> src_span,pair<int,int> tgt_span);

	private:
		SyntaxTree *src_tree;
		vector<string> tgt_words;
		int tgt_sen_len;
		vector<pair<int,int> > src_idx_to_tgt_span;   // 记录每个源语言单词对应的目标端span
		vector<pair<int,int> > tgt_idx_to_src_span;   // 记录每个目标语言单词对应的源端span
		vector<vector<int> > src_idx_to_tgt_idx;      // 记录每个源语言单词对应的目标端单词位置
		vector<vector<int> > tgt_idx_to_src_idx;      // 记录每个目标语言单词对应的源端单词位置

};

#endif

