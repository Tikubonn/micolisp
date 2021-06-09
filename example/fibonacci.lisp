
(function fibonacchi (n)
  (if (<= n 1) n
    (+ (fibonacchi (- n 2)) (fibonacchi (- n 1)))))

(var count 0)

(while (<= count 30)
  (println (cons count (fibonacchi count)))
  (var count (+ count 1)))
