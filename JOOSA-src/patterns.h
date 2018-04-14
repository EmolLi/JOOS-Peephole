/*
 * JOOS is Copyright (C) 1997 Laurie Hendren & Michael I. Schwartzbach
 *
 * Reproduction of all or part of this software is permitted for
 * educational or research use on condition that this copyright notice is
 * included in any copy. This software comes with no warranty of any
 * kind. In no event will the authors be liable for any damages resulting from
 * use of this software.
 *
 * email: hendren@cs.mcgill.ca, mis@brics.dk
 */

/* iload x        iload x        iload x
 * ldc 0          ldc 1          ldc 2
 * imul           imul           imul
 * ------>        ------>        ------>
 * ldc 0          iload x        iload x
 *                               dup
 *                               iadd
 */

#include <stdlib.h>
#include <string.h>

/* nth next code */
CODE* nextN(CODE* c, int n){
    CODE* result = c;
    while (n > 0){
        result = next(result);
        n --;
    }
    return result;
}


int simplify_multiplication_right(CODE **c)
{ int x,k;
  if (is_iload(*c,&x) &&
      is_ldc_int(next(*c),&k) &&
      is_imul(next(next(*c)))) {
     if (k==0) return replace(c,3,makeCODEldc_int(0,NULL));
     else if (k==1) return replace(c,3,makeCODEiload(x,NULL));
     else if (k==2) return replace(c,3,makeCODEiload(x,
                                       makeCODEdup(
                                       makeCODEiadd(NULL))));
     return 0;
  }
  return 0;
}

/* dup
 * astore x
 * pop
 * -------->
 * astore x
 * reason: apparently there is no need to do dup and pop around astore
 */
int simplify_astore(CODE **c)
{ int x;
  if (is_dup(*c) &&
      is_astore(next(*c),&x) &&
      is_pop(next(next(*c)))) {
     return replace(c,3,makeCODEastore(x,NULL));
  }
  return 0;
}








/*
 *
 *  aconst_null
 *  if_acmpeq/if_acmpne l1
 *  ----->
 *  ifnull/ifnonnull l1
 *
 * reason: obviously there we can combine the two steps (load null, compare with it )
 * into one step compareWithNull
 */
int aconst_null_cmp(CODE **c)
{
    int l;
    if (is_aconst_null(*c)){
        if (is_if_acmpeq(next(*c), &l)){
            return replace(c, 2, makeCODEifnull(l, NULL));
        }
        if (is_if_acmpne(next(*c), &l) ){
            return replace(c, 2, makeCODEifnonnull(l, NULL));
        }
    }
    return 0;
}

/* dup
 * aload x
 * swap
 * putfield
 * pop
 * -------->
 * aload x
 * swap
 * putfield
 *
 * reason: the dupped value is not used in any way before we pop it out
 */
int simplify_dup_swap_putfield_pop(CODE **c)
{   int x;
    char* args = (char*) malloc(100*sizeof(char));
  if (
        is_dup(*c) &&
        is_aload(next(*c), &x) &&
        is_swap(nextN(*c, 2)) &&
        is_putfield(nextN(*c, 3), &args) &&
        is_pop(nextN(*c, 4))){
            return replace(c, 5, makeCODEaload(x, makeCODEswap(makeCODEputfield(args, NULL))));
        }
  return 0;
}

/*
reason: if a label is not used, we can safely remove it
*/
int dead_label(CODE** c){
    int l;
    if (is_label(*c, &l) && deadlabel(l)) return replace(c, 1, NULL);
    return 0;

}

/*
    load a
    load b
    swap
    -----
    load b
    load a

    reason: load two values then swap is equal to load two value in reverse order
*/
int simplify_swap(CODE **c)
{   int a;
    int b;
    char* str = (char*) malloc(100*sizeof(char));
  if (
        is_ldc_string(*c, &str) &&
        is_aload(next(*c), &b) &&
        is_swap(nextN(*c, 2))
    ) return replace(c, 3, makeCODEaload(b, makeCODEldc_string(str, NULL)));

    if (
          is_ldc_int(*c, &a) &&
          is_aload(next(*c), &b) &&
          is_swap(nextN(*c, 2))
      ) return replace(c, 3, makeCODEaload(b, makeCODEldc_int(a, NULL)));

  if (
        is_aload(*c, &a) &&
        is_aload(next(*c), &b) &&
        is_swap(nextN(*c, 2))
    ) return replace(c, 3, makeCODEaload(b, makeCODEaload(a, NULL)));
    if (
          is_iload(*c, &a) &&
          is_aload(next(*c), &b) &&
          is_swap(nextN(*c, 2))
      ) return replace(c, 3, makeCODEaload(b, makeCODEiload(a, NULL)));

  if (
        is_aload(*c, &a) &&
        is_iload(next(*c), &b) &&
        is_swap(nextN(*c, 2))
    ) return replace(c, 3, makeCODEiload(b, makeCODEaload(a, NULL)));

    if (
          is_aconst_null(*c) &&
          is_aload(next(*c), &b) &&
          is_swap(nextN(*c, 2))
      ) return replace(c, 3, makeCODEaload(b, makeCODEaconst_null(NULL)));


  return 0;
}

/*
new A
dup
invokenonvirtual d
aload b
swap
------------------>
aload b
new A
dup
invokenonvirtual d

==========OR==========

new A
dup
ldc e
invokenonvirtual d
aload b
swap
---------------->
aload b
new A
dup
ldc e
invokenonvirtual d
reason: load b first so there is no need to swap
*/

int simplify_swap_load_after_new(CODE **c)
{
    int b;
    int e;
    char* a = (char*) malloc(50 * sizeof(char));
    char* d = (char*) malloc(50 * sizeof(char));
    if (is_new(*c, &a) &&
        is_dup(next(*c)) &&
        is_invokenonvirtual(nextN(*c, 2), &d) &&
        is_aload(nextN(*c, 3), &b) &&
        is_swap(nextN(*c, 4))){
            return replace(c, 5, makeCODEaload(b, makeCODEnew(a, makeCODEdup(makeCODEinvokenonvirtual(d, NULL)))));
        }

    if (is_new(*c, &a) &&
        is_dup(next(*c)) &&
        is_ldc_int(nextN(*c, 2), &e) &&
        is_invokenonvirtual(nextN(*c, 3), &d) &&
        is_aload(nextN(*c, 3), &b) &&
        is_swap(nextN(*c, 5))){
            return replace(c, 6, makeCODEaload(b, makeCODEnew(a, makeCODEdup(makeCODEldc_int(e, makeCODEinvokenonvirtual(d, NULL))))));
        }

  return 0;
}





/* dup
 * istore x
 * pop
 * -------->
 * istore x
 */
int simplify_istore(CODE **c)
{ int x;
  if (is_dup(*c) &&
      is_istore(next(*c),&x) &&
      is_pop(next(next(*c)))) {
     return replace(c,3,makeCODEistore(x,NULL));
  }
  return 0;
}


/*
 * ldc x
 * mathOp1   e.g.idiv
 * ldc y
 * mathOp2
 * -------->
 * ldc z
 * mathOp3  // combine the effect of two operations if possible
 * reason: some math ops can be combined, e.g +1+2 = +3
 */
int simplify_math_ops(CODE **c)
{
    int x, y;
    int result = 0;
    if (is_ldc_int(*c, &x) &&
        (is_iadd(next(*c)) || is_isub(next(*c)) || is_imul(next(*c))) &&
        is_ldc_int(nextN(*c, 2), &y) &&
        (is_iadd(nextN(*c, 3)) || is_isub(nextN(*c, 3)) || is_imul(nextN(*c, 3)))){

            if (is_iadd(next(*c)) || is_isub(next(*c))){
                x = is_iadd(next(*c)) ? x : -x;
                if (is_iadd(nextN(*c, 3)) || is_isub(nextN(*c, 3))){
                    y = is_iadd(nextN(*c, 3)) ? y : -y;
                    result = x + y;
                    if (result == 0 ) return replace(c, 4, NULL);
                    if (result > 0) return replace(c, 4, makeCODEldc_int(result, makeCODEiadd(NULL)));
                    else return replace(c, 4, makeCODEldc_int(result, makeCODEisub(NULL)));
                }
            }

            else if (is_imul(next(*c))){
                if (is_imul(nextN(*c, 3))){
                    result = x*y;
                    if (result == 1 ) return replace(c, 4, NULL);
                    return replace(c, 4, makeCODEldc_int(result, makeCODEimul(NULL)));
                }

            }
        }
  return 0;
}


/*
 * return
 * nop
 * -------->
 * nop after return can be removed safely
 */
int simplify_nop(CODE **c)
{
  if (is_ireturn(*c) && is_nop(next(*c))) {
     return replace(c,2, makeCODEireturn(NULL));
  }
  if (is_areturn(*c) && is_nop(next(*c))){
      return replace(c,2, makeCODEareturn(NULL));
  }

  return 0;
}

/* iload x
 * ldc k   (0<=k<=127)
 * iadd
 * istore x
 * --------->
 * iinc x k
 */
int positive_increment(CODE **c)
{ int x,y,k;
  if (is_iload(*c,&x) &&
      is_ldc_int(next(*c),&k) &&
      is_iadd(next(next(*c))) &&
      is_istore(next(next(next(*c))),&y) &&
      x==y && 0<=k && k<=127) {
     return replace(c,4,makeCODEiinc(x,k,NULL));
  }
  return 0;
}

/* iload x
 * ldc k   (0<=k<=128)
 * isub
 * istore x
 * --------->
 * iinc x -k
 */
int positive_decrement(CODE **c)
{ int x,y,k;
  if (is_iload(*c,&x) &&
      is_ldc_int(next(*c),&k) &&
      is_isub(next(next(*c))) &&
      is_istore(next(next(next(*c))),&y) &&
      x==y && 0<=k && k<=128) {
     return replace(c,4,makeCODEiinc(x,-k,NULL));
  }
  return 0;
}




/*
   if_icmpX l1
   ldc a (a = 0)
   goto l2
   l1:
   ldc b (b = 1)
   l2:
   ifeq l3
   ----->
   oppositeOfIf_icmpX l3    e.g if if_icmpX is is_if_icmplt, then oppositeOfIf_icmpX should be is_if_icmpge
   (l1, l2 reference count reduced by 1)

   reason: basically simplify the control struture
 */
 int simplify_cmp_goto(CODE **c)
 { int l1, l1t, l2, l2t, l3;
   int a, b;
   if ( (is_if_icmplt(*c, &l1) || is_if_icmpeq(*c, &l1) || is_if_icmpge(*c, &l1) || is_if_icmpgt(*c, &l1) || is_if_icmple(*c, &l1) || is_if_icmpne(*c, &l1) || is_ifeq(*c, &l1) || is_ifne(*c, &l1) ) &&
        is_ldc_int(next(*c), &a) && a == 0 &&
        is_goto(nextN(*c, 2), &l2) &&
        is_label(nextN(*c, 3), &l1t) && l1t == l1 &&
        is_ldc_int(nextN(*c, 4), &b) && b == 1 &&
        is_label(nextN(*c, 5), &l2t) && l2t == l2 &&
        is_ifeq(nextN(*c, 6),  &l3)) {
            droplabel(l1);
            droplabel(l2);
            if (is_if_icmplt(*c, &l1)) return replace(c,7,makeCODEif_icmpge(l3,NULL));
            if (is_if_icmpge(*c, &l1)) return replace(c,7,makeCODEif_icmplt(l3,NULL));
            if (is_if_icmpeq(*c, &l1)) return replace(c,7,makeCODEif_icmpne(l3,NULL));
            if (is_if_icmpne(*c, &l1)) return replace(c,7,makeCODEif_icmpeq(l3,NULL));
            if (is_if_icmpgt(*c, &l1)) return replace(c,7,makeCODEif_icmple(l3,NULL));
            if (is_if_icmple(*c, &l1)) return replace(c,7,makeCODEif_icmpgt(l3,NULL));
            if (is_ifne(*c, &l1)) return replace(c,7,makeCODEifeq(l3,NULL));
            if (is_ifeq(*c, &l1)) return replace(c,7,makeCODEifne(l3,NULL));
   }
   return 0;
 }



 /*
 ldc str
 dup
 ifnull l1
 goto l2
 l1:
 pop
 ldc "null"
 l2
 =================>
 (if input is null)
    ldc "null"
 (else)
    ldc str

 reason: we can compare if the string is null in advance and directly load the result directly
 */

 int null_string(CODE **c)
 { int l1,l2,l1t, l2t;
    char* str = (char*) malloc(100 * sizeof(char));
    char* strt = (char*) malloc(100 * sizeof(char));
   if (
       is_ldc_string(*c,&str) &&
       is_dup(next(*c)) &&
       is_ifnull(nextN(*c, 2), &l1) &&
       is_goto(nextN(*c, 3), &l2) &&
       is_label(nextN(*c, 4), &l1t) && l1t == l1 &&
       is_pop(nextN(*c, 5)) &&
       is_ldc_string(nextN(*c, 6), &strt) && strcmp(strt, "null") == 0 &&
       is_label(nextN(*c, 7), &l2t) && l2t == l2){
           droplabel(l1);
           droplabel(l2);
           if (str == NULL) return replace(c, 8, makeCODEldc_string("null", NULL));
           return replace(c, 8, makeCODEldc_string(str, NULL));
       }
       return 0;
   }


/*
 ifA l1
 goto l2
 l1
 --------------->
 oppositeOf(ifA) l2

 reason: ifA we go to l1, and the next line is goto l2, this means that !ifA we go to l2.
*/
int simplify_branch(CODE** c){
    int l1, l1t;
    int l2;
    if(
        (is_ifnull(*c, &l1) || is_ifeq(*c, &l1) ||  is_ifnonnull(*c, &l1) || is_ifne(*c, &l1)
        ||  is_if_icmpeq(*c, &l1) ||  is_if_icmpge(*c, &l1) ||  is_if_icmplt(*c, &l1) || is_if_icmple(*c, &l1) ||  is_if_icmpgt(*c, &l1) || is_if_icmpne(*c, &l1) || is_if_acmpeq(*c, &l1) || is_if_acmpne(*c, &l1)) &&
        is_goto(next(*c), &l2) &&
        is_label(nextN(*c, 2), &l1t) && l1 == l1t){
            droplabel(l1);
            if (is_ifnull(*c, &l1)) return replace(c, 3, makeCODEifnonnull(l2, NULL));
            if (is_ifnonnull(*c, &l1)) return replace(c, 3, makeCODEifnull(l2, NULL));
            if (is_ifeq(*c, &l1)) return replace(c, 3, makeCODEifne(l2, NULL));
            if (is_ifne(*c, &l1)) return replace(c, 3, makeCODEifeq(l2, NULL));
            if (is_if_icmpeq(*c, &l1)) return replace(c, 3, makeCODEif_icmpne(l2, NULL));
            if (is_if_icmpne(*c, &l1)) return replace(c, 3, makeCODEif_icmpeq(l2, NULL));
            if (is_if_acmpne(*c, &l1)) return replace(c, 3, makeCODEif_acmpeq(l2, NULL));
            if (is_if_acmpeq(*c, &l1)) return replace(c, 3, makeCODEif_acmpne(l2, NULL));
            if (is_if_icmpge(*c, &l1)) return replace(c, 3, makeCODEif_icmplt(l2, NULL));
            if (is_if_icmplt(*c, &l1)) return replace(c, 3, makeCODEif_icmpge(l2, NULL));
            if (is_if_icmpgt(*c, &l1)) return replace(c, 3, makeCODEif_icmple(l2, NULL));
            if (is_if_icmple(*c, &l1)) return replace(c, 3, makeCODEif_icmpgt(l2, NULL));
        }
    return 0;
}




/*
 *  iconst_0
 *  if_cmpeq/if_cmpne L
 *  --------->
 *  ifeq/ifne L
 *
 * reason: load 0 and compare with it is equal to ifeq/ifne
 */
int compare_zero(CODE **c)
{
    int x, l;
    if (is_ldc_int(*c, &x) && x == 0){
        if (is_if_icmpeq(next(*c), &l)){
            return replace(c, 2, makeCODEifeq(l, NULL));
        }
        else if (is_if_icmpne(next(*c), &l)){
            return replace(c, 2, makeCODEifne(l, NULL));
        }
    }
    return 0;
}



/* goto L1
 * ...
 * L1:
 * goto L2
 * ...
 * L2:
 * --------->
 * goto L2
 * ...
 * L1:    (reference count reduced by 1)
 * goto L2
 * ...
 * L2:    (reference count increased by 1)
 */
int simplify_goto_goto(CODE **c)
{ int l1,l2;
  if (is_goto(*c,&l1) && is_goto(next(destination(l1)),&l2) && l1>l2) {
     droplabel(l1);
     copylabel(l2);
     return replace(c,1,makeCODEgoto(l2,NULL));
  }
  return 0;
}
void init_patterns(void) {
	ADD_PATTERN(simplify_multiplication_right);
	ADD_PATTERN(simplify_astore);
	ADD_PATTERN(simplify_istore);
	ADD_PATTERN(positive_increment);
	ADD_PATTERN(simplify_goto_goto);
	ADD_PATTERN(simplify_cmp_goto);
	ADD_PATTERN(simplify_nop);
    ADD_PATTERN(simplify_dup_swap_putfield_pop);
    ADD_PATTERN(null_string);
    ADD_PATTERN(simplify_branch);
    ADD_PATTERN(simplify_swap);
    ADD_PATTERN(aconst_null_cmp);
    ADD_PATTERN(compare_zero);
    ADD_PATTERN(simplify_math_ops);
    ADD_PATTERN(dead_label);
    ADD_PATTERN(simplify_swap_load_after_new);
}
