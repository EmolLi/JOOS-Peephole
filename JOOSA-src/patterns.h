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
}
