<!DOCTYPE style-sheet PUBLIC "-//James Clark//DTD DSSSL Style Sheet//EN">

(style-sheet

 ;; ========================================================================
 ;; RELATIONAL UNIFICATION & GROVE SYNTHESIS KERNEL
 ;; Expression: 10 + (?x * 5) = 20  =>  ?x = 2
 ;; ========================================================================

 (define (lookup var env)
   (let ((cell (assoc var env)))
     (if cell (cdr cell) #f)))

 (define (unify-hole var candidate env)
   (let ((bound (lookup var env)))
     (cond
      ((not bound) (cons (cons var candidate) env))
      ((equal? bound candidate) env)
      (else #f))))

 (define (eval-grove-node node env)
   (let ((tag (gi node)))
     (cond
      ((string=? tag "INT")
       (string->number (data node)))
      ((string=? tag "HOLE")
       (let* ((var (attribute-string "var" node))
              (val (lookup var env)))
         (if val val (quote UNBOUND))))
      ((string=? tag "EXPR")
       (let* ((op-node    (node-list-first (select-elements (children node) "OP")))
              (left-node  (node-list-first (select-elements (children node) "LEFT")))
              (right-node (node-list-first (select-elements (children node) "RIGHT")))
              (op    (data op-node))
              (l-val (eval-grove-node (node-list-first (children left-node))  env))
              (r-val (eval-grove-node (node-list-first (children right-node)) env)))
         (cond
          ((or (equal? l-val (quote UNBOUND))
               (equal? r-val (quote UNBOUND))) (quote UNBOUND))
          ((string=? op "+") (+ l-val r-val))
          ((string=? op "*") (* l-val r-val))
          (else 0))))
      (else 0))))

 (define *target-invariant-value* 20)

 (define (synthesize-bindings hole-var candidates env)
   (let loop ((rest-candidates candidates))
     (if (null? rest-candidates)
         #f
         (let* ((cand     (car rest-candidates))
                (test-env (unify-hole hole-var cand env)))
           (if test-env
               (if (= (eval-grove-node (root-element) test-env)
                      *target-invariant-value*)
                   test-env
                   (loop (cdr rest-candidates)))
               (loop (cdr rest-candidates)))))))

 ;; CONSTRUCTION RULES
 (element GROVE
   (let* ((candidate-pool (quote (1 2 3 4 5)))
          (solved-env     (synthesize-bindings "?x" candidate-pool (quote ()))))
     (make scroll
       font-family-name: "Monospace"
       font-size:        10pt
       (make element
         gi:         "SYNTHESIZED-GROVE"
         attributes: (list (list "STATUS" (if solved-env "VERIFIED" "FAILED")))
         (process-children-with-env solved-env)))))

 (element EXPR
   (make element
     gi:         "EXPR"
     attributes: (list (list "ID" (attribute-string "id")))
     (process-children)))

 (element OP    (make element gi: "OP"    (make literal (data (current-node)))))
 (element LEFT  (make element gi: "LEFT"  (process-children)))
 (element RIGHT (make element gi: "RIGHT" (process-children)))
 (element INT   (make element gi: "INT"   (make literal (data (current-node)))))

 (element HOLE
   (let* ((var        (attribute-string "var"))
          (solved-val (lookup var (current-synthesis-env))))
     (if solved-val
         (make element
           gi:         "SYNTHESIZED-INT"
           attributes: (list (list "RESOLVED-FROM" var))
           (make literal (number->string solved-val)))
         (make element
           gi:         "UNRESOLVED-HOLE"
           attributes: (list (list "VAR" var))))))
)
