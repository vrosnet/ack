#ifndef NORCSID
static char rcsid[]= "$Header$";
#endif

#include "param.h"
#include "reg.h"
#include "lookup.h"
#include "property.h"
#include "expr.h"
#include "set.h"
#include "varinfo.h"
#include "instruct.h"
#include "token.h"
#include "regvar.h"
#include <cgg_cg.h>
#include "extern.h"

n_proc(name) char *name; {
	register symbol *sy_p;
	extern int npatbytes;

	sy_p = lookup(name,symproc,newsymbol);
	sy_p->sy_value.syv_procoff = npatbytes;
}

struct varinfo *
make_erase(name) char *name; {
	expr_t e,ident_expr();
	struct varinfo *result;

	e = ident_expr(name);
	if (e.ex_typ != TYPREG)
		error("Register name required here");
	NEW(result,struct varinfo);
	result->vi_next = VI_NULL;
	result->vi_int[0] = e.ex_index;
	return(result);
}

n_instr(name,asname,oplist,eraselist,cost)
char *name,*asname;
operand *oplist;
struct varinfo *eraselist,*cost;
{
	register instrno;
	register cc_count;
	register instr_p ip;

	instrno = NEXT(ninstr,MAXINSTR,"Instructions");
	ip = &l_instr[instrno];
	ip->i_name = name;
	ip->i_asname = strlookup(asname!=0 ? asname : name);
	ip->i_nops = 0;
	ip->i_oplist = oplist;
	ip->i_erases = eraselist;
	if (cost==0) {
		ip->i_cost.ct_space = 0;
		ip->i_cost.ct_time = 0;
	} else {
		ip->i_cost.ct_space = cost->vi_int[0];
		ip->i_cost.ct_space = cost->vi_int[1];
	}
	for (cc_count=0; oplist!=0; oplist = oplist->o_next) {
		ip->i_nops++;
		if(oplist->o_adorn&AD_CC)
			cc_count++;
	}
	while (eraselist!=VI_NULL) {
		if (eraselist->vi_int[0] == -1 && cc_count)
			error("Instruction can't both set and break the condition codes");
		eraselist=eraselist->vi_next;
	}
	if (cc_count>1)
		error("No instruction can set condition codes more than once");
}

n_set(name,number) char *name; {
	register symbol *sy_p;

	sy_p = lookup(name,symset,newsymbol);
	sy_p->sy_value.syv_setno = number;
}

n_tok(name,atts,size,cost,format)
char *name;
struct varinfo *atts,*cost,*format;
{
	register symbol *sy_p;
	register token_p tp;
	register struct varinfo *vip;
	int i;
	int tokno;
	char formstr[50],smallstr[2];

	sy_p = lookup(name,symtok,newsymbol);
	NEW(tp,token_t);
	tokno = NEXT(ntokens,MAXTOKENS,"Tokens");
	sy_p->sy_value.syv_tokno = tokno;
	l_tokens[tokno] = tp;
	tp->tk_name = sy_p->sy_name;
	tp->tk_size = size;
	if (cost != 0) {
		tp->tk_cost.ct_space = cost->vi_int[0];
		tp->tk_cost.ct_time  = cost->vi_int[1];
	} else {
		tp->tk_cost.ct_space = 0;
		tp->tk_cost.ct_time  = 0;
	}
	for(i=0,vip=atts;i<MAXATT && vip!=0;i++,vip=vip->vi_next) {
		tp->tk_att[i].ta_type = vip->vi_int[0];
		tp->tk_att[i].ta_name = vip->vi_str[0];
		vip->vi_str[0]=0;
	}
	if (i>maxtokensize)
		maxtokensize=i;
	if (vip!=0)
		error("More then %d attributes, rest discarded",MAXATT);
	for(;i<MAXATT;i++)
		tp->tk_att[i].ta_type= -3;
	if (format!=0) {
		formstr[0] = 0;
		for (vip=format;vip!=0;vip=vip->vi_next) {
			if (vip->vi_int[0]==0)
				strcat(formstr,vip->vi_str[0]);
			else {
				for(i=0;i<MAXATT;i++) {
					if (strcmp(vip->vi_str[0],tp->tk_att[i].ta_name)==0) {
						smallstr[0] = i+1;
						smallstr[1] = 0;
						strcat(formstr,smallstr);
						break;
					}
				}
				if (i==MAXATT)
					error("%s not a known attribute",
						vip->vi_str[0]);
			}
		}
		tp->tk_format = strlookup(formstr);
	} else
		tp->tk_format = -1;
}

checkprintformat(n) {
	register short *s;
	register i;
	extern set_t l_sets[];
	
	s= l_sets[n].set_val;
	for(i=nregs;i<nregs+ntokens;i++)
		if (BIT(s,i) && l_tokens[i-nregs]->tk_format<0)
			error("Token %s in set does not have printformat",
				l_tokens[i-nregs]->tk_name);
}

n_prop(name,size) char *name; int size; {
	int propno;
	register symbol *sp;

	propno = NEXT(nprops,MAXPROPS,"Properties");
	sp = lookup(name,symprop,newsymbol);
	sp->sy_value.syv_propno = propno;
	if (size <= 0) {
		error("Size of property must be >0");
		size = wordsize;
	}
	l_props[propno].pr_size = size;
}

prophall(n) {
	register i;
	short hallset[SETSIZE];
	
	for(i=0;i<SETSIZE;i++)
		hallset[i] = i<SZOFSET(MAXREGS) ? l_props[n].pr_regset[i] : 0;
	nexthall(hallset);
}

n_reg(name,printstring,nmemb,member1,member2) char *name,*printstring; {
	register symbol *sy_p;
	register reginfo *ri_p;
	int regno;

	sy_p = lookup(name,symreg,newsymbol);
	sy_p->sy_value.syv_regno = regno = NEXT(nregs,MAXREGS,"Number of registers");
	ri_p = &l_regs[regno];
	ri_p->ri_name = mystrcpy(name);
	ri_p->ri_repr = printstring!=0 ? mystrcpy(printstring) : ri_p->ri_name;
	ri_p->ri_memb[0] = member1;
	ri_p->ri_memb[1] = member2;
	if (nmemb>maxmembers)
		maxmembers=nmemb;
	return(regno);
}

make_const() {

	wordsize = cmustbeset("EM_WSIZE");
	pointersize = cmustbeset("EM_PSIZE");
}

cmustbeset(ident) char *ident; {

	return(lookup(ident,symconst,mustexist)->sy_value.syv_cstval);
}

n_const(ident,val) char *ident; {
	register symbol *sy_p;

	sy_p = lookup(ident,symconst,newsymbol);
	sy_p->sy_value.syv_cstval = val;
}

n_sconst(ident,val) char *ident,*val; {
	register symbol *sy_p;

	sy_p = lookup(ident,symsconst,newsymbol);
	sy_p->sy_value.syv_stringno = strlookup(val);
}

regline(rl,pl,rv) varinfo *rl,*pl; {
	register varinfo *rrl,*rpl;
	register short *sp;
	register reginfo *regp;
	int thissize;
	int propno;

	for(rrl=rl;rrl!=0;rrl=rrl->vi_next) {
		regp = &l_regs[rrl->vi_int[0]];
		thissize = 0;
		for(rpl=pl;rpl!=0;rpl=rpl->vi_next) {
			propno = rpl->vi_int[0];
			sp= l_props[propno].pr_regset;
			BIS(sp,rrl->vi_int[0]);
			if (thissize==0)
				thissize = l_props[propno].pr_size;
			else if (thissize!=-1 && thissize!=l_props[propno].pr_size)
				error("Register %s has no clear size",
					regp->ri_name);
		}
		regp->ri_size = thissize;
		regp->ri_class = regclass;
		regp->ri_rregvar = rv;
		if (rv>=0) {
			if (regp->ri_memb[0]!=0)
				error("Register variables may not have subregisters");
			rvused |= ANY_REGVAR;
			if (regp->ri_size == wordsize)
				rvused |= SL_REGVAR;
			else if (regp->ri_size == 2*wordsize)
				rvused |= DL_REGVAR;
			if (nregvar[rv]==0)
				rvsize[rv] = regp->ri_size;
			else if (rvsize[rv]!=regp->ri_size)
				error("All register variables of one type must have the same size");
			NEXT(nregvar[rv],MAXREGVAR,"Register variable");
			rvnumbers[rv][nregvar[rv]-1] = rrl->vi_int[0];
		}
	}
	regclass++;
}

setallreg(vi) struct varinfo *vi; {

	nallreg=0;
	for(;vi!=0;vi=vi->vi_next) {
		if (vi->vi_int[0]<0)
			continue;
		allreg[nallreg++] = vi->vi_int[0];
	}
}

freevi(vip) register struct varinfo *vip; {
	register i;
	extern char *end;

	if (vip==0)
		return;
	freevi(vip->vi_next);
	freevi(vip->vi_vi);
	for (i=0;i<VI_NSTR;i++)
		if (vip->vi_str[i]>end)
			free((char *) vip->vi_str[i]);
	free(vip);
}

int myatoi(s) register char *s; {
	register int base=10;
	register sum=0;

	if (*s=='0') {
		base = 8;
		s++;
		if (*s=='x') {
			base=16;
			s++;
		}
	}
	for (;;) {
		switch (*s) {
		default:	return(sum);
		case '8':
		case '9':
			if (base==8) error("Bad digit in octal number");
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
			sum = sum*base + *s++ - '0';
			break;
		case 'a':
		case 'b':
		case 'c':
		case 'd':
		case 'e':
		case 'f':
			if (base!=16) error("Hexletter in number not expected");
			sum = sum*base + 10 + *s++ - 'a';
			break;
		case 'A':
		case 'B':
		case 'C':
		case 'D':
		case 'E':
		case 'F':
			if (base!=16) error("Hexletter in number not expected");
			sum = sum*base + 10 + *s++ - 'A';
			break;
		}
	}
}

char *mystrcpy(s) char *s; {
	register char *p;
	char *myalloc();

	p=myalloc(strlen(s)+1);
	strcpy(p,s);
	return(p);
}

char *myalloc(n) register n; {
	register char *p,*result;
	char *malloc();

	result=p=malloc(n);
	if (p== (char *) 0)
		fatal("Out of memory");
	do *p++=0; while (--n);
	return(result);
}

chkincl(value,lwb,upb) {

	if (value<lwb || value>upb)
		error("Number %d should have been between %d and %d",
			value,lwb,upb);
	return(value);
}

subset(sp1,sp2,setsize) short *sp1,*sp2; {
	register i;

	for(i=0;i<setsize;i++)
		if ( (sp1[i] | sp2[i]) != sp2[i])
			return(0);
	return(1);
}

vilength(vip) register struct varinfo *vip; {
	register l=0;

	while(vip!=0) {
		vip=vip->vi_next;
		l++;
	}
	return(l);
}
