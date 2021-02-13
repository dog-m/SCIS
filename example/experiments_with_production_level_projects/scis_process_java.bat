@set SCIS_TXL=../../txl_grammar.txl
@set SCIS_RULESET=../../ruleset_grammar.txl

@set SRC=./_normalized.java
@set DST=./_instrumented.java
@set RULESET=./rules/java.yml
@set ANNOTATION=../lang/java/annotation.xml
@set FRAGMENTS=../fragments/java/

@"../../Debug/scis" --src %SRC% --dst %DST% --ruleset %RULESET% --annotation %ANNOTATION% --fragments-dir %FRAGMENTS% --scis-grm-txl %SCIS_TXL% --scis-grm-ruleset %SCIS_RULESET%

@pause
