/fibb {dup 2 ge {dup 2 sub fibb exch 1 sub fibb add} if} bind def
/fibtimeb {usertime exch fibb usertime exch pop exch sub} bind def

/fib {dup 2 ge {dup 2 sub fib exch 1 sub fib add} if} def
/fibtime {usertime exch fib usertime exch pop exch sub} def
