/*  I N L I N E   S U B S T I T U T I O N */
#include <stdio.h>
#include "../share/types.h"
#include "il.h"
#include "../share/debug.h"
#include "../share/alloc.h"
#include "../share/global.h"
#include "../share/lset.h"
#include "../share/files.h"
#include "../../../h/em_mnem.h"
#include "../../../h/em_pseu.h"
#include "../share/map.h"
#include "il1_anal.h"
#include "il2_aux.h"
#include "il3_subst.h"
#include "../share/get.h"
#include "../share/put.h"
#include "../share/go.h"

int calnr;
calcnt_p cchead;	/* call-count info of current proc */
STATIC short space = 0;

STATIC char cname[] = "/usr/tmp/ego.i1.XXXXXX";
STATIC char ccname[] = "/usr/tmp/ego.i2.XXXXXX";

/* For debugging only */
STATIC char sname[] = "/usr/tmp/ego.i3.XXXXXX";

int Ssubst;
#ifdef VERBOSE
int Senv,Srecursive,Slocals,Sinstrlab,Sparsefails,Spremoved,Scals;
int Sbig_caller,Sdispensable,Schangedcallee,Sbigcallee,Sspace,Szeroratio;
#endif

/* P A S S  1
 *
 * Pass 1 reads and analyses the EM text and the CFG.
 * It determines for every procedure if it may be expanded
 * in line and how it uses its formal parameters.
 * It also collects all calls appearing in the program and
 * recognizes the actual parameters of every call.
 * The call descriptors are put in a file (calfile).
 */

pass1(lnam,bnam,cnam)
	char *lnam, *bnam, *cnam;
{
	FILE *f, *gf, *cf, *ccf; /* The EM input, the basic block graph,
				  * the call-list file and the calcnt file.
				  */
	long laddr;
	bblock_p g;
	short kind;
	line_p l;

	f = openfile(lnam,"r");
	gf = openfile(bnam,"r");
	cf = openfile(cnam,"w");
	ccf = openfile(ccname,"w");
	mesregs = Lempty_set();
	apriori(fproc);
	/* use information from the procedure table to
	 * see which calls certainly cannot be expanded.
	 */
	while(TRUE) {
		laddr = ftell(f);
		if (!getunit(gf,f,&kind,&g,&l,&curproc,TRUE)) break;
		/* Read the control flow graph and EM text of
		 * one procedure and analyze it.
		 */
		if (kind == LDATA) {
			remunit(LDATA,(proc_p) 0,l);
			continue;
		}
		/* OUTTRACE("flow graph of proc %d read",curproc->p_id); */
		assert(INSTR(g->b_start) == ps_pro);
		curproc->p_start = g;
		curproc->P_LADDR = laddr;
		/* address of em text in em-file */
		/* address of graph in basic block file */
		curproc->P_SIZE = proclength(curproc); /* #instructions */
		if (BIG_PROC(curproc)) {
			/* curproc is too large to be expanded in line */
			UNSUITABLE(curproc);
		}
		calnr = 0;
		anal_proc(curproc,cf,ccf);
		/* OUTTRACE("proc %d processed",curproc->p_id); */
		remunit(LTEXT,curproc,(line_p) 0);
		/* remove control flow graph + text */
		/* OUTTRACE("graph of proc %d removed",curproc->p_id); */
		Ldeleteset(mesregs);
		mesregs = Lempty_set();
	}
	fclose(f);
	fclose(gf);
	fclose(cf);
	fclose(ccf); 
}



/* P A S S  2
 *
 * Pass 2 reads the calfile and determines which calls should
 * be expanded in line. It does not use the EM text.
 */



STATIC char cname2[] = "/usr/tmp/ego.i4.XXXXXX";

pass2(cnam,space)
	char *cnam;
	short space;
{
	FILE  *cf, *cf2, *ccf;
	call_p c,a;

	cf = openfile(cnam,"r");
	cf2 = openfile(cname2,"w");
	ccf = openfile(ccname,"r");
	while ((c = getcall(cf)) != (call_p) 0) {
		/* process all calls */
		if (SUITABLE(c->cl_proc)) {
			/* called proc. may be put in line */
			anal_params(c);
			/* see which parameters may be put in line */
			assign_ratio(c); /* assign a rank */
			a = abstract(c); /* abstract essential info */
			append_abstract(a,a->cl_caller);
			/* put it in call-list of calling proc. */
			putcall(c,cf2,(short) 0);
		} else {
			rem_call(c);
		}
	}
	select_calls(fproc,ccf,space);
	fclose(cf); unlink(cnam);
	fclose(cf2);
	fclose(ccf); unlink(ccname);
	cf2 = openfile(cname2,"r");
	add_actuals(fproc,cf2);
	cleancals(fproc); /* remove calls that were not selected */
	/* add actual parameters to each selected call */
	fclose(cf2); unlink(cname2);
}



/* P A S S  3
 *
 * pass 3 reads the substitution file and performs all
 * substitutions described in that file. It reads the
 * original EM text and produced a new (optimized)
 * EM textfile.
 */


pass3(lnam,lnam2)
	char *lnam,*lnam2;
{
	bool verbose = TRUE;
	FILE *lfile, *lfilerand, *lfile2, *sfile;
	call_p c,next;
	line_p l,startscan,cal;
	short lastcid; /* last call-id seen */

	lfile = openfile(lnam, "r");
	lfilerand = openfile(lnam, "r");
	lfile2 = openfile(lnam2,"w");
	if (verbose) {
		sfile = openfile(sname,"w");
	}
	mesregs = Lempty_set();
	while ((l = get_text(lfile,&curproc)) != (line_p) 0) {
		if (curproc == (proc_p) 0) {
			/* Just a data-unit; no real instructions */
			putlines(l->l_next,lfile2);
			oldline(l);
			continue;
		}
		if (IS_DISPENSABLE(curproc)) {
			liquidate(curproc,l->l_next);
		} else {
			startscan = l->l_next;
			lastcid = 0;
			for (c = curproc->P_CALS; c != (call_p) 0; c = next) {
				next = c->cl_cdr;
				cal = scan_to_cal(startscan,c->cl_id - lastcid);
				assert (cal != (line_p) 0);
				startscan = scan_to_cal(cal->l_next,1);
				/* next CAL */
				lastcid = c->cl_id;
				/* next CAL after current one */
				substitute(lfilerand,c,cal,l->l_next);
				if (verbose) {
					putcall(c,sfile,0);
				} else {
					rem_call(c);
				}
			}
		}
		putlines(l->l_next,lfile2);
		Ldeleteset(mesregs);
		mesregs = Lempty_set();
		oldline(l);
	}
	fclose(lfile);
	fclose(lfile2);
	if (verbose) {
		fclose(sfile);
		unlink(sname);
	}
}


STATIC il_extptab(ptab)
	proc_p ptab;
{
	/* Allocate space for extension of proctable entries.
	 * Also, initialise some of the fields just allocated.
	 */

	register proc_p p;

	for (p = ptab; p != (proc_p) 0; p = p->p_next) {
		p->p_extend = newilpx();
		p->P_ORGLABELS = p->p_nrlabels;
		p->P_ORGLOCALS = p->p_localbytes;
	}
}

STATIC il_cleanptab(ptab)
	proc_p ptab;
{
	/* De-allocate space for extensions */

	register proc_p p;

	for (p = ptab; p != (proc_p) 0; p = p->p_next) {
		oldilpx(p->p_extend);
	}
}

#ifdef VERBOSE
Sdiagnostics()
{
	/* print statictical information */

	fprintf(stderr,"STATISTICS:\n");
	fprintf(stderr,"Info about procedures:\n");
	fprintf(stderr,"environment accessed: %d\n",Senv);
	fprintf(stderr,"recursive: %d\n",Srecursive);
	fprintf(stderr,"too many locals: %d\n",Slocals);
	fprintf(stderr,"instr. lab in data block: %d\n",Sinstrlab);
	fprintf(stderr,"procedures removed: %d\n",Spremoved);
	fprintf(stderr,"\nInfo about calls:\n");
	fprintf(stderr,"total number of calls: %d\n",Scals);
	fprintf(stderr,"total number of calls substituted: %d\n",Ssubst);
	fprintf(stderr,"parser failed: %d\n",Sparsefails);
	fprintf(stderr,"caller too big: %d\n",Sbig_caller);
	fprintf(stderr,"caller dispensable: %d\n",Sdispensable);
	fprintf(stderr,"callee is changed: %d\n",Schangedcallee);
	fprintf(stderr,"callee too big: %d\n",Sbigcallee);
	fprintf(stderr,"no space available: %d\n",Sspace);
	fprintf(stderr,"zero ratio: %d\n",Szeroratio);
}
#endif

il_flags(p)
	char *p;
{
	if (*p++ == 's') {
		while (*p != '\0') {
			space = 10*space +*p++ -'0';
		}
	}
}

main(argc,argv)
	int argc;
	char *argv[];
{
	FILE *f;
	
	go(argc,argv,no_action,no_action,no_action,il_flags);
	il_extptab(fproc); /* add extended data structures */
	mktemp(cname);
	mktemp(ccname);
	mktemp(sname);
	mktemp(cname2);
	pass1(lname,bname,cname); /* grep calls, analyse procedures */
	pass2(cname,space);  /* select calls to be expanded */
	pass3(lname,lname2); /* do substitutions */
	f = openfile(dname2,"w");
	il_cleanptab(fproc); /* remove extended data structures */
	putdtable(fdblock,f);
	f = openfile(pname2,"w");
	putptable(fproc,f,FALSE);
	report("inline substitutions",Ssubst);
#ifdef VERBOSE
	if (verbose_flag) {
		Sdiagnostics();
	}
#endif
#ifdef DEBUG
	core_usage();
#endif
	exit(0);
}
