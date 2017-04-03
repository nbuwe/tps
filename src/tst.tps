/avg {10 exch repeat 9 {add} repeat 10 div} bind def
/rpt {exch /i exch def {i dup 1 sub /i exch def} exch while} bind def

/t1 {{usertime 10000 {} repeat usertime exch sub} avg} def
/t2 {{usertime 10000 {noop} repeat usertime exch sub} avg} def
/t0 { {t1 t2 exch sub} avg 10000 div} def

/tb1 {{usertime 10000 {} repeat usertime exch sub} avg} bind def
/tb2 {{usertime 10000 {noop} repeat usertime exch sub} avg} bind def
/tb0 {{tb1 tb2 exch sub} avg 10000 div} bind def

/T1 {{usertime 10000 {} rpt usertime exch sub} avg} def
/T2 {{usertime 10000 {noop} rpt usertime exch sub} avg} def
/T0 { {{t1} avg {t2} avg exch sub} avg } def
/T0 {{T1 T2 exch sub} avg 10000 div} def

/TB1 {{usertime 10000 {} rpt usertime exch sub} avg} bind def
/TB2 {{usertime 10000 {noop} rpt usertime exch sub} avg} bind def
/TB0 {{TB1 TB2 exch sub} avg 10000 div} bind def
