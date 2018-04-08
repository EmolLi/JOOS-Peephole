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


/* dup
 * aload x
 * swap
 * putfield
 * pop
 * -------->
 *aload x
 * swap
 * putfield
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
    load a
    load b
    swap
    -----
    load b
    load a
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
        is_ldc_int(*c, &a) &&
        is_aload(next(*c), &b) &&
        is_swap(nextN(*c, 2))
    ) return replace(c, 3, makeCODEaload(b, makeCODEldc_int(a, NULL)));

  return 0;
}



/* dup
 * ifne l1
 * pop
 * -------->
 *
 */
/*
int simplify_dup_ifne_pop(CODE **c)
{
    int x;
    if (is_dup(*c) &&
        is_ifne(next(*c),&x) &&
        is_pop(next(next(*c)))) {
        return replace(c,3,makeCODEifne(x,NULL));
     }
    return 0;
}*/



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
 *//*
int simplify_math_ops(CODE **c)
{
    int x, y;
    int result = 0;
    if (is_ldc_int(*c, &x) &&
        (is_iadd(next(*c)) || is_isub(next(*c)) || is_idiv(next(*c)) || is_imul(next(*c))) &&
        is_ldc_int(nextN(*c, 2), &y) &&
        (is_iadd(nextN(*c, 3)) || is_isub(nextN(*c, 3)) || is_idiv(nextN(*c, 3)) || is_imul(nextN(*c, 3)))){

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
                if (is_idiv(nextN(*c, 3))){
                    result = x/y;
                    if (result == 1) return replace(c, 4, NULL);
                }
            }
            else if (is_idiv(next(*c))){
                if (is_imul(nextN(*c, 3))){
                    result = y/x;
                    if (result == 1) return replace(c, 4, NULL);
                }
                if (is_idiv(nextN(*c, 3))){
                    result = y*x;
                    if (result == 1) return replace(c, 4, NULL);
                }
            }
        }
  return 0;
}
*/


/*
 * nop
 * -------->
 * -
 */
int simplify_nop(CODE **c)
{
  if (is_ireturn(*c) && is_nop(next(*c))) {
     return replace(c,2, makeCODEireturn(NULL));
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
 *//*
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
}*/




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
 */
 int simplify_cmp_goto(CODE **c)
 { int l1, l1t, l2, l2t, l3;
   int a, b;
   if ( (is_if_icmplt(*c, &l1) || is_if_icmpeq(*c, &l1) || is_if_icmpge(*c, &l1) || is_if_icmpgt(*c, &l1) || is_if_icmple(*c, &l1) || is_if_icmpne(*c, &l1)) &&
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
   }
   return 0;
 }


/*

 iload_2
 iconst_0
 if_icmpeq true_12

 iconst_0
 goto stop_13
 true_12:
 iconst_1
 stop_13:

 dup
 ifne true_11
 pop

 iload_2
 iconst_4
 if_icmpeq true_14

 iconst_0
 goto stop_15
 true_14:
 iconst_1
 stop_15:

 true_11:

 dup
 ifne true_10
 pop

 iload_2
 ldc 8
 if_icmpeq true_16

 iconst_0
 goto stop_17
 true_16:
 iconst_1
 stop_17:

 true_10:

 ifeq else_8
 aload_1
 ldc "|"



 typedef struct OrTerm {
     enum type { first, middle, last };
     int ild;
     int ldc;
     int l1;
     int l2;
     int l3;
     int l4;
     int l5;
     int l6;
     OrTerm* next;
} OrTerm;


 int simplify_or_terms(CODE **c){
     int cnt = 0;
     OrTerm* first = is_or_term(c);
     int line = 11;
     if (first != NULL && first->type == first){
         OrTerm* last;
         cnt++;
         OrTerm* next = is_or_term(nextN(c, line));
         while (next != NULL){
             cnt++;
             if (next->type == last) {
                 last = next;
                 break;
             }
             if (next->type == middle){
                 line += 12;
                 next = is_or_term(nextN(c, line));
             }
             else{
                 printf("errr\n");
                 exit(-1);
             }
         }


         if (cnt > 2 && last != NULL){
             OrTerm* cur = first;
             CODE* code = makeCODEif_icmpne()
             for (int i = 0; i < cnt; i++){

             }

         }
     }
 }


 OrTerm* is_or_term(CODE* c){
     OrTerm t = NEW(OrTerm);
     int iload;
     int ldc;
     int l1, l2, l3, l4, l5, l6;
     int a, b;
       if (
            is_iload(c, &iload) &&
            is_ldc_int(next(c), &ldc) &&
            is_if_icmpeq(nextN(c, 2), &l1)) &&
            is_ldc_int(nextN(c, 3), &a) && a == 0 &&
            is_goto(nextN(c, 4), &l2) &&
            is_label(nextN(c, 5), &l3) &&
            is_ldc_int(nextN(c, 6), &b) && b == 1 &&
            is_label(nextN(c, 7), &l4) &&
        ){
            if (is_label(nextN(c, 8), &l5) &&
                is_dup(nextN(c, 9)) &&
                is_ifne(nextN(c, 10), &l6) &&
                is_pop(nextN(c, 11))){
                    t->l1 = l1;
                    t->l2 = l2;
                    t->l3 = l3;
                    t->l4 = l4;
                    t->l5 = l5;
                    t->l6 = l6;
                    t->ild = iload;
                    t->ldc = ldc;
                    t->type = middle;
                    return t;   // middle term
                }

            if (is_dup(nextN(c, 8)) &&
                is_ifne(nextN(c, 9), &l6) &&
                is_pop(nextN(c, 10))){
                    t->l1 = l1;
                    t->l2 = l2;
                    t->l3 = l3;
                    t->l4 = l4;
                    t->l5 = l5;
                    t->l6 = l6;
                    t->ild = iload;
                    t->ldc = ldc;
                    t->type = first;
                    return t;
                }

            if (is_label(nextN(c, 8), &l5) &&
                is_ifeq(nextN(c, 9), &l6)){
                    t->l1 = l1;
                    t->l2 = l2;
                    t->l3 = l3;
                    t->l4 = l4;
                    t->l5 = l5;
                    t->l6 = l6;
                    t->ild = iload;
                    t->ldc = ldc;
                    t->type = last;
                    return t;
                }
        }
       }
       return NULL;
 }




 int simplify_cmp_goto(CODE **c)
 { int l1, l1t, l2, l2t, l3;
   int a, b;
   if ( (is_if_icmplt(*c, &l1) || is_if_icmpeq(*c, &l1) || is_if_icmpge(*c, &l1) || is_if_icmpgt(*c, &l1) || is_if_icmple(*c, &l1) || is_if_icmpne(*c, &l1)) &&
        is_ldc_int(next(*c), &a) && a == 0 &&
        is_goto(next(next(*c)), &l2) &&
        is_label(next(next(next(*c))), &l1t) && l1t == l1 &&
        is_ldc_int(next(next(next(next(*c)))), &b) && b == 1 &&
        is_label(next(next(next(next(next(*c))))), &l2t) && l2t == l2 &&
        is_ifeq(next(next(next(next(next(next(*c)))))), &l3)) {
            droplabel(l1);
            droplabel(l2);
            if (is_if_icmplt(*c, &l1)) return replace(c,7,makeCODEif_icmpge(l3,NULL));
            if (is_if_icmpge(*c, &l1)) return replace(c,7,makeCODEif_icmplt(l3,NULL));
            if (is_if_icmpeq(*c, &l1)) return replace(c,7,makeCODEif_icmpne(l3,NULL));
            if (is_if_icmpne(*c, &l1)) return replace(c,7,makeCODEif_icmpeq(l3,NULL));
            if (is_if_icmpgt(*c, &l1)) return replace(c,7,makeCODEif_icmple(l3,NULL));
            if (is_if_icmple(*c, &l1)) return replace(c,7,makeCODEif_icmpgt(l3,NULL));
   }
   return 0;
 }
*/
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
    ADD_PATTERN(simplify_swap);
}
