#ifndef RULE_EXTRACTOR_H
#define RULE_EXTRACTOR_H
#include "stdafx.h"
#include "myutils.h"
#include "tree_str_pair.h"

class RuleExtractor
{
	public:
		RuleExtractor(const string &line_tree,const string &line_str,const string &line_align);
		~RuleExtractor()
		{
			delete tspair;
		}
		void extract_rules();

	private:
		void extract_GHKM_rules(SyntaxNode* node);
		void find_frontier_frag(SyntaxNode* node,Rule &rule);
		void attach_unaligned_words();
		void extract_SPMT_rules();
		bool find_syntax_phrase_frag(SyntaxNode* node,Rule &rule,pair<int,int> src_span);
		pair<int,int> cal_src_span_for_tgt_span(pair<int,int> tgt_span);
		bool check_alignment_for_src_span(pair<int,int> src_span,pair<int,int> tgt_span);
		void extract_compose_rules(SyntaxNode* node);
		void expand_rule(Rule &rule,vector<Rule>* composed_rules);

	private:
		TreeStrPair *tspair;
};

#endif

